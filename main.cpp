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
  std::vector<const char*> instanceExtensions;
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

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  vkDestroyInstance(instance, nullptr);
  glfwDestroyWindow(window);
  glfwTerminate();
}
