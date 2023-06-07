# This code is intended to run with cmake -P
# It's not recommended to include this file

file(REMOVE_RECURSE "./nexus")

set(dir dist/client/data)
file(COPY ${dir} DESTINATION "./nexus/sp")

file(GLOB_RECURSE files "./nexus/sp/*")
if(files)
  foreach(file ${files})
    if("${file}" MATCHES ".*(SkyrimSoulsRE\\.(dll|ini)|version.*\\.bin|skymp5-client.js|skymp5-client-settings.txt|Scripts/Source|uimenubase.pex|TIF__000361DF.pex|QF_MQ201_00035D5F.pex|playerbookshelfcontainerscript.pex|DLC2SummonDremoraMerchantScript.pex|Interface)")
      file(REMOVE_RECURSE "${file}")
      message(STATUS "Excluding ${file} from SP build")
    endif()
  endforeach()
endif()

if(WIN32)
  set(EXECUTABLE_SUFFIX ".exe")
else()
  set(EXECUTABLE_SUFFIX "")
endif()

file(COPY "dist/papyrus-vm/papyrus-vm${EXECUTABLE_SUFFIX}" DESTINATION "./nexus/papyrus-vm")
