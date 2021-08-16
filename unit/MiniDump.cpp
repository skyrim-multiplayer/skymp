// OpenCppCoverage is an open source code coverage for C++.
// Copyright (C) 2020 OpenCppCoverage
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "MiniDump.h"
#include <iostream>

#ifdef WIN32

#  pragma warning(push)
#  pragma warning(disable : 4091) // 'typedef ': ignored on left of '' when no
                                  // variable is declared
#  include <DbgHelp.h>
#  pragma warning(pop)

namespace Tools {
namespace {

//-----------------------------------------------------------------------------
MINIDUMP_TYPE GetMiniDumpDefaultType()
{
  return static_cast<MINIDUMP_TYPE>(
    MiniDumpWithDataSegs | MiniDumpWithPrivateReadWriteMemory |
    MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo);
}

//-----------------------------------------------------------------------------
void CreateMiniDump(MINIDUMP_EXCEPTION_INFORMATION& minidumpInfo, HANDLE hFile,
                    const char* dmpFilename)
{
  auto miniDumpType = GetMiniDumpDefaultType();

  std::cerr << "\tTrying to create memory dump..." << std::endl;
  if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
                        miniDumpType, &minidumpInfo, nullptr, nullptr)) {
    std::cerr << "\tMemory dump created successfully: " << dmpFilename
              << std::endl;
    std::cerr << "\tPlease create a new issue on ";
    std::cerr << "https://github.com/skyrim-multiplayer/skymp "
                 "and attached the memory dump ";
    std::cerr << dmpFilename << std::endl;
  } else
    std::cerr << "\tFailed to create memory dump." << std::endl;
}

//-----------------------------------------------------------------------------
LONG WINAPI
CreateMiniDumpOnUnHandledException(PEXCEPTION_POINTERS exceptionInfo)
{
  MINIDUMP_EXCEPTION_INFORMATION minidumpInfo;

  std::wcerr << L"Unexpected error occurs." << std::endl;

  minidumpInfo.ThreadId = GetCurrentThreadId();
  minidumpInfo.ExceptionPointers = exceptionInfo;
  minidumpInfo.ClientPointers = FALSE;

  const auto dmpFilename = TEXT("unit.dmp");
  HANDLE hFile = CreateFile(dmpFilename, GENERIC_WRITE, 0, nullptr,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (hFile != INVALID_HANDLE_VALUE) {
    CreateMiniDump(minidumpInfo, hFile, dmpFilename);
    CloseHandle(hFile);
  }
  abort();
  return 0;
}
}

//-------------------------------------------------------------------------
void CreateMiniDumpOnUnHandledException()
{
  DWORD dwMode = GetErrorMode();
  SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);

  SetUnhandledExceptionFilter(CreateMiniDumpOnUnHandledException);
}
}

#endif // WIN32
