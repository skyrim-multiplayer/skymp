#include "AnyBackend.h"

AnyBackend& AnyBackend::GetInstanceForCurrentThread() {
    thread_local AnyBackend g_instance;
    return g_instance; 
}
