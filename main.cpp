#include <iostream>
#include <limits>
#include <vector>
#include <math.h>
#include <algorithm>
#include <fstream>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

struct vertex {
  glm::vec3 pos, color;
  static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions;
    bindingDescriptions.push_back(VkVertexInputBindingDescription{
        .binding = 0,
        .stride = sizeof(vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    });
    return bindingDescriptions;
  }
  static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    attributeDescriptions.push_back(VkVertexInputAttributeDescription{
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = 0,
    });
    attributeDescriptions.push_back(VkVertexInputAttributeDescription{
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = sizeof(pos),
    });
    return attributeDescriptions;
  }
};

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

  VkPipeline pipeline;
  std::ifstream shaderFile("../shaders/shader.spv", std::ios::ate | std::ios::binary);
  std::vector<char> shaderCode(shaderFile.tellg());
  shaderFile.seekg(0, std::ios::beg);
  shaderFile.read(shaderCode.data(), static_cast<std::streamsize>(shaderCode.size()));
  shaderFile.close();
  VkShaderModule shader;
  VkShaderModuleCreateInfo shaderModuleCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = static_cast<uint32_t>(shaderCode.size()) * sizeof(char),
      .pCode = reinterpret_cast<const uint32_t*>(shaderCode.data()),
  };
  vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shader);
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  shaderStages.push_back(VkPipelineShaderStageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = shader,
      .pName = "vertMain",
  });
  shaderStages.push_back(VkPipelineShaderStageCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = shader,
      .pName = "fragMain",
  });
  std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = vertex::GetAttributeDescriptions();
  std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions = vertex::GetBindingDescriptions();
  VkPipelineVertexInputStateCreateInfo vertexInputState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size()),
      .pVertexBindingDescriptions = vertexBindingDescriptions.data(),
      .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size()),
      .pVertexAttributeDescriptions = vertexAttributeDescriptions.data(),
  };
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
  };
  VkViewport viewport{
      .x = 0,
      .y = 0,
      .width = static_cast<float>(extent.width),
      .height = static_cast<float>(extent.height),
  };
  VkRect2D scissor{
      .offset = VkOffset2D{.x = 0, .y = 0},
      .extent = extent,
  };
  VkPipelineViewportStateCreateInfo viewportState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor,
  };
  VkPipelineRasterizationStateCreateInfo rasterizationState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_FRONT_AND_BACK,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f,
  };
  VkPipelineMultisampleStateCreateInfo multisampleState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
  };
  VkPipelineColorBlendStateCreateInfo colorBlendState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
  };
  VkPipelineLayout pipelineLayout;
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0,
      .pushConstantRangeCount = 0,
  };
  vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
  VkGraphicsPipelineCreateInfo pipelineCreateInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = static_cast<uint32_t>(shaderStages.size()),
      .pStages = shaderStages.data(),
      .pVertexInputState = &vertexInputState,
      .pInputAssemblyState = &inputAssemblyState,
      .pViewportState = &viewportState,
      .pRasterizationState = &rasterizationState,
      .pMultisampleState = &multisampleState,
      .pColorBlendState = &colorBlendState,
      .layout = pipelineLayout,
  };
  vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyShaderModule(device, shader, nullptr);
  vkDestroyPipeline(device, pipeline, nullptr);
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
