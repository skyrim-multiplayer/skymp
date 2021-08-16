#pragma once

#ifdef WIN32
#  include <Windows.h>

namespace Tools {
void CreateMiniDumpOnUnHandledException();
}

#endif