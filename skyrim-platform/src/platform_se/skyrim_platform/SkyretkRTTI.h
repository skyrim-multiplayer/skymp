// ============================================================================
// dump_rtti/RTTI.h
// Part of the Skyrim64 Reverse Engineering Toolkit (SkyRETK)
//
// Copyright (c) 2022 Nox Sidereum (for 64-bit Skyrim)
//               2017 Himika (for 32-bit Skyrim)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// (The MIT License)
// ============================================================================
#pragma once

#include <cstdint>
#include <list>
#include <map>

// #include "common/ITypes.h"

// ============================================================================
//                Section Offsets (from base module address)
// ----------------------------------------------------------------------------
// These are all correct for Skyrim 1.6.659 (GOG version).
// Refer MODULE SUMMARY header in skyretk_dump_rtti.log.
// ============================================================================
// 0: .text (+rx)
// const uint64_t TEXT_SEG_BEGIN = 0x00001000; // start
// const uint64_t PURE_CALL_ADDR = 0x01471648;
// const uint64_t TEXT_SEG_END = 0x015fcb8c; // end
//
//// 1: .rdata (+r)
// const uint64_t RDATA_SEG_BEGIN = 0x015fd000; // start
// const uint64_t TYPE_INFO_VTBL = 0x019752c0;
// const uint64_t RDATA_SEG_END = 0x01e3c276; // end
//
//// 2: .data (+rw)
// const uint64_t DATA_SEG_BEGIN = 0x01e3d000; // start
// const uint64_t DATA_SEG_END = 0x0352baf0;   // end

#include "Windows.h"

inline bool FindSectionOffsets(HMODULE hModule, const char* sectionName,
                        uintptr_t& start, uintptr_t& end)
{
  auto base = (uintptr_t)hModule;
  auto dos = (IMAGE_DOS_HEADER*)base;
  auto nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
  auto section = IMAGE_FIRST_SECTION(nt);

  for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, section++) {
    if (strncmp((char*)section->Name, sectionName, IMAGE_SIZEOF_SHORT_NAME) ==
        0) {
      start = base + section->VirtualAddress;
      end = start + section->Misc.VirtualSize;
      return true;
    }
  }
  return false;
}

inline std::pair<uintptr_t, uintptr_t> GetTextStartEnd()
{
  uintptr_t textStart, textEnd;
  FindSectionOffsets(GetModuleHandle(NULL), ".text", textStart, textEnd);
  return { textStart, textEnd };
}

inline std::pair<uintptr_t, uintptr_t> GetRdataStartEnd()
{
  uintptr_t rdataStart, rdataEnd;
  FindSectionOffsets(GetModuleHandle(NULL), ".rdata", rdataStart, rdataEnd);
  return { rdataStart, rdataEnd };
}

inline std::pair<uintptr_t, uintptr_t> GetDataStartEnd()
{
  uintptr_t dataStart, dataEnd;
  FindSectionOffsets(GetModuleHandle(NULL), ".data", dataStart, dataEnd);
  return { dataStart, dataEnd };
}

#define TEXT_SEG_BEGIN (static_cast<uint64_t>(GetTextStartEnd().first))
#define TEXT_SEG_END (static_cast<uint64_t>(GetTextStartEnd().second))

#define RDATA_SEG_BEGIN (static_cast<uint64_t>(GetRdataStartEnd().first))
#define RDATA_SEG_END (static_cast<uint64_t>(GetRdataStartEnd().second))

#define DATA_SEG_BEGIN (static_cast<uint64_t>(GetDataStartEnd().first))
#define DATA_SEG_END (static_cast<uint64_t>(GetDataStartEnd().second))

// ============================================================================
//                          RTTI structures.
// ----------------------------------------------------------------------------
// For more info see the excellent article by Igor Skochinsky
// at http://www.openrce.org/articles/full_view/23.
// In the below, all OFFSETs are relative to the module's base address.
// ============================================================================
struct TypeDescriptor
{
  uint64_t pVFTable; // 00: points to type_info's vftable.
  uint64_t spare;    // 08: unused field (currently always set to nullptr).
  char name[];       // 10: null-terminated string with the mangled type name.
};

// "The PMD structure describes how a base class is placed inside
//  the complete class. In the case of simple inheritance it is
//  situated at a fixed offset from the start of object, and that
//  value is the _mdisp_ field. If it's a virtual base, an additional
//  offset needs to be fetched from the vbtable."
//        - http://www.openrce.org/articles/full_view/23
struct PMD
{
  uint32_t mdisp; // 00: member displacement
  uint32_t pdisp; // 04: vbtable displacement
  uint32_t vdisp; // 08: displacement inside vbtable
};

// Each entry in the Base Class Array has the following structure.
// "The Base Class Array describes all base classes together with information
// which
//  allows the compiler to cast the derived class to any of them during
//  execution of the _dynamic_cast_ operator."
//        - http://www.openrce.org/articles/full_view/23
struct RTTIBaseClassDescriptor
{
  uint32_t pTypeDescriptor;   // 00: contains the OFFSET to the object's
                              // TypeDescriptor.
  uint32_t numContainedBases; // 04: number of contained bases
  struct PMD where;           // 08: pointer-to-member displacement info
  uint32_t attributes;        // 14: flags, usually 0
};

// "Class Hierarchy Descriptor describes the inheritance hierarchy
//  of the class. It is shared by all COLs for a class."
//        - http://www.openrce.org/articles/full_view/23
struct RTTIClassHierarchyDescriptor
{
  uint32_t signature;       // 00: always 0?
  uint32_t attributes;      // 04: always 0?
  uint32_t numBaseClasses;  // 08: number of elements in the RTTIBaseClassArray
  uint32_t pBaseClassArray; // 0C: contains the OFFSET to the first pointer in
                            // the RTTIBaseClassArray.
};

// "MSVC compiler puts a pointer to the structure called "Complete Object
// Locator" [COL]
//  just before the vftable. The structure is called so because it allows
//  compiler to find the location of the complete object from a specific
//  vftable pointer (since a class can have several of them)."
//        - http://www.openrce.org/articles/full_view/23
constexpr auto COL_SIG_REV1 = 1;

struct RTTICompleteObjectLocator
{
  uint32_t signature; // 00: for x64, this is COL_SIG_REV1
  uint32_t offset;    // 04: offset from complete object to this sub-object.
  uint32_t cdOffset;  // 08: the constructor displacement's offset.
  uint32_t pTypeDescriptor;  // 0C: contains the OFFSET to this object's
                             // TypeDescriptor.
  uint32_t pClassDescriptor; // 10: contains the OFFSET to this object's
                             // RTTIClassHierarchyDescriptor.
  uint32_t pSelf; // 14: contains the OFFSET to this RTTICompleteObjectLocator.
};

typedef std::list<uint64_t*> VtblList;

// ============================================================================
//                             Functions.
// ============================================================================
// public:
void LoadVTables(const uint64_t baseAddr,
                 std::map<uint64_t, VtblList>& vtblMap);

void PrintVirtuals(const uint64_t baseAddr,
                   const std::map<uint64_t, VtblList> vtblMap);

void DumpObjectClassHierarchy(const uint64_t* vtbl, const bool verbose,
                              const uint64_t baseAddr);

// private:
static void UnmangleRTTITypeName(const char* mangled, std::string& unmangled);

static void GetUnmangledTypeName(const TypeDescriptor* type,
                                 const uint64_t baseAddr,
                                 std::string& unmangled);

static const TypeDescriptor* GetTypeDescriptor(const uint64_t* vtbl,
                                               const uint64_t baseAddr);

static uint64_t* GetParentVtbl(const uint64_t* vtbl,
                               const std::map<uint64_t, VtblList> vtblMap,
                               const uint64_t baseAddr);

static bool GetTypeHierarchyInfo(const uint64_t* vtbl, std::string& name,
                                 uint32_t& offset,
                                 RTTIClassHierarchyDescriptor*& hierarchy,
                                 const uint64_t baseAddr);

static void GetObjectClassName(const uint64_t* vtbl, const uint64_t baseAddr,
                               std::string& name);

static void SimpleFunctionDecompiler(const uint64_t funcAddr,
                                     std::string& retOut,
                                     std::string& paramsOut,
                                     std::string& bodyOut,
                                     const uint64_t baseAddr);
