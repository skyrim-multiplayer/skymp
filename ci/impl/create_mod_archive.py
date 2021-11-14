import getopt
import sys
import os
import re
from zipfile import ZipFile
from zipfile import ZIP_DEFLATED
from utils import find_skymp_root

def filter_skyrim_platform(path):
    if re.search(".*(SkyrimSoulsRE\.(dll|ini)|version-.*\.bin|skymp5-client.js|skymp5-client-settings.txt)", path):
        return False
    return True
    
def pack_skyrim_platform():
    dist_client_path = os.path.join(find_skymp_root(), "build/dist/client/data")
    zip_files_in_dir(dist_client_path, "SkyrimPlatform.zip", filter_skyrim_platform)

def main(argv):
    mod_name = get_mod_name(argv)
    if mod_name == "SkyrimPlatform":
        pack_skyrim_platform()
    else:
        print("Unknown mod_name")
        sys.exit(1)

def get_mod_name(argv):
    try:
        opts, args = getopt.getopt(argv,"",["mod_name="])
    except getopt.GetoptError:
        print("create_mod_archive.py --mod_name <mod_name>")
        sys.exit(1)
    for opt, arg in opts:
        if opt in ("--mod_name"):
            return arg
            
# Zip the files from given directory that matches the filter
def zip_files_in_dir(dir_name, zip_file_name, filter):
   # create a ZipFile object
   with ZipFile(zip_file_name, 'w', ZIP_DEFLATED) as zip_obj:
       # Iterate over all the files in directory
       for folder_name, subfolders, filenames in os.walk(dir_name):
           for filename in filenames:
               # create complete filepath of file in directory
               file_path = os.path.join(folder_name, filename)
               if filter(file_path):
                   # Add file to zip
                   zip_obj.write(file_path, os.path.relpath(file_path, dir_name))

if __name__ == "__main__":
    main(sys.argv[1:])
