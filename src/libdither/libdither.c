#include <stdlib.h>
#include "libdither.h"

#ifdef __cplusplus
#define INITIALIZER(f) \
        static void f(void); \
        struct f##_t_ { f##_t_(void) { f(); } }; static f##_t_ f##_; \
        static void f(void)
#elif defined(_MSC_VER)
#pragma section(".CRT$XCU",read)
#define INITIALIZER2_(f,p) \
        static void f(void); \
        __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
        __pragma(comment(linker,"/include:" p #f "_")) \
        static void f(void)
#ifdef _WIN64
#define INITIALIZER(f) INITIALIZER2_(f,"")
#else
#define INITIALIZER(f) INITIALIZER2_(f,"_")
#endif
#else
#define INITIALIZER(f) \
        static void f(void) __attribute__((constructor)); \
        static void f(void)
#endif

static void finalize(void) {
    // library finalizer code goes here...
}

INITIALIZER(initialize) {
    // library initializer code goes here...
    atexit(finalize);
}

MODULE_API const char* libdither_version() {
#ifdef LIB_VERSION
    static char version[] = LIB_VERSION;
#else
    static char version[] = "unknown";
#endif
    return version;
}
