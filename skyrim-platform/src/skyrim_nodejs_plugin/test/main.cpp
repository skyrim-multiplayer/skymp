#include <iostream>
#include <windows.h>

int main(int argc, char* argv[])
{
  std::cout << "Test started" << std::endl;

  auto handle = LoadLibrary("SkyrimNodeJS.dll");

  if (handle == NULL) {
    std::cout << "Failed to load library" << std::endl;
    return 1;
  }

  auto init =
    (int (*)(int, char**))GetProcAddress(handle, "SkyrimNodeJS_Init");
  auto createEnvironment = (int (*)(int, char**, void**))GetProcAddress(
    handle, "SkyrimNodeJS_CreateEnvironment");
  auto destroyEnvironment =
    (int (*)(void*))GetProcAddress(handle, "SkyrimNodeJS_DestroyEnvironment");
  auto tick = (int (*)(void*))GetProcAddress(handle, "SkyrimNodeJS_Tick");
  auto executeScript = (int (*)(void*, const char*))GetProcAddress(
    handle, "SkyrimNodeJS_ExecuteScript");
  auto getError = (uint64_t(*)(char*, uint64_t))GetProcAddress(
    handle, "SkyrimNodeJS_GetError");

  if (init == NULL || createEnvironment == NULL ||
      destroyEnvironment == NULL || tick == NULL || executeScript == NULL ||
      getError == NULL) {
    std::cout << "Failed to get function address" << std::endl;
    return 1;
  }

  if (init(argc, argv) != 0) {
    size_t errorSize = getError(NULL, 0);
    char* error = new char[errorSize];
    getError(error, errorSize);
    std::cout << "Failed to initialize " << error << std::endl;
    delete[] error;
    return 1;
  }

  std::cout << "Initialized" << std::endl;

  // Create environment
  void* env;
  if (createEnvironment(argc, argv, &env) != 0) {
    size_t errorSize = getError(NULL, 0);
    char* error = new char[errorSize];
    getError(error, errorSize);
    std::cout << "Failed to create environment " << error << std::endl;
    delete[] error;
    return 1;
  }

  std::cout << "Environment created" << std::endl;

  // Execute script
  const char* script = R"(
      console.log('Hello from Node.js!');
      // print process args
      process.argv.forEach((val, index) => {
        console.log(`${index}: ${val}`);
      });

      // read file from disk
      const fs = require('fs');
      const v = fs.readFileSync('C:/projects/skymp1/skyrim-platform/src/skyrim_nodejs_plugin/test/main.cpp', 'utf8');
      console.log(v);
    )";
  // script = "1";
  if (executeScript(env, script) != 0) {
    size_t errorSize = getError(NULL, 0);
    char* error = new char[errorSize];
    getError(error, errorSize);
    std::cout << "Failed to execute script " << error << std::endl;
    delete[] error;
    return 1;
  }

  std::cout << "Script executed" << std::endl;

  // Destroy environment
  if (destroyEnvironment(env) != 0) {
    size_t errorSize = getError(NULL, 0);
    char* error = new char[errorSize];
    getError(error, errorSize);
    std::cout << "Failed to destroy environment " << error << std::endl;
    delete[] error;
    return 1;
  }

  std::cout << "Environment destroyed" << std::endl;

  FreeLibrary(handle);

  return 0;
}
