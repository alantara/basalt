#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

int main() {
  glfwInit();
  GLFWwindow* window = glfwCreateWindow(680, 420, "Vulkan 1.3", nullptr, nullptr);

  VkInstance instance;
  std::vector<const char*> instanceLayers;
  instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
  std::vector<const char*> instanceExtensions;
  uint32_t glfwExtensionCount;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  instanceExtensions.insert(instanceExtensions.end(), glfwExtensions, glfwExtensions + glfwExtensionCount);
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

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  vkDestroyDevice(device, nullptr);
  vkDestroyInstance(instance, nullptr);
  glfwDestroyWindow(window);
  glfwTerminate();
}
