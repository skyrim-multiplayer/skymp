#pragma once

#ifdef WIN32
#  define NOMINMAX
#  include <Windows.h>

namespace Tools {
void CreateMiniDumpOnUnHandledException();
}

#endif
