#ifndef VHVC_COMMON
#define VHVC_COMMON
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <SDL.h>
#if __GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ >= 1
#	define always_inline __attribute__((__always_inline__)) inline
#elif defined(_MSC_VER)
#	define always_inline __forceinline
#else
#	define always_inline inline
#endif
#endif
