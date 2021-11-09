import os

def find_skymp_root():
    current_dir = os.path.realpath(os.path.dirname(os.path.abspath(__file__)))
    max_directory_depth = 5
    root = current_dir
    for i in range(max_directory_depth):
        root = os.path.realpath(os.path.join(root, ".."))
        dist_path = os.path.join(root, "build/dist")
        if os.path.exists(dist_path):
            return root
    print("Couldn't find skymp root")
    sys.exit(1)
