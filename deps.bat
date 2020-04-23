IF EXIST vcpkg GOTO REPO_EXISTS
git clone https://github.com/Microsoft/vcpkg.git
:REPO_EXISTS

cd vcpkg
git fetch
git checkout f29a191d0afccc3ed6f481283d6d15e0186096ae .

IF EXIST vcpkg.exe GOTO VCPKG_EXISTS
	call .\bootstrap-vcpkg.bat
:VCPKG_EXISTS

.\vcpkg install chakracore:x64-windows chakracore:x86-windows
.\vcpkg install getopt:x64-windows

cd ..
