#include "../Include/Core/Swapchain.h"

Core::Swapchain::Swapchain(Device &device, vk::raii::SurfaceKHR &surface,
                     GLFWwindow &window)
    : window_(window), device_(device), surface_(surface) {
  createSwapchain();
  createImageViews();
}

void Core::Swapchain::createSwapchain() {
  auto physicalDevice = device_.vkPhysicalDevice();
  auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface_);
  swapChainExtent = chooseSwapExtent(surfaceCapabilities);
  swapChainSurfaceFormat =
      chooseSwapSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(*surface_));
  vk::SwapchainCreateInfoKHR swapChainCreateInfo{
      .surface = *surface_,
      .minImageCount = chooseSwapMinImageCount(surfaceCapabilities),
      .imageFormat = swapChainSurfaceFormat.format,
      .imageColorSpace = swapChainSurfaceFormat.colorSpace,
      .imageExtent = swapChainExtent,
      .imageArrayLayers = 1,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
      .imageSharingMode = vk::SharingMode::eExclusive,
      .preTransform = surfaceCapabilities.currentTransform,
      .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
      .presentMode = chooseSwapPresentMode(
          physicalDevice.getSurfacePresentModesKHR(*surface_)),
      .clipped = true};

  swapChain = vk::raii::SwapchainKHR(device_.vkDevice(), swapChainCreateInfo);
  swapChainImages = swapChain.getImages();
}

vk::Extent2D Core::Swapchain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width != 0xFFFFFFFF) {
    return capabilities.currentExtent;
  }
  int width, height;
  glfwGetFramebufferSize(&window_, &width, &height);

  return {std::clamp<uint32_t>(width, capabilities.minImageExtent.width,
                               capabilities.maxImageExtent.width),
          std::clamp<uint32_t>(height, capabilities.minImageExtent.height,
                               capabilities.maxImageExtent.height)};
}

void Core::Swapchain::createImageViews() {
  assert(swapChainImageViews.empty());

  vk::ImageViewCreateInfo imageViewCreateInfo{
      .viewType = vk::ImageViewType::e2D,
      .format = swapChainSurfaceFormat.format,
      .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
  for (auto image : swapChainImages) {
    imageViewCreateInfo.image = image;
    swapChainImageViews.emplace_back(device_.vkDevice(), imageViewCreateInfo);
  }
}
