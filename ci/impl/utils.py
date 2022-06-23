import os
import json
import sys

def find_skymp_root():
    current_dir = os.path.realpath(os.path.dirname(os.path.abspath(__file__)))
    max_directory_depth = 5
    root = current_dir
    for i in range(max_directory_depth):
        root = os.path.realpath(os.path.join(root, ".."))
        dist_path = os.path.join(root, ".github\workflows")
        if os.path.exists(dist_path):
            return root
    print("Couldn't find skymp root")
    sys.exit(1)

def get_sp_version():
    sp_root = os.path.join(find_skymp_root(), "skyrim-platform")
    sp_package_json = os.path.join(sp_root, "package.json")
    with open(sp_package_json) as json_file:
        data = json.load(json_file)
        return data["version"]
        
def set_sp_version(new_version_str):
    sp_root = os.path.join(find_skymp_root(), "skyrim-platform")
    sp_package_json = os.path.join(sp_root, "package.json")
    data = None
    with open(sp_package_json) as json_file:
        data = json.load(json_file)
    with open(sp_package_json, "w") as json_file:
        data["version"] = new_version_str
        # newline is for linelint (github actions)
        json_file.write(json.dumps(data, indent=2) + "\n")
