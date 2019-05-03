/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

 #ifndef LIBREALUVC_RU_EXCEPTION_H
 #define LIBREALUVC_RU_EXCEPTION_H 1
 
 #include <exception>
 
 namespace librealuvc {
 
 enum ru_exception_type {
   RU_EXCEPTION_TYPE_UNKNOWN,
   RU_EXCEPTION_TYPE_CAMERA_DISCONNECTED,
   RU_EXCEPTION_TYPE_BACKEND,
   RU_EXCEPTION_TYPE_INVALID_VALUE,
   RU_EXCEPTION_TYPE_WRONG_API_CALL_SEQUENCE,
   RU_EXCEPTION_TYPE_NOT_IMPLEMENTED,
   RU_EXCEPTION_TYPE_DEVICE_IN_RECOVERY_MODE,
   RU_EXCEPTION_TYPE_IO,
   RU_EXCEPTION_TYPE_COUNT
 };
 
 typedef ru_exception_type rs2_exception_type;
 
 class ru_exception : public std::exception {
  public:
   string what_;
   ru_exception_type type_;
  
  public:
   ru_exception(const string& what, ru_exception_type t = RU_EXCEPTION_TYPE_UNKNOWN) :
     what_(what),
     type_(t) {
   }
   
   virtual const char* what() const noexcept override { return what_.c_str(); }

   inline ru_exception_type get_type() const { return type_; }
 };
 
 template<ru_exception_type T>
 class ru_exception_sub : public ru_exception {
  public:
   ru_exception_sub(const string& what) :
     ru_exception(what, T) {
   }
 };
 
 typedef ru_exception_sub<RU_EXCEPTION_TYPE_CAMERA_DISCONNECTED> camera_disconnected_exception;
 typedef ru_exception_sub<RU_EXCEPTION_TYPE_BACKEND> backend_exception;
 typedef ru_exception_sub<RU_EXCEPTION_TYPE_INVALID_VALUE> invalid_value_exception;
 typedef ru_exception_sub<RU_EXCEPTION_TYPE_WRONG_API_CALL_SEQUENCE> wrong_api_call_sequence_exception;
 typedef ru_exception_sub<RU_EXCEPTION_TYPE_NOT_IMPLEMENTED> not_implemented_exception;
 typedef ru_exception_sub<RU_EXCEPTION_TYPE_IO> io_exception;

 class linux_backend_exception : public backend_exception {
  public:
   linux_backend_exception(const string& what) :
     backend_exception(what) {
   }
 };
   
 class windows_backend_exception : public backend_exception {
  public:
   windows_backend_exception(const string& what) :
     backend_exception(what) {
   }
 };
   
 }
 
 #endif
 