#include "device.h"
#include "image.h"

#include <cstring>
#include <climits>
#include <thread>
#include <algorithm>

using namespace rsimpl;

static_device_info rsimpl::add_standard_unpackers(const static_device_info & device_info)
{
    static_device_info info = device_info;
    for(auto & mode : device_info.subdevice_modes)
    {
        // Unstrided YUYV modes can be unpacked into RGB and BGR
        if(mode.pf->fourcc == 'YUY2' && mode.unpacker == &unpack_subrect && mode.width == mode.streams[0].width && mode.height == mode.streams[0].height)
        {
            auto m = mode;
            m.streams[0].format = RS_FORMAT_RGB8;
            m.unpacker = &unpack_rgb_from_yuy2;
            info.subdevice_modes.push_back(m);

            m.streams[0].format = RS_FORMAT_BGR8;
            m.unpacker = &unpack_bgr_from_yuy2;
            info.subdevice_modes.push_back(m);

            m.streams[0].format = RS_FORMAT_RGBA8;
            m.unpacker = &unpack_rgba_from_yuy2;
            info.subdevice_modes.push_back(m);

            m.streams[0].format = RS_FORMAT_BGRA8;
            m.unpacker = &unpack_bgra_from_yuy2;
            info.subdevice_modes.push_back(m);
        }
    }

    // Flag all standard options as supported
    for(int i=0; i<RS_OPTION_COUNT; ++i)
    {
        if(uvc::is_pu_control((rs_option)i))
        {
            info.option_supported[i] = true;
        }
    }

    return info;
}

rs_device::rs_device(std::shared_ptr<rsimpl::uvc::device> device, const rsimpl::static_device_info & info) : device(device), config(add_standard_unpackers(info)), capturing(false),
    depth(config, RS_STREAM_DEPTH), color(config, RS_STREAM_COLOR), infrared(config, RS_STREAM_INFRARED), infrared2(config, RS_STREAM_INFRARED2),
    rect_color(color), color_to_depth(color, depth), depth_to_color(depth, color), depth_to_rect_color(depth, rect_color)
{
    streams[RS_STREAM_DEPTH    ] = native_streams[RS_STREAM_DEPTH]     = &depth;
    streams[RS_STREAM_COLOR    ] = native_streams[RS_STREAM_COLOR]     = &color;
    streams[RS_STREAM_INFRARED ] = native_streams[RS_STREAM_INFRARED]  = &infrared;
    streams[RS_STREAM_INFRARED2] = native_streams[RS_STREAM_INFRARED2] = &infrared2;
    streams[RS_STREAM_RECTIFIED_COLOR]                                 = &rect_color;
    streams[RS_STREAM_COLOR_ALIGNED_TO_DEPTH]                          = &color_to_depth;
    streams[RS_STREAM_DEPTH_ALIGNED_TO_COLOR]                          = &depth_to_color;
    streams[RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR]                = &depth_to_rect_color;
}

rs_device::~rs_device()
{

}

void rs_device::enable_stream(rs_stream stream, int width, int height, rs_format format, int fps)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(config.info.stream_subdevices[stream] == -1) throw std::runtime_error("unsupported stream");

    config.requests[stream] = {true, width, height, format, fps};
    for(auto & s : native_streams) s->buffer.reset(); // Changing stream configuration invalidates the current stream info
}

void rs_device::enable_stream_preset(rs_stream stream, rs_preset preset)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(!config.info.presets[stream][preset].enabled) throw std::runtime_error("unsupported stream");

    config.requests[stream] = config.info.presets[stream][preset];
    for(auto & s : native_streams) s->buffer.reset(); // Changing stream configuration invalidates the current stream info
}

void rs_device::disable_stream(rs_stream stream)
{
    if(capturing) throw std::runtime_error("streams cannot be reconfigured after having called rs_start_device()");
    if(config.info.stream_subdevices[stream] == -1) throw std::runtime_error("unsupported stream");

    config.requests[stream] = {};
    for(auto & s : native_streams) s->buffer.reset(); // Changing stream configuration invalidates the current stream info
}

void rs_device::start()
{
    if(capturing) throw std::runtime_error("cannot restart device without first stopping device");
        
    auto selected_modes = config.info.select_modes(config.requests);

    for(auto & s : native_streams) s->buffer.reset(); // Starting capture invalidates the current stream info, if any exists from previous capture

    // Satisfy stream_requests as necessary for each subdevice, calling set_mode and
    // dispatching the uvc configuration for a requested stream to the hardware
    for(auto mode : selected_modes)
    {
        // Create a stream buffer for each stream served by this subdevice mode
        std::vector<std::shared_ptr<stream_buffer>> stream_list;
        for(auto & stream_mode : mode.streams)
        {
            // Create a buffer to receive the images from this stream
            auto stream = std::make_shared<stream_buffer>(stream_mode);
            stream_list.push_back(stream);
                    
            // If this is one of the streams requested by the user, store the buffer so they can access it
            if(config.requests[stream_mode.stream].enabled) native_streams[stream_mode.stream]->buffer = stream;
        }
                
        // Initialize the subdevice and set it to the selected mode
        int serial_frame_no = 0;
        bool only_stream = selected_modes.size() == 1;
        set_subdevice_mode(*device, mode.subdevice, mode.width, mode.height, mode.pf->fourcc, mode.fps, [mode, stream_list, only_stream, serial_frame_no](const void * frame) mutable
        {
            // Unpack the image into the user stream interface back buffer
            std::vector<byte *> dest;
            for(auto & stream : stream_list) dest.push_back(stream->get_back_data());
            mode.unpacker(dest.data(), reinterpret_cast<const byte *>(frame), mode);
            int frame_number = (mode.use_serial_numbers_if_unique && only_stream) ? serial_frame_no++ : mode.frame_number_decoder(mode, frame);
            if(frame_number == 0) frame_number = ++serial_frame_no; // No dinghy on LibUVC backend?
                
            // Swap the backbuffer to the middle buffer and indicate that we have updated
            for(auto & stream : stream_list) stream->swap_back(frame_number);
        });
    }
    
    on_before_start(selected_modes);
    start_streaming(*device, config.info.num_libuvc_transfer_buffers);
    capture_started = std::chrono::high_resolution_clock::now();
    capturing = true;
    base_timestamp = 0;
}

void rs_device::stop()
{
    if(!capturing) throw std::runtime_error("cannot stop device without first starting device");
    stop_streaming(*device);
    capturing = false;
}

void rs_device::wait_all_streams()
{
    if(!capturing) return;

    // Determine timeout time
    const auto timeout = std::max(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(500), capture_started + std::chrono::seconds(5));

    // Check if any of our streams do not have data yet, if so, wait for them to have data, and remember that we are on the first frame
    bool first_frame = false;
    for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
    {
        if(native_streams[i]->buffer && !native_streams[i]->buffer->is_front_valid())
        {
            while(!native_streams[i]->buffer->swap_front())
            {
                std::this_thread::sleep_for(std::chrono::microseconds(100)); // todo - Use a condition variable or something to avoid this
                if(std::chrono::high_resolution_clock::now() >= timeout) throw std::runtime_error("Timeout waiting for frames");
            }
            assert(native_streams[i]->buffer->is_front_valid());
            last_stream_timestamp = native_streams[i]->buffer->get_front_number();
            first_frame = true;
        }
    }

    // If this is not the first frame, check for new data on all streams, make sure that at least one stream provides an update
    if(!first_frame)
    {
        bool updated = false;
        while(true)
        {
            for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
            {
                if(native_streams[i]->buffer && native_streams[i]->buffer->swap_front())
                {
                    updated = true;
                }
            }
            if(updated) break;

            std::this_thread::sleep_for(std::chrono::microseconds(100)); // todo - Use a condition variable or something to avoid this
            if(std::chrono::high_resolution_clock::now() >= timeout) throw std::runtime_error("Timeout waiting for frames");
        }
    }

    // Determine the largest change from the previous timestamp
    int frame_delta = INT_MIN;    
    for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
    {
        if(!native_streams[i]->buffer) continue;
        int delta = native_streams[i]->buffer->get_front_number() - last_stream_timestamp; // Relying on undefined behavior: 32-bit integer overflow -> wraparound
        frame_delta = std::max(frame_delta, delta);
    }

    // Wait for any stream which expects a new frame prior to the frame time
    for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
    {
        if(!native_streams[i]->buffer) continue;
        int next_timestamp = native_streams[i]->buffer->get_front_number() + native_streams[i]->buffer->get_front_delta();
        int next_delta = next_timestamp - last_stream_timestamp;
        if(next_delta > frame_delta) continue;
        while(!native_streams[i]->buffer->swap_front())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100)); // todo - Use a condition variable or something to avoid this
            if(std::chrono::high_resolution_clock::now() >= timeout) throw std::runtime_error("Timeout waiting for frames");
        }
    }

    if(first_frame)
    {
        // Set time 0 to the least recent of the current frames
        base_timestamp = 0;
        for(int i=0; i<RS_STREAM_NATIVE_COUNT; ++i)
        {
            if(!native_streams[i]->buffer) continue;
            int delta = native_streams[i]->buffer->get_front_number() - last_stream_timestamp; // Relying on undefined behavior: 32-bit integer overflow -> wraparound
            if(delta < 0) last_stream_timestamp = native_streams[i]->buffer->get_front_number();
        }
    }
    else
    {
        base_timestamp += frame_delta;
        last_stream_timestamp += frame_delta;
    }    
}

int rs_device::get_frame_timestamp(rs_stream stream) const 
{ 
    if(!streams[stream]->is_enabled()) throw std::runtime_error(to_string() << "stream not enabled: " << stream); 
    return base_timestamp == -1 ? 0 : convert_timestamp(base_timestamp + streams[stream]->get_frame_number() - last_stream_timestamp);
}

const byte * rs_device::get_frame_data(rs_stream stream) const 
{ 
    if(!streams[stream]->is_enabled()) throw std::runtime_error(to_string() << "stream not enabled: " << stream);
    return streams[stream]->get_frame_data();
}

void rs_device::get_option_range(rs_option option, int * min, int * max) const
{
    if(!supports_option(option)) throw std::runtime_error(to_string() << "option not supported by this device - " << option);
    if(option >= RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE && option <= RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE)
    {
        if(min) *min = 0;
        if(max) *max = 1;
    }
    else if(uvc::is_pu_control(option))
    {           
        uvc::get_pu_control_range(get_device(), config.info.stream_subdevices[RS_STREAM_COLOR], option, min, max);
    }
    else
    {
        get_xu_range(option, min, max);
    }
}

void rs_device::set_option(rs_option option, int value)
{
    if(!supports_option(option)) throw std::runtime_error(to_string() << "option not supported by this device - " << option);
    if(uvc::is_pu_control(option))
    {
        uvc::set_pu_control(get_device(), config.info.stream_subdevices[RS_STREAM_COLOR], option, value);
    }
    else
    {
        set_xu_option(option, value);
    }
}

int rs_device::get_option(rs_option option) const
{
    if(!supports_option(option)) throw std::runtime_error(to_string() << "option not supported by this device - " << option);
    if(uvc::is_pu_control(option))
    {
        return uvc::get_pu_control(get_device(), config.info.stream_subdevices[RS_STREAM_COLOR], option);
    }
    else
    {
        return get_xu_option(option);
    }
}
