## MIT License
##
## Copyright (c) 2019 vector-of-bool
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in all
## copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
## SOFTWARE.

# Bump this version to change what PMM version is downloaded
set(PMM_VERSION_INIT 1.4.2)

# Helpful macro to set a variable if it isn't already set
macro(_pmm_set_if_undef varname)
    if(NOT DEFINED "${varname}")
        set("${varname}" "${ARGN}")
    endif()
endmacro()

## Variables used by this script
# The version:
_pmm_set_if_undef(PMM_VERSION ${PMM_VERSION_INIT})
# The base URL we download PMM from:
_pmm_set_if_undef(PMM_URL_BASE "https://vector-of-bool.github.io/pmm")
# The real URL we download from (Based on the version)
_pmm_set_if_undef(PMM_URL "${PMM_URL_BASE}/${PMM_VERSION}")
# The directory where we store our downloaded files
_pmm_set_if_undef(PMM_DIR_BASE "${CMAKE_BINARY_DIR}/_pmm")
_pmm_set_if_undef(PMM_DIR "${PMM_DIR_BASE}/${PMM_VERSION}")

# The file that we first download
set(_PMM_ENTRY_FILE "${PMM_DIR}/entry.cmake")

# Guard against multiple processes trying to use the PMM dir simultaneously
file(LOCK "${PMM_DIR}/_init-pmm"
    GUARD PROCESS
    TIMEOUT 10
    RESULT_VARIABLE _lock_res
    )
if(NOT _lock_res STREQUAL "0")
    message(WARNING "PMM entry didn't lock the directory ${PMM_DIR} successfully (${_lock_res}). We'll continue as best we can.")
    set(_pmm_init_did_lock FALSE)
else()
    set(_pmm_init_did_lock TRUE)
endif()

if(NOT EXISTS "${_PMM_ENTRY_FILE}" OR PMM_ALWAYS_DOWNLOAD)
    file(
        DOWNLOAD "${PMM_URL}/entry.cmake"
        "${_PMM_ENTRY_FILE}.tmp"
        STATUS pair
        )
    list(GET pair 0 rc)
    list(GET pair 1 msg)
    if(rc)
        message(FATAL_ERROR "Failed to download PMM entry file")
    endif()
    file(RENAME "${_PMM_ENTRY_FILE}.tmp" "${_PMM_ENTRY_FILE}")
endif()

# ^^^ DO NOT CHANGE THIS LINE vvv
set(_PMM_BOOTSTRAP_VERSION 2)
# ^^^ DO NOT CHANGE THIS LINE ^^^

include("${_PMM_ENTRY_FILE}")

if(_pmm_init_did_lock)
    file(LOCK "${PMM_DIR}/_init-pmm" RELEASE)
endif()