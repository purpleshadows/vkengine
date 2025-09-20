#pragma once
#include <Core/Device.h>
#include <Core/Shaders/ShaderLoader.h>
#include <Core/Swapchain.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdint> // for uint32_t
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_raii.hpp>

#include <iostream>
const std::vector validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

namespace Core {
    class Renderer {
    public:
        Renderer();           // <-- we’ll define it to build everything
        ~Renderer() = default;
        void run();

    private:
        void initVulkan();
        void initWindow();
        void mainLoop();
        void cleanup();
        void createSurface();
        void createInstance();

        std::vector<const char*> getRequiredExtensions();

        void setupDebugMessenger();

        static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
            vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
            vk::DebugUtilsMessageTypeFlagsEXT type,
            const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
            if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError ||
                severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
                std::cerr << "validation layer: type " << to_string(type)
                    << " msg: " << pCallbackData->pMessage << std::endl;
            }

            return vk::False;
        }

        const uint32_t WIDTH = 800;
        const uint32_t HEIGHT = 600;

        GLFWwindow* window = nullptr;

        vk::raii::Context context{};
        vk::raii::Instance instance{ nullptr };
        vk::raii::SurfaceKHR surface{nullptr};
        vk::raii::DebugUtilsMessengerEXT debugMessenger{ nullptr };

        //std::optional < Shaders::ShaderLoader> shaderLoader;
        Device device;
        std::optional<Swapchain> swapchain;
    };
} // namespace Core
