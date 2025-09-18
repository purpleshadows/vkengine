#include "Include/Core/Renderer.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main() {
  Core::Renderer renderer;

  try {
    renderer.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;

}
