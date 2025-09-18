#include <Core/Device.h>

#include <cstring>
#include <ranges>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

Core::Device::Device(vk::raii::Instance &instance, vk::raii::SurfaceKHR &surface,
               uint32_t apiVersion)
    : instance_(&instance), surface_(&surface), apiVersion_(apiVersion) {
  pickPhysical();
  createLogical();
}

void Core::Device::pickPhysical() {
  std::vector<vk::raii::PhysicalDevice> devices =
      instance_->enumeratePhysicalDevices();
  const auto devIter = std::ranges::find_if(
      devices, [&](auto const &device) { return isSuitable(device); });
  if (devIter != devices.end()) {
    physicalDevice = *devIter;
  } else {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

bool Core::Device::isSuitable(vk::raii::PhysicalDevice const &dev) const {
  // Check if the device supports the Vulkan 1.3 API version
  bool supportsVulkan1_3 = dev.getProperties().apiVersion >= VK_API_VERSION_1_3;

  // Check if any of the queue families support graphics operations
  auto queueFamilies = dev.getQueueFamilyProperties();
  bool supportsGraphics =
      std::ranges::any_of(queueFamilies, [](auto const &qfp) {
        return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
      });

  // Check if all required device extensions are available
  auto availableDeviceExtensions = dev.enumerateDeviceExtensionProperties();
  bool supportsAllRequiredExtensions = std::ranges::all_of(
      requiredDeviceExtension,
      [&availableDeviceExtensions](auto const &requiredDeviceExtension) {
        return std::ranges::any_of(
            availableDeviceExtensions,
            [requiredDeviceExtension](auto const &availableDeviceExtension) {
              return strcmp(availableDeviceExtension.extensionName,
                            requiredDeviceExtension) == 0;
            });
      });

  auto features = dev.template getFeatures2<
      vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
      vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
  bool supportsRequiredFeatures =
      features.template get<vk::PhysicalDeviceVulkan13Features>()
          .dynamicRendering &&
      features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>()
          .extendedDynamicState;

  return supportsVulkan1_3 && supportsGraphics &&
         supportsAllRequiredExtensions && supportsRequiredFeatures;
}

void Core::Device::createLogical() {
  // find the index of the first queue family that supports graphics
  std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
      physicalDevice.getQueueFamilyProperties();

  // get the first index into queueFamilyProperties which supports graphics
  auto graphicsQueueFamilyProperty =
      std::ranges::find_if(queueFamilyProperties, [](auto const &qfp) {
        return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) !=
               static_cast<vk::QueueFlags>(0);
      });

  auto graphicsIndex = static_cast<uint32_t>(std::distance(
      queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

  // determine a queueFamilyIndex that supports present
  // first check if the graphicsIndex is good enough
  auto presentIndex =
      physicalDevice.getSurfaceSupportKHR(graphicsIndex, *surface_)
          ? graphicsIndex
          : static_cast<uint32_t>(queueFamilyProperties.size());

  if (presentIndex == queueFamilyProperties.size()) {
    // the graphicsIndex doesn't support present -> look for another family
    // index that supports both graphics and present
    for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
      if ((queueFamilyProperties[i].queueFlags &
           vk::QueueFlagBits::eGraphics) &&
          physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i),
                                              *surface_)) {
        graphicsIndex = static_cast<uint32_t>(i);
        presentIndex = graphicsIndex;
        break;
      }
    }

    if (presentIndex == queueFamilyProperties.size()) {
      // there's nothing like a single family index that supports both graphics
      // and present -> look for another family index that supports present
      for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
        if (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i),
                                                *surface_)) {
          presentIndex = static_cast<uint32_t>(i);
          break;
        }
      }
    }
  }

  if ((graphicsIndex == queueFamilyProperties.size()) ||
      (presentIndex == queueFamilyProperties.size())) {
    throw std::runtime_error(
        "Could not find a queue for graphics or present -> terminating");
  }

  // query for Vulkan 1.3 features
  auto features = physicalDevice.getFeatures2();
  vk::PhysicalDeviceVulkan13Features vulkan13Features;
  vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
      extendedDynamicStateFeatures;
  vulkan13Features.dynamicRendering = vk::True;
  extendedDynamicStateFeatures.extendedDynamicState = vk::True;
  vulkan13Features.pNext = &extendedDynamicStateFeatures;
  features.pNext = &vulkan13Features;

  // create a Device
  float queuePriority = 0.0f;

  vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
      .queueFamilyIndex = graphicsIndex,
      .queueCount = 1,
      .pQueuePriorities = &queuePriority};

  vk::DeviceCreateInfo deviceCreateInfo{
      .pNext = &features,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &deviceQueueCreateInfo,
      .enabledExtensionCount =
          static_cast<uint32_t>(requiredDeviceExtension.size()),
      .ppEnabledExtensionNames = requiredDeviceExtension.data()};
  deviceCreateInfo.enabledExtensionCount = requiredDeviceExtension.size();
  deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtension.data();

  device = vk::raii::Device(physicalDevice, deviceCreateInfo);
  q.graphics = vk::raii::Queue(device, graphicsIndex, 0);
  q.present = vk::raii::Queue(device, presentIndex, 0);
}
