#include <iostream>
#include <limits>
#include <vector>
#include <math.h>
#include <algorithm>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>

int main() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(680, 420, "Vulkan 1.3", nullptr, nullptr);

  VkInstance instance;
  std::vector<const char*> instanceLayers;
  instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
  std::vector<const char*> instanceExtensions;
  uint32_t glfwExtensionCount;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  instanceExtensions.insert(instanceExtensions.end(), glfwExtensions, glfwExtensions + glfwExtensionCount);
  instanceExtensions.push_back("VK_KHR_get_surface_capabilities2");
  VkApplicationInfo applicationInfo{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "Application Test",
      .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
      .pEngineName = "Basalt",
      .engineVersion = VK_MAKE_VERSION(0, 1, 0),
      .apiVersion = VK_API_VERSION_1_3,
  };
  VkInstanceCreateInfo instanceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &applicationInfo,
      .enabledLayerCount = static_cast<uint32_t>(instanceLayers.size()),
      .ppEnabledLayerNames = instanceLayers.data(),
      .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
      .ppEnabledExtensionNames = instanceExtensions.data(),
  };
  vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

  uint32_t physicalDeviceCount;
  vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
  std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
  vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
  VkPhysicalDevice physicalDevice = physicalDevices[0];

  uint32_t deviceQueueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &deviceQueueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties2> deviceQueueFamilies(deviceQueueFamilyCount, {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
  vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &deviceQueueFamilyCount, deviceQueueFamilies.data());

  uint32_t graphicsQueueFamily;
  for (uint32_t i = 0; i < deviceQueueFamilyCount; i++) {
    if (deviceQueueFamilies[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      graphicsQueueFamily = i;
    }
  }

  VkQueue graphicsQueue;
  float graphicsQueuePriority = 1.0f;
  VkDeviceQueueCreateInfo graphicsQueueCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = graphicsQueueFamily,
      .queueCount = 1,
      .pQueuePriorities = &graphicsQueuePriority,
  };

  VkDevice device;
  VkPhysicalDeviceExtendedDynamicStateFeaturesEXT deviceExtendedDynamicStateFeatures{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
      .extendedDynamicState = VK_TRUE,
  };
  VkPhysicalDeviceVulkan13Features device13Features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext = &deviceExtendedDynamicStateFeatures,
      .synchronization2 = VK_TRUE,
      .dynamicRendering = VK_TRUE,
  };
  std::vector<const char*> deviceExtension;
  deviceExtension.push_back("VK_KHR_swapchain");
  std::vector<VkDeviceQueueCreateInfo> deviceQueues;
  deviceQueues.push_back(graphicsQueueCreateInfo);
  VkDeviceCreateInfo deviceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &device13Features,
      .queueCreateInfoCount = static_cast<uint32_t>(deviceQueues.size()),
      .pQueueCreateInfos = deviceQueues.data(),
      .enabledExtensionCount = static_cast<uint32_t>(deviceExtension.size()),
      .ppEnabledExtensionNames = deviceExtension.data(),
  };
  vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
  vkGetDeviceQueue(device, graphicsQueueFamily, 0, &graphicsQueue);

  VkSurfaceKHR surface;
  std::cout << glfwCreateWindowSurface(instance, window, nullptr, &surface);

  VkSwapchainKHR swapchain;
  VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
      .surface = surface,
  };
  VkSurfaceCapabilities2KHR surfaceCapabilities{VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};
  vkGetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, &surfaceInfo, &surfaceCapabilities);
  uint32_t minImageCount = surfaceCapabilities.surfaceCapabilities.minImageCount + 1;
  VkFormat surfaceFormat = VK_FORMAT_B8G8R8A8_SRGB;
  VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  VkExtent2D extent = surfaceCapabilities.surfaceCapabilities.currentExtent;
  if (extent.width == std::numeric_limits<uint32_t>::max()) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    extent = VkExtent2D{
        .width = std::clamp<uint32_t>(width, surfaceCapabilities.surfaceCapabilities.minImageExtent.width, surfaceCapabilities.surfaceCapabilities.minImageExtent.width),
        .height = std::clamp<uint32_t>(height, surfaceCapabilities.surfaceCapabilities.minImageExtent.height, surfaceCapabilities.surfaceCapabilities.minImageExtent.height),
    };
  }
  VkSurfaceTransformFlagBitsKHR surfaceTransform = surfaceCapabilities.surfaceCapabilities.currentTransform;
  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
  VkSwapchainCreateInfoKHR swapchainCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = minImageCount,
      .imageFormat = surfaceFormat,
      .imageColorSpace = colorSpace,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &graphicsQueueFamily,
      .preTransform = surfaceTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = presentMode,
      .clipped = VK_TRUE,
      .oldSwapchain = nullptr,
  };
  vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);

  uint32_t swapchainImageCount;
  vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);
  std::vector<VkImage> swapchainImages(swapchainImageCount);
  vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data());

  std::vector<VkImageView> imageViews(swapchainImageCount);
  for (uint32_t i = 0; i < swapchainImageCount; i++) {
    VkComponentMapping componentMapping{
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };
    VkImageSubresourceRange imageSubresourceRange{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    VkImageViewCreateInfo imageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = swapchainImages[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = surfaceFormat,
        .components = componentMapping,
        .subresourceRange = imageSubresourceRange};
    vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageViews[i]);
  }

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  for (auto imageView : imageViews) {
    vkDestroyImageView(device, imageView, nullptr);
  }
  vkDestroySwapchainKHR(device, swapchain, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyDevice(device, nullptr);
  vkDestroyInstance(instance, nullptr);
  glfwDestroyWindow(window);
  glfwTerminate();
}
