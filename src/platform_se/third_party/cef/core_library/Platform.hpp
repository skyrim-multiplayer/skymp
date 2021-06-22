#pragma once

// Do mac someday ?
#ifdef _WIN32
#define TP_PLATFORM_WINDOWS 1
#define TP_PLATFORM_LINUX 0
#else
#define TP_PLATFORM_WINDOWS 0
#define TP_PLATFORM_LINUX 1
#endif

#if defined(_LP64) || defined(_WIN64)
#define TP_PLATFORM_64 1
#define TP_PLATFORM_32 0
#else
#define TP_PLATFORM_64 0
#define TP_PLATFORM_32 1
#endif

#ifdef NDEBUG
#define TP_RELEASE 1
#define TP_DEBUG 0
#else
#define TP_RELEASE 0
#define TP_DEBUG 1
#endif

#define TP_UNUSED(x) (void)x;
