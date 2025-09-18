#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

namespace Core {
struct Queues {
  uint32_t graphicsFamily = UINT32_MAX;
  uint32_t presentFamily = UINT32_MAX;
  vk::Queue graphics{};
  vk::Queue present{};
};

class Device {
public:
  Device() = default;
  Device(vk::raii::Instance &instance, vk::raii::SurfaceKHR &surface,
         uint32_t apiVersion = VK_API_VERSION_1_3);

  vk::raii::Device &vkDevice() { return device; }
  vk::raii::PhysicalDevice vkPhysicalDevice() { return physicalDevice; }
  const Queues &queues() const { return q; }
  uint32_t api() const { return apiVersion_; }

private:
  vk::raii::Instance *instance_{};

  vk::raii::SurfaceKHR *surface_ = nullptr;

  vk::raii::PhysicalDevice physicalDevice = nullptr;

  vk::raii::Device device = nullptr;
  Queues q{};

  uint32_t apiVersion_{}; //

  void pickPhysical();
  bool isSuitable(vk::raii::PhysicalDevice const &dev) const;

  void createLogical();

  std::vector<const char *> requiredDeviceExtension = {
      vk::KHRSwapchainExtensionName, vk::KHRSpirv14ExtensionName,
      vk::KHRSynchronization2ExtensionName,
      vk::KHRCreateRenderpass2ExtensionName};
};
}
