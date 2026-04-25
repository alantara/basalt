#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <fstream>
#include <algorithm>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define chk(f, e) if(f != VK_SUCCESS) throw std::runtime_error(e)
#define MAX_FRAMES_IN_FLIGHT 2

static std::vector<char> readFile(const std::string& filename){
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if(!file.is_open()){
    throw std::runtime_error("Failed to open file");
  }
  std::vector<char> buffer(file.tellg());
  file.seekg(0, std::ios::beg);
  file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
  file.close();
  return buffer;
}

int main(){
  GLFWwindow* window;
  VkInstance instance;
  VkSurfaceKHR surface;
  VkPhysicalDevice physicalDevice;
  VkQueue graphicsQueue;
  VkDevice device;
  VkSwapchainKHR swapChain;
  VkPipeline pipeline;
  VkCommandPool commandPool;
  std::vector<VkCommandBuffer> commandBuffers;

  uint32_t graphicsQueueIndex;
  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;

  std::vector<VkSemaphore> presentCompleteSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  //Create window
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // Create instance
  window = glfwCreateWindow(800, 600, "Test Vulkan", nullptr, nullptr);
  VkApplicationInfo applicationInfo{
    .sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName="Test Application",
    .applicationVersion=VK_MAKE_VERSION(1, 0, 0),
    .pEngineName="Tuff",
    .engineVersion=VK_MAKE_VERSION(1, 0, 0),
    .apiVersion=VK_API_VERSION_1_3,
  };
  std::vector<const char*> instanceLayers = {"VK_LAYER_KHRONOS_validation"};
  uint32_t glfwExtensionSize;
  auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionSize);
  std::vector<const char*> instanceExtensions = {std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionSize)};
  VkInstanceCreateInfo instanceCreateInfo{
    .sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo=&applicationInfo,
    .enabledLayerCount=static_cast<uint32_t>(instanceLayers.size()),
    .ppEnabledLayerNames=instanceLayers.data(),
    .enabledExtensionCount=static_cast<uint32_t>(instanceExtensions.size()),
    .ppEnabledExtensionNames=instanceExtensions.data(),
  };
  chk(vkCreateInstance(&instanceCreateInfo, nullptr, &instance), "Failed to initialize instance");
  
  //Create Surface
  chk(glfwCreateWindowSurface(instance, window, nullptr, &surface), "Failed to create surface");
  
  //Get devices
  uint32_t physicalDeviceSize;
  vkEnumeratePhysicalDevices(instance, &physicalDeviceSize, nullptr);
  std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceSize);
  vkEnumeratePhysicalDevices(instance, &physicalDeviceSize, physicalDevices.data());
  uint32_t physicalDeviceIndex = 0;
  physicalDevice = physicalDevices[physicalDeviceIndex];

  //Get physical device families
  uint32_t queueFamiliesCount;
  vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamiliesCount, nullptr);
  std::vector<VkQueueFamilyProperties2> queueFamilies(queueFamiliesCount, VkQueueFamilyProperties2{.sType=VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
  vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamiliesCount, queueFamilies.data());

  for(uint32_t i = 0; i < queueFamilies.size(); i++){
    if(queueFamilies[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT){
      graphicsQueueIndex = i;
      break;
    }
  }

  //Create device
  float queuePriorities = {1.0f};
  VkDeviceQueueCreateInfo graphicsQueueCreateInfo{
    .sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex=graphicsQueueIndex,
    .queueCount=1,
    .pQueuePriorities=&queuePriorities,
  };

  std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
  VkPhysicalDeviceVulkan11Features physicalDeviceVulkan11Features{
    .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
    .shaderDrawParameters=VK_TRUE,
  };
  VkPhysicalDeviceVulkan13Features physicalDeviceVulkan13Features{
    .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    .pNext=&physicalDeviceVulkan11Features,
    .synchronization2=VK_TRUE,
    .dynamicRendering=VK_TRUE,
  };
  VkDeviceCreateInfo deviceCreateInfo{
    .sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext=&physicalDeviceVulkan13Features,
    .queueCreateInfoCount=1,
    .pQueueCreateInfos=&graphicsQueueCreateInfo,
    .enabledExtensionCount=static_cast<uint32_t>(deviceExtensions.size()),
    .ppEnabledExtensionNames=deviceExtensions.data(),
    .pEnabledFeatures=0,
  };
  chk(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device), "Failed to create device");

  //Get graphics queue
  vkGetDeviceQueue(device, graphicsQueueIndex, 0, &graphicsQueue);

  //Get capabilities
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
  VkExtent2D extent{
    std::clamp<uint32_t>(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
    std::clamp<uint32_t>(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height),
  };
  VkFormat format{
    VK_FORMAT_B8G8R8A8_SRGB,
  };

  //Create swapchain
  VkSwapchainCreateInfoKHR swapChainCreateInfo{
    .sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface=surface,
    .minImageCount=surfaceCapabilities.minImageCount,
    .imageFormat=format,
    .imageColorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    .imageExtent=extent,
    .imageArrayLayers=1,
    .imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode=VK_SHARING_MODE_EXCLUSIVE,
    .preTransform=surfaceCapabilities.currentTransform,
    .compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode=VK_PRESENT_MODE_FIFO_KHR,
    .clipped=VK_TRUE,
  };
  chk(vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapChain), "Failed to create swapchain");

  //Get Images
  uint32_t swapChainImageCount;
  vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, nullptr);
  swapChainImages = std::vector<VkImage>(swapChainImageCount);
  vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, swapChainImages.data());

  //Create Image Views
  swapChainImageViews = std::vector<VkImageView>(swapChainImageCount);
  for(uint32_t i = 0; i < swapChainImageCount; i++){
    VkImageViewCreateInfo imageViewCreateInfo{
      .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image=swapChainImages[i],
      .viewType=VK_IMAGE_VIEW_TYPE_2D,
      .format=format,
      .components={
        .r=VK_COMPONENT_SWIZZLE_IDENTITY,
        .g=VK_COMPONENT_SWIZZLE_IDENTITY,
        .b=VK_COMPONENT_SWIZZLE_IDENTITY,
        .a=VK_COMPONENT_SWIZZLE_IDENTITY,
      },
      .subresourceRange={
        .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel=0,
        .levelCount=1,
        .baseArrayLayer=0,
        .layerCount=1,
      },
    };
    chk(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapChainImageViews[i]), "Failed to create image views");
  }

  //Create shader module
  auto shaderCode = readFile("../shaders/slang.spv");

  VkShaderModule shaderModule;
  VkShaderModuleCreateInfo shaderModuleCreateInfo{
    .sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize=shaderCode.size()*sizeof(char),
    .pCode=reinterpret_cast<const uint32_t*>(shaderCode.data()),
  };
  chk(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule), "Failed to create shader module");

  //Create shader pipeline
  VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{
    .sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage=VK_SHADER_STAGE_VERTEX_BIT,
    .module=shaderModule,
    .pName="vertMain",
  };

  VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{
    .sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage=VK_SHADER_STAGE_FRAGMENT_BIT,
    .module=shaderModule,
    .pName="fragMain",
  };

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShaderStageCreateInfo, fragShaderStageCreateInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{
    .sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
  };

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{
    .sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
  };

  std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{
    .sType=VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount=static_cast<uint32_t>(dynamicStates.size()),
    .pDynamicStates=dynamicStates.data(),
  };
  VkPipelineViewportStateCreateInfo viewportStateCreateInfo{
    .sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount=1,
    .scissorCount=1,
  };

  VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{
    .sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable=VK_FALSE,
    .rasterizerDiscardEnable=VK_FALSE,
    .polygonMode=VK_POLYGON_MODE_FILL,
    .cullMode=VK_CULL_MODE_BACK_BIT,
    .frontFace=VK_FRONT_FACE_CLOCKWISE,
    .depthBiasClamp=VK_FALSE,
    .lineWidth=1.0f,
  };

  VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{
    .sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples=VK_SAMPLE_COUNT_1_BIT,
    .sampleShadingEnable=VK_FALSE,
  };

  VkPipelineColorBlendAttachmentState colorBlendAttachmentState{
    .blendEnable=VK_FALSE,
    .colorWriteMask=VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };

  VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{
    .sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable=VK_FALSE,
    .attachmentCount=1,
    .pAttachments=&colorBlendAttachmentState,
  };

  VkPipelineLayout pipelineLayout;
  VkPipelineLayoutCreateInfo pipelineCreateInfo{
    .sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount=0,
    .pushConstantRangeCount=0,
  };
  chk(vkCreatePipelineLayout(device, &pipelineCreateInfo, nullptr, &pipelineLayout), "Failed to create pipeline layout");

  VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{
    .sType=VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .colorAttachmentCount=1,
    .pColorAttachmentFormats=&format,
  };

  VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{
    .sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext=&pipelineRenderingCreateInfo,
    .stageCount=2,
    .pStages=shaderStages.data(),
    .pVertexInputState=&vertexInputStateCreateInfo,
    .pInputAssemblyState=&inputAssemblyStateCreateInfo,
    .pViewportState=&viewportStateCreateInfo,
    .pRasterizationState=&rasterizationStateCreateInfo,
    .pMultisampleState=&multisampleStateCreateInfo,
    .pColorBlendState=&colorBlendStateCreateInfo,
    .pDynamicState=&dynamicStateCreateInfo,
    .layout=pipelineLayout,
  };

  chk(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline), "Failed to create graphics pipeline");

  //Create command pool
  VkCommandPoolCreateInfo commandPoolCreateInfo{
    .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex=graphicsQueueIndex,
  };
  chk(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool), "Failed to create command pool");

  //Allocate command buffer
  commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  VkCommandBufferAllocateInfo commandBufferAllocateInfo{
    .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool=commandPool,
    .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount=MAX_FRAMES_IN_FLIGHT,
  };
  chk(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, commandBuffers.data()), "Failed to allocate command buffer");

  //Creating synchronization
  presentCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  VkSemaphoreCreateInfo presentSemaphoreCreateInfo{
    .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    chk(vkCreateSemaphore(device, &presentSemaphoreCreateInfo, nullptr, &presentCompleteSemaphores[i]), "Failed to create present complete semphore");
  }

  renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  VkSemaphoreCreateInfo renderSemaphoreCreateInfo{
    .sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  for (int i = 0; i < swapChainImageCount; i++) {
    chk(vkCreateSemaphore(device, &renderSemaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]), "Failed to create render complete semphore");
  }

  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  VkFenceCreateInfo inFlightCreateInfo{
    .sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags=VK_FENCE_CREATE_SIGNALED_BIT,
  };
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    chk(vkCreateFence(device, &inFlightCreateInfo, nullptr, &inFlightFences[i]), "Failed to create draw fence");
  }

  uint32_t frameIndex = 0;

  //Game loop
  while(!glfwWindowShouldClose(window)){
    glfwPollEvents();

    chk(vkQueueWaitIdle(graphicsQueue), "Failed in waiting to commands to finish");
    
    //Wait previous frame fence
    vkWaitForFences(device, 1, &inFlightFences[frameIndex], VK_TRUE, UINT32_MAX);
    vkResetFences(device, 1, &inFlightFences[frameIndex]);

    //Get next frame
    uint32_t imageIndex;
    VkAcquireNextImageInfoKHR acquireNextImageInfo{
      .sType=VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
      .swapchain=swapChain,
      .timeout=UINT32_MAX,
      .semaphore=presentCompleteSemaphores[frameIndex],
      .deviceMask=1,
    };
    vkAcquireNextImage2KHR(device, &acquireNextImageInfo, &imageIndex);

    //Begin command buffer
    VkCommandBufferBeginInfo commandBufferBeginInfo{
      .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    chk(vkBeginCommandBuffer(commandBuffers[frameIndex], &commandBufferBeginInfo), "Failed to begin write");

    //Transition to color layout
    VkImageMemoryBarrier2 beginCommandImageMemoryBarrier{
      .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask=VK_ACCESS_2_NONE,
      .dstStageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstAccessMask=VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout=VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
      .image=swapChainImages[imageIndex],
      .subresourceRange={
        .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel=0,
        .levelCount=1,
        .baseArrayLayer=0,
        .layerCount=1,
      },
    };
    VkDependencyInfo beginCommandDependencyInfo{
      .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount=1,
      .pImageMemoryBarriers=&beginCommandImageMemoryBarrier,
    };
    vkCmdPipelineBarrier2(commandBuffers[frameIndex], &beginCommandDependencyInfo);

    //Begin rendering
    VkClearValue clearValue{
      .color={
        .float32={0.0f, 0.0f, 0.0f, 1.0f},
      },
    };
    VkRenderingAttachmentInfo renderingAttachmentInfo{
      .sType=VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView=swapChainImageViews[imageIndex],
      .imageLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp=VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue=clearValue,
    };
    VkRenderingInfo renderingInfo{
      .sType=VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea={
        .offset={0, 0},
        .extent=extent,
      },
      .layerCount=1,
      .colorAttachmentCount=1,
      .pColorAttachments=&renderingAttachmentInfo,
    };
    vkCmdBeginRendering(commandBuffers[frameIndex], &renderingInfo);

    //Drawing commands
    vkCmdBindPipeline(commandBuffers[frameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    ///Dynamic states
    VkViewport viewport{
      .x=0,
      .y=0,
      .width=static_cast<float>(extent.width),
      .height=static_cast<float>(extent.height),
    };
    vkCmdSetViewport(commandBuffers[frameIndex], 0, 1, &viewport);

    VkRect2D scissors{
      .offset={0, 0},
      .extent=extent,
    };
    vkCmdSetScissor(commandBuffers[frameIndex], 0, 1, &scissors);

    //Draw triangle
    vkCmdDraw(commandBuffers[frameIndex], 3, 1, 0, 0);

    //End rendering
    vkCmdEndRendering(commandBuffers[frameIndex]);

    //Transition to present layout
    VkImageMemoryBarrier2 endCommandImageMemoryBarrier{
      .sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask=VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
      .dstStageMask=VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
      .dstAccessMask=VK_ACCESS_2_NONE,
      .oldLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,
      .image=swapChainImages[imageIndex],
      .subresourceRange={
        .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel=0,
        .levelCount=1,
        .baseArrayLayer=0,
        .layerCount=1,
      },
    };
    VkDependencyInfo endCommandDependencyInfo{
      .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount=1,
      .pImageMemoryBarriers=&endCommandImageMemoryBarrier,
    };
    vkCmdPipelineBarrier2(commandBuffers[frameIndex], &endCommandDependencyInfo);

    //End command buffer
    vkEndCommandBuffer(commandBuffers[frameIndex]);

    //Submit queue
    VkSemaphoreSubmitInfo waitSemaphoreSubmitInfo{
      .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore=presentCompleteSemaphores[frameIndex],
      .stageMask=VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    VkCommandBufferSubmitInfo commandBufferSubmitInfo{
      .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
      .commandBuffer=commandBuffers[frameIndex],
    };
    VkSemaphoreSubmitInfo signalSemaphoreSubmitInfo{
      .sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore=renderFinishedSemaphores[imageIndex],
    };
    VkSubmitInfo2 submitInfo{
      .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
      .waitSemaphoreInfoCount=1,
      .pWaitSemaphoreInfos=&waitSemaphoreSubmitInfo,
      .commandBufferInfoCount=1,
      .pCommandBufferInfos=&commandBufferSubmitInfo,
      .signalSemaphoreInfoCount=1,
      .pSignalSemaphoreInfos=&signalSemaphoreSubmitInfo
    };
    chk(vkQueueSubmit2(graphicsQueue, 1, &submitInfo, inFlightFences[frameIndex]), "Failed to submit to queue");
  
    //Present image
    VkPresentInfoKHR presentInfo{
      .sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount=1,
      .pWaitSemaphores=&renderFinishedSemaphores[imageIndex],
      .swapchainCount=1,
      .pSwapchains=&swapChain,
      .pImageIndices=&imageIndex,
    };
    chk(vkQueuePresentKHR(graphicsQueue, &presentInfo), "Failed to present image");

    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  chk(vkQueueWaitIdle(graphicsQueue), "Failed in waiting to commands to finish");

  //Cleanup
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, presentCompleteSemaphores[i], nullptr);
  }
  for (int i = 0; i < swapChainImageCount; i++) {
    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
  }
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyFence(device, inFlightFences[i], nullptr);
  }
  vkDestroyCommandPool(device, commandPool, nullptr);
  vkDestroyPipeline(device, pipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyShaderModule(device, shaderModule, nullptr);
  for(uint32_t i = 0; i < swapChainImageCount; i++){
    vkDestroyImageView(device, swapChainImageViews[i], nullptr);
  }
  vkDestroySwapchainKHR(device, swapChain, nullptr);
  vkDestroyDevice(device, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);
  glfwDestroyWindow(window);
  glfwTerminate();
  return EXIT_SUCCESS;
}
