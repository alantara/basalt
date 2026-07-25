#include <iostream>
#include <limits>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <cstddef>
#include <vulkan/vulkan.h>
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
        .offset = offsetof(vertex, color),
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
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &deviceQueueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> deviceQueueFamilies(deviceQueueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &deviceQueueFamilyCount, deviceQueueFamilies.data());

  uint32_t graphicsQueueFamily = UINT32_MAX;
  for (uint32_t i = 0; i < deviceQueueFamilyCount; i++) {
    if (deviceQueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      graphicsQueueFamily = i;
      break;
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
  deviceExtension.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
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
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
  uint32_t minImageCount = surfaceCapabilities.minImageCount + 1;
  if (surfaceCapabilities.maxImageCount > 0) {
    minImageCount = std::min(minImageCount, surfaceCapabilities.maxImageCount);
  }
  VkFormat surfaceFormat = VK_FORMAT_B8G8R8A8_SRGB;
  VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  VkExtent2D extent = surfaceCapabilities.currentExtent;
  if (extent.width == std::numeric_limits<uint32_t>::max()) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    extent = VkExtent2D{
        .width = std::clamp<uint32_t>(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
        .height = std::clamp<uint32_t>(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height),
    };
  }
  VkSurfaceTransformFlagBitsKHR surfaceTransform = surfaceCapabilities.currentTransform;
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
      .codeSize = shaderCode.size(),
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
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
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
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f,
  };
  VkPipelineMultisampleStateCreateInfo multisampleState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
  };
  VkPipelineColorBlendAttachmentState colorBlendAttachmentState{
      .blendEnable = VK_FALSE,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };
  VkPipelineColorBlendStateCreateInfo colorBlendState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachmentState,
  };
  VkPipelineLayout pipelineLayout;
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0,
      .pushConstantRangeCount = 0,
  };
  vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
  VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &surfaceFormat,
  };
  VkGraphicsPipelineCreateInfo pipelineCreateInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &pipelineRenderingCreateInfo,
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

  VkCommandPool commandPool;
  VkCommandPoolCreateInfo commandPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = graphicsQueueFamily,
  };
  vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);

  VkCommandBuffer commandBuffer;
  VkCommandBufferAllocateInfo commandBufferAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);

  std::vector<vertex> triangleVertices = {
      {.pos = {0.0f, -0.5f, 0.0f}, .color = {1.0f, 0.0f, 0.0f}},
      {.pos = {0.5f, 0.5f, 0.0f}, .color = {0.0f, 1.0f, 0.0f}},
      {.pos = {-0.5f, 0.5f, 0.0f}, .color = {0.0f, 0.0f, 1.0f}},
  };
  VkDeviceSize vertexBufferSize = sizeof(vertex) * triangleVertices.size();

  VkBuffer vertexBuffer;
  VkBufferCreateInfo vertexBufferCreateInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = vertexBufferSize,
      .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  vkCreateBuffer(device, &vertexBufferCreateInfo, nullptr, &vertexBuffer);

  VkMemoryRequirements vertexBufferMemoryRequirements;
  vkGetBufferMemoryRequirements(device, vertexBuffer, &vertexBufferMemoryRequirements);

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
  VkMemoryPropertyFlags vertexBufferMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  uint32_t vertexBufferMemoryTypeIndex;
  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
    if ((vertexBufferMemoryRequirements.memoryTypeBits & (1 << i)) &&
        (memoryProperties.memoryTypes[i].propertyFlags & vertexBufferMemoryFlags) == vertexBufferMemoryFlags) {
      vertexBufferMemoryTypeIndex = i;
      break;
    }
  }

  VkDeviceMemory vertexBufferMemory;
  VkMemoryAllocateInfo vertexBufferMemoryAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = vertexBufferMemoryRequirements.size,
      .memoryTypeIndex = vertexBufferMemoryTypeIndex,
  };
  vkAllocateMemory(device, &vertexBufferMemoryAllocateInfo, nullptr, &vertexBufferMemory);
  vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

  void* vertexData;
  vkMapMemory(device, vertexBufferMemory, 0, vertexBufferSize, 0, &vertexData);
  memcpy(vertexData, triangleVertices.data(), static_cast<size_t>(vertexBufferSize));
  vkUnmapMemory(device, vertexBufferMemory);

  VkSemaphoreCreateInfo semaphoreCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  VkSemaphore imageAvailableSemaphore;
  vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphore);
  std::vector<VkSemaphore> renderFinishedSemaphores(swapchainImageCount);
  for (uint32_t i = 0; i < swapchainImageCount; i++) {
    vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]);
  }

  VkFenceCreateInfo fenceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };
  VkFence inFlightFence;
  vkCreateFence(device, &fenceCreateInfo, nullptr, &inFlightFence);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
    vkResetFences(device, 1, &inFlightFence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandPool(device, commandPool, 0);

    VkCommandBufferBeginInfo commandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

    VkImageSubresourceRange imageSubresourceRange{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    VkImageMemoryBarrier2 toColorAttachmentBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        .srcAccessMask = VK_ACCESS_2_NONE,
        .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchainImages[imageIndex],
        .subresourceRange = imageSubresourceRange,
    };
    VkDependencyInfo toColorAttachmentDependency{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &toColorAttachmentBarrier,
    };
    vkCmdPipelineBarrier2(commandBuffer, &toColorAttachmentDependency);

    VkRect2D renderArea{
        .offset = VkOffset2D{.x = 0, .y = 0},
        .extent = extent,
    };
    VkClearValue clearValue{
        .color = VkClearColorValue{{1.0f, 1.0f, 1.0f, 1.0f}},
    };
    VkRenderingAttachmentInfo colorAttachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = imageViews[imageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clearValue,
    };
    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = renderArea,
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
    };
    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    VkDeviceSize vertexBufferOffset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);
    vkCmdDraw(commandBuffer, static_cast<uint32_t>(triangleVertices.size()), 1, 0, 0);

    vkCmdEndRendering(commandBuffer);

    VkImageMemoryBarrier2 toPresentBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        .dstAccessMask = VK_ACCESS_2_NONE,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchainImages[imageIndex],
        .subresourceRange = imageSubresourceRange,
    };
    VkDependencyInfo toPresentDependency{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &toPresentBarrier,
    };
    vkCmdPipelineBarrier2(commandBuffer, &toPresentDependency);

    vkEndCommandBuffer(commandBuffer);

    VkSemaphoreSubmitInfo waitSemaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = imageAvailableSemaphore,
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    VkCommandBufferSubmitInfo commandBufferSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = commandBuffer,
    };
    VkSemaphoreSubmitInfo signalSemaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = renderFinishedSemaphores[imageIndex],
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    VkSubmitInfo2 submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &waitSemaphoreInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &commandBufferSubmitInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signalSemaphoreInfo,
    };
    vkQueueSubmit2(graphicsQueue, 1, &submitInfo, inFlightFence);

    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinishedSemaphores[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex,
    };
    vkQueuePresentKHR(graphicsQueue, &presentInfo);
  }

  vkDeviceWaitIdle(device);

  vkDestroyFence(device, inFlightFence, nullptr);
  for (auto renderFinishedSemaphore : renderFinishedSemaphores) {
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
  }
  vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
  vkDestroyBuffer(device, vertexBuffer, nullptr);
  vkFreeMemory(device, vertexBufferMemory, nullptr);
  vkDestroyCommandPool(device, commandPool, nullptr);
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
