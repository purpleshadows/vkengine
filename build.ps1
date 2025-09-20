cmake -S . -B build -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE=F:/vcpkg-2025.08.27/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic `
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
 
Copy-Item -Force build\compile_commands.json compile_commands.json

cmake --build build
