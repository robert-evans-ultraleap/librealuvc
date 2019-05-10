#pragma once

#ifdef LEAPLIBRARY_EXPORTS
#define LEAPLIBRARY_API __declspec(dllexport)
#else
#define LEAPLIBRARY_API __declspec(dllimport)
#endif

extern "C" LEAPLIBRARY_API
int triple_input(int input);

extern "C" LEAPLIBRARY_API
uint32_t try_get_image(uint8_t* write_addr, uint32_t size);
