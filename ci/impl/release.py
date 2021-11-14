import getopt
import sys
import os
from zipfile import ZipFile
from utils import find_skymp_root
from create_mod_archive import pack_skyrim_platform
from generate_changelog import generate_changelog

def release_skyrim_platform():
    # 1. Change verison in GetPlatformVersion (DevApi.cpp) and in version.ts (skymp5-client)
    # 2. Compile
    # 3. Let python do the job
    generate_changelog()
    pack_skyrim_platform()
    # 4. Pull request changes
    # 5. Upload archive to nexus
    
def main(argv):
    mod_name = get_mod_name(argv)
    if mod_name == "SkyrimPlatform":
        release_skyrim_platform()
    else:
        print("Unknown mod_name")
        sys.exit(1)
        

def get_mod_name(argv):
    try:
        opts, args = getopt.getopt(argv,"",["mod_name="])
    except getopt.GetoptError:
        print("release.py --mod_name <mod_name>")
        sys.exit(1)
    for opt, arg in opts:
        if opt in ("--mod_name"):
            return arg

if __name__ == "__main__":
    main(sys.argv[1:])
