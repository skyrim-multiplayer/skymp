import os

HEADER_TYPES = (".h", ".hpp", ".hxx")
SOURCE_TYPES = (".c", ".cpp", ".cxx")
ALL_TYPES = HEADER_TYPES + SOURCE_TYPES

def make_cmake():
	tmp = list()
	directories = ("include", "src")
	for directory in directories:
		for dirpath, dirnames, filenames in os.walk(directory):
			for filename in filenames:
				if filename.endswith(ALL_TYPES):
					path = os.path.join(dirpath, filename)
					tmp.append(os.path.normpath(path))

	headers = list()
	sources = list()
	for file in tmp:
		name = file.replace("\\", "/")
		if name.endswith(HEADER_TYPES):
			headers.append(name)
		elif name.endswith(SOURCE_TYPES):
			sources.append(name)

	def do_make(a_filename, a_varname, a_files):
		out = open("cmake/" + a_filename + ".cmake", "w", encoding="utf-8")
		out.write("set(" + a_varname + " ${" + a_varname + "}\n")

		for file in a_files:
			out.write("\t" + file + "\n")

		out.write(")\n")

	do_make("headerlist", "headers", headers)
	do_make("sourcelist", "sources", sources)

def main():
	cur = os.path.dirname(os.path.realpath(__file__))
	os.chdir(cur)
	make_cmake()

if __name__ == "__main__":
	main()
