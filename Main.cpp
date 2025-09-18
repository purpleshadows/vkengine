#include <Core/Renderer.h>
#include <glslang/Public/ShaderLang.h>
#include <cstdlib>
#include <iostream>

void InitGlslang() { glslang::InitializeProcess(); }
void ShutdownGlslang(){ glslang::FinalizeProcess(); }

int main() {

  Core::Renderer r;

  try {
    r.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;

}
