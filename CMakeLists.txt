cmake_minimum_required(VERSION 3.23)
project(VkTutorial LANGUAGES CXX)

# ---- Tooling / language ----
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Dependencies from vcpkg ----
find_package(Vulkan REQUIRED)          # Vulkan::Vulkan
find_package(glfw3 CONFIG REQUIRED)    # target: glfw
find_package(glslang CONFIG REQUIRED)  # targets: glslang::glslang, glslang::SPIRV

# ---- Library: core ----
add_library(core)

# Sources
target_sources(core
  PRIVATE
    Core/Backend/Pipeline.cpp
    Core/Device.cpp
    Core/Renderer.cpp
    Core/Swapchain.cpp
    Core/Shaders/ShaderLoader.cpp
)

# Public headers (nice for IDEs / install)
target_sources(core
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS Include
    FILES
      Include/Core/Utils/Hash/Hash.h
      Include/Core/Backend/Pipeline.h
      Include/Core/Device.h
      Include/Core/Renderer.h
      Include/Core/Swapchain.h
      Include/Core/Shaders/ShaderLoader.h
      Include/Core/Shaders/ShaderModule.h
      Include/Core/Shaders/ShaderBlob.h
      Include/Core/Shaders/ShaderCommon.h
      Include/Core/Shaders/ShaderKey.h
)

# Include path for public headers
target_include_directories(core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    $<INSTALL_INTERFACE:include>
)

# Link by imported targets (portable across MSVC/MinGW/Clang)
target_link_libraries(core
  PUBLIC
    Vulkan::Vulkan
    glslang::glslang
    glslang::SPIRV
)

# Some glslang builds expose extra component targets; link them only if present
if (TARGET glslang::MachineIndependent)
  target_link_libraries(core PUBLIC glslang::MachineIndependent)
endif()
if (TARGET glslang::GenericCodeGen)
  target_link_libraries(core PUBLIC glslang::GenericCodeGen)
endif()
if (TARGET glslang::OGLCompiler)
  target_link_libraries(core PUBLIC glslang::OGLCompiler)
endif()
if (TARGET glslang::OSDependent)
  target_link_libraries(core PUBLIC glslang::OSDependent)
endif()

target_compile_features(core PUBLIC cxx_std_20)

# Warnings (optional)
if (MSVC)
  target_compile_options(core PRIVATE /W4 /permissive-)
else()
  target_compile_options(core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# If your headers use this, keep it (optional)
target_compile_definitions(core PUBLIC VULKAN_HPP_NO_CONSTRUCTORS)

# ---- Executable ----
add_executable(VkTutorial Main.cpp)

# Link GLFW target name used by vcpkg is 'glfw'
target_link_libraries(VkTutorial PRIVATE core glfw Vulkan::Vulkan)

# App-only compile defs
target_compile_definitions(VkTutorial PRIVATE GLFW_NO_API)

if (MSVC)
  target_compile_options(VkTutorial PRIVATE /W4 /permissive-)
else()
  target_compile_options(VkTutorial PRIVATE -Wall -Wextra -Wpedantic)
endif()
