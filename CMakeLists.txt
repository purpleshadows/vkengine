cmake_minimum_required(VERSION 3.23)
project(VkTutorial LANGUAGES CXX)

# --- Tooling / language ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies ---
find_package(Vulkan REQUIRED)         # Vulkan::Vulkan
find_package(glfw3 CONFIG REQUIRED)   # glfw

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

# Public headers (IDE/install friendly)
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

# Expose "Include/" as the include root
target_include_directories(core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    $<INSTALL_INTERFACE:include>
)

# If core's headers reference Vulkan headers/types, make this PUBLIC; else PRIVATE
target_link_libraries(core PUBLIC Vulkan::Vulkan)

target_compile_features(core PUBLIC cxx_std_20)

# Warnings (optional)
if (MSVC)
  target_compile_options(core PRIVATE /W4 /permissive-)
else()
  target_compile_options(core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# If your core headers rely on VULKAN_HPP_NO_CONSTRUCTORS, make it PUBLIC here.
target_compile_definitions(core PUBLIC VULKAN_HPP_NO_CONSTRUCTORS)

# ---- Executable ----
add_executable(VkTutorial
  Main.cpp
)

# The exe inherits core's PUBLIC include dirs automatically
target_link_libraries(VkTutorial PRIVATE core glfw Vulkan::Vulkan)

# App-only compile defs
target_compile_definitions(VkTutorial PRIVATE GLFW_NO_API)

if (MSVC)
  target_compile_options(VkTutorial PRIVATE /W4 /permissive-)
else()
  target_compile_options(VkTutorial PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---- Shader compilation (pick ONE approach; this example keeps the loop) ----
set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})
find_program(SLANGC_EXECUTABLE slangc HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin" REQUIRED)

file(GLOB SLANG_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/shaders/*.slang)

set(SLANG_OUTPUTS)
foreach(src ${SLANG_SOURCES})
  get_filename_component(name ${src} NAME_WE)
  set(out ${SHADER_OUT_DIR}/${name}.spv)

  add_custom_command(
    OUTPUT  ${out}
    COMMAND ${SLANGC_EXECUTABLE} ${src}
            -target spirv -profile spirv_1_4
            -emit-spirv-directly -fvk-use-entrypoint-name
            -entry vertMain -entry fragMain
            -o ${out}
    DEPENDS ${src}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "slangc ${src} -> ${out}"
    VERBATIM
  )
  list(APPEND SLANG_OUTPUTS ${out})
endforeach()

add_custom_target(compile_shaders DEPENDS ${SLANG_OUTPUTS})
add_dependencies(VkTutorial compile_shaders)
