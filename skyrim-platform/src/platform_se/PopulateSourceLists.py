import os

CMAKE_DIR = "cmake"
LIBRARIES = ("tilted_core", "tilted_reverse", "tilted_hooks", "tilted_ui", "tilted_ui_process", "skyrim_platform", "skyrim_platform_entry")
HEADER_TYPES = (".h", ".hpp", ".hxx")
SOURCE_TYPES = (".c", ".cpp", ".cxx")
ALL_TYPES = HEADER_TYPES + SOURCE_TYPES

def parse_sources(lib, directory):
  headerlist = list()
  sourcelist = list()
  for dirpath, dirnames, filenames in os.walk(directory):
    for filename in filenames:
      if filename.endswith(HEADER_TYPES):
        headerlist.append(os.path.normpath(os.path.join(dirpath, filename)))
      if filename.endswith(SOURCE_TYPES):
        sourcelist.append(os.path.normpath(os.path.join(dirpath, filename)))

  out = open(CMAKE_DIR + "/" + lib + "_headers.cmake", "w", encoding="utf8")
  out.write("set(" + lib + "_headers" + " ${" + lib + "_headers" + "}\n")
  for header in headerlist:
    name = header.replace("\\", "/")
    out.write("\t" + name + "\n")
  out.write(")\n")

  out = open(CMAKE_DIR + "/" + lib + "_sources.cmake", "w", encoding="utf8")
  out.write("set(" + lib + "_sources" + " ${" + lib + "_sources" + "}\n")
  for source in sourcelist:
    name = source.replace("\\", "/")
    out.write("\t" + name + "\n")
  out.write(")\n")


def populate_sourcelists():
  for lib in LIBRARIES:
    if lib == "tilted_core":
      directory = "../tilted/core_library"
    if lib == "tilted_reverse":
      directory = "../tilted/reverse"
    if lib == "tilted_hooks":
      directory = "../tilted/hooks"
    if lib == "tilted_ui":
      directory = "../tilted/ui"
    if lib == "tilted_ui_process":
      directory = "../tilted/ui_process"
    if lib == "skyrim_platform":
      directory = "skyrim_platform"
    if lib == "skyrim_platform_entry":
      directory = "skyrim_platform_entry"

    parse_sources(lib, directory)

def main():
  cur = os.path.dirname(os.path.realpath(__file__))
  os.chdir(cur)

  if not os.path.exists(CMAKE_DIR):
    os.makedirs(CMAKE_DIR)

  populate_sourcelists()

if __name__ == "__main__":
  main()
