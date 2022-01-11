import sys
import os
import shutil
from utils import find_skymp_root

def main(argv):
    skymp_root = find_skymp_root()
    cache = [
        "build",
        "skymp5-client/node_modules",
        "skymp5-front/node_modules",
        "skymp5-functions-lib/node_modules",
        "skymp5-server/cmake-js-fetch-build",
        "skymp5-server/node_modules",
        "skyrim-platform/tools/dev_service/node_modules"
    ]
    for cache_path_relative in cache:
        cache_path_full = os.path.join(skymp_root, cache_path_relative)
        if os.path.exists(cache_path_full):
            print("Deleting", cache_path_full)
            shutil.rmtree(cache_path_full)
        else:
            print("Skipping missing", cache_path_full)

if __name__ == "__main__":
    main(sys.argv[1:])
