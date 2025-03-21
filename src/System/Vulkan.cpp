#include "stdafx.hpp"
#include "System/Vulkan.hpp"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PUBLIC METHODS //////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

zh::Vulkan::Vulkan()
    : physicalDevice(VK_NULL_HANDLE), window(nullptr), currentFrame(0), framebufferResized(false), cleaned(false)
{
    initValidationLayers();
    initDeviceExtensions();
    initGLFW();
    initVulkanInstance();
    initDebugMessenger();
}

zh::Vulkan::~Vulkan()
{
    cleanup();
}

// zh::Window zh::Vulkan::createWindow(const unsigned int width, const unsigned int height, const std::string &title)
// {
//     if (window != nullptr)
//         throw std::runtime_error("Failed to create window. Only one window instance is allowed.");

//     glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

//     window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
//     glfwSetWindowUserPointer(window, this);
//     glfwSetFramebufferSizeCallback(window, framebufferResizedCallback);

//     if (glfwCreateWindowSurface(vkInstance, window, nullptr, &surface))
//         throw std::runtime_error("Failed to create window surface.");

//     pickPhysicalDevice();
//     createLogicalDevice();
//     initMemoryAllocator();
//     createSwapchain();
//     createImageViews();
//     createRenderPass();
//     createDescriptorSetLayout();
//     createGraphicsPipeline();
//     createFramebuffers();
//     createCommandPools();
//     createVertexBuffer();
//     createIndexBuffer();
//     createUniformBuffers();
//     createDescriptorPool();
//     createDescriptorSets();
//     createCommandBuffers();
//     createSyncObjects();

//     return Window(window, surface);
// }

void zh::Vulkan::drawFrameTemp()
{
    // Wait for frame to finish
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // Acquire an image from the swapchain
    uint32_t image_index = 0;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
                                            VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to acquire a swapchain image.");
    }

    // Only reset the fence if work is being submitted.
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    // Record command buffer
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], image_index);

    updateUniformBuffer(currentFrame);

    // Submit command buffer
    VkSemaphore wait_semaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkSemaphore signal_semaphores[] = {renderFinishedSemaphores[currentFrame]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &commandBuffers[currentFrame];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submit_info, inFlightFences[currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command buffer.");

    VkSwapchainKHR swapchains[] = {swapchain};
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;

    result = vkQueuePresentKHR(presentQueue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
    {
        setFramebufferResized(false);
        recreateSwapchain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present swapchain image.");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void zh::Vulkan::setFramebufferResized(const bool resized)
{
    framebufferResized = resized;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PUBLIC STATIC METHODS ///////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

const uint32_t zh::Vulkan::getExtensionCount()
{
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

    return extension_count;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PRIVATE METHODS /////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void zh::Vulkan::initValidationLayers()
{
#ifdef NDEBUG
    enableValidationLayers = false;
#else
    enableValidationLayers = true;
#endif

    validationLayers = {"VK_LAYER_KHRONOS_validation"};
}

void zh::Vulkan::initDeviceExtensions()
{
    deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME};
}

void zh::Vulkan::initGLFW()
{
    glfwInit();
}

void zh::Vulkan::initVulkanInstance()
{
    // Check Validation Layers
    if (enableValidationLayers && !checkValidationLayerSupport())
        throw std::runtime_error("Some validation layers were requested but are not available.");

    // Aplication info
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Azha alpha v0.0.1";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.pEngineName = "Azha";
    app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.apiVersion = VK_API_VERSION_1_2;

    // Create info
    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    // Global Extensions
    auto glfw_extensions = getRequiredExtensions();

#ifdef __APPLE__
    // Avoid VK_ERROR_INCOMPATIBLE_DRIVER
    glfw_extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    create_info.enabledExtensionCount = static_cast<uint32_t>(glfw_extensions.size());
    create_info.ppEnabledExtensionNames = glfw_extensions.data();

    // Validation Layers
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
    if (enableValidationLayers)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        create_info.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debug_create_info);
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debug_create_info;
    }
    else
    {
        create_info.enabledLayerCount = 0;
        create_info.pNext = nullptr;
    }

    if (vkCreateInstance(&create_info, nullptr, &vkInstance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Vulkan Instance.");
}

void zh::Vulkan::initDebugMessenger()
{
    if (!enableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT create_info;
    populateDebugMessengerCreateInfo(create_info);

    if (createDebugUtilsMessengerEXT(vkInstance, &create_info, nullptr, &debugMessenger) != VK_SUCCESS)
        throw std::runtime_error("Failed to initialize debug messenger.");
}

void zh::Vulkan::pickPhysicalDevice()
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(vkInstance, &device_count, nullptr);

    if (device_count == 0)
        throw std::runtime_error("No GPU's with Vulkan support found.");

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(vkInstance, &device_count, devices.data());

    std::multimap<int, VkPhysicalDevice> candidate_devices;

    for (auto &device : devices)
    {
        int score = rateDeviceSuitability(device);
        candidate_devices.insert(std::make_pair(score, device));
    }

    if (candidate_devices.rbegin()->first >= 0)
        physicalDevice = candidate_devices.rbegin()->second;
    else
        throw std::runtime_error("Failed to find suitable GPU(s)");
}

void zh::Vulkan::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = {indices.getGraphicsFamily(), indices.getPresentFamily()};

    float queue_priority = 1.f;

    for (auto &queue_family : unique_queue_families)
    {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;

        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features{};

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    create_info.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        create_info.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        create_info.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &create_info, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a logical device from the current physical device.");

    // Retrieve Device Queues
    vkGetDeviceQueue(device, indices.getGraphicsFamily(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.getPresentFamily(), 0, &presentQueue);
}

void zh::Vulkan::initMemoryAllocator()
{
    VmaVulkanFunctions vulkan_functions{};
    vulkan_functions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkan_functions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocator_create_info{};
    allocator_create_info.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_2;
    allocator_create_info.physicalDevice = physicalDevice;
    allocator_create_info.device = device;
    allocator_create_info.instance = vkInstance;
    allocator_create_info.pVulkanFunctions = &vulkan_functions;

    if (vmaCreateAllocator(&allocator_create_info, &allocator) != VK_SUCCESS)
        throw std::runtime_error("Failed to initialize Vulkan Memory Allocator.");
}

void zh::Vulkan::createSwapchain()
{
    SwapchainSupportDetails swap_chain_support = querySwapchainSupport(physicalDevice);
    VkSurfaceFormatKHR format = chooseSwapSurfaceFormat(swap_chain_support.formats);
    VkPresentModeKHR mode = chooseSwapPresentMode(swap_chain_support.presentModes);
    VkExtent2D extent = chooseSwapExtent(swap_chain_support.capabilities);
    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;

    if (swap_chain_support.capabilities.maxImageCount > 0 &&
        image_count > swap_chain_support.capabilities.maxImageCount)
        image_count = swap_chain_support.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = format.format;
    create_info.imageColorSpace = format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queue_family_indices[] = {indices.getGraphicsFamily(), indices.getPresentFamily()};

    if (indices.graphicsFamily != indices.presentFamily)
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a swapchain.");
    }

    vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
    swapchainImages.resize(image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchainImages.data());

    swapchainImageFormat = format.format;
    swapchainExtent = extent;
}

void zh::Vulkan::createImageViews()
{
    swapchainImageViews.resize(swapchainImages.size());

    for (int i = 0; i < swapchainImages.size(); ++i)
    {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swapchainImages[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swapchainImageFormat;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &create_info, nullptr, &swapchainImageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create an image view.");
        }
    }
}

void zh::Vulkan::createRenderPass()
{
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swapchainImageFormat;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_reference{};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &render_pass_info, nullptr, &renderPass) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Render Pass.");
}

void zh::Vulkan::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = 1;
    create_info.pBindings = &ubo_layout_binding;

    if (vkCreateDescriptorSetLayout(device, &create_info, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout.");
}

void zh::Vulkan::createGraphicsPipeline()
{
    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;

    {
        auto vert_shader_code = readFile("Assets/Shaders/vert.spv");
        auto frag_shader_code = readFile("Assets/Shaders/frag.spv");

        vert_shader_module = createShaderModule(vert_shader_code);
        frag_shader_module = createShaderModule(frag_shader_code);
    }

    // Shader States
    VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

    // Dynamic States
    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info{};
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state_info.pDynamicStates = dynamic_states.data();

    // Vertex Input State
    auto binding_description = Vertex::getBindingDescription();
    auto attribute_descriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_state_info{};
    vertex_input_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_state_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_state_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    // Input Assembly State
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state{};
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;

    // Viewport State (dynamic)
    VkPipelineViewportStateCreateInfo viewport_state_info{};
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.scissorCount = 1;

    // Rasterization State
    VkPipelineRasterizationStateCreateInfo rasterization_state_info{};
    rasterization_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_info.depthClampEnable = VK_FALSE;
    rasterization_state_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_info.lineWidth = 1.f;
    rasterization_state_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state_info.depthBiasEnable = VK_FALSE;
    rasterization_state_info.depthBiasConstantFactor = 0.f;
    rasterization_state_info.depthBiasClamp = 0.f;
    rasterization_state_info.depthBiasSlopeFactor = 0.f;

    // Multisample State
    VkPipelineMultisampleStateCreateInfo multisample_state_info{};
    multisample_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_info.sampleShadingEnable = VK_FALSE;
    multisample_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_info.minSampleShading = 1.f;
    multisample_state_info.pSampleMask = nullptr;
    multisample_state_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_info.alphaToOneEnable = VK_FALSE;

    // TODO: Depth and Stencil State

    // Color Blending State
    VkPipelineColorBlendAttachmentState color_blending_attachment_state{};
    color_blending_attachment_state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blending_attachment_state.blendEnable = VK_FALSE;
    color_blending_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blending_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blending_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blending_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blending_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blending_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending_state{};
    color_blending_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_state.logicOpEnable = VK_FALSE;
    color_blending_state.logicOp = VK_LOGIC_OP_COPY;
    color_blending_state.attachmentCount = 1;
    color_blending_state.pAttachments = &color_blending_attachment_state;
    color_blending_state.blendConstants[0] = 0.0f;
    color_blending_state.blendConstants[1] = 0.0f;
    color_blending_state.blendConstants[2] = 0.0f;
    color_blending_state.blendConstants[3] = 0.0f;

    // Pipeline Layout
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &descriptorSetLayout;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Pipeline Layout.");
    }

    // Graphics Pipeline
    VkGraphicsPipelineCreateInfo graphics_pipeline_info{};
    graphics_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_info.stageCount = 2;
    graphics_pipeline_info.pStages = shader_stages;
    graphics_pipeline_info.pVertexInputState = &vertex_input_state_info;
    graphics_pipeline_info.pInputAssemblyState = &input_assembly_state;
    graphics_pipeline_info.pViewportState = &viewport_state_info;
    graphics_pipeline_info.pRasterizationState = &rasterization_state_info;
    graphics_pipeline_info.pMultisampleState = &multisample_state_info;
    graphics_pipeline_info.pDepthStencilState = nullptr;
    graphics_pipeline_info.pColorBlendState = &color_blending_state;
    graphics_pipeline_info.pDynamicState = &dynamic_state_info;
    graphics_pipeline_info.layout = pipelineLayout;
    graphics_pipeline_info.renderPass = renderPass;
    graphics_pipeline_info.subpass = 0;
    graphics_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    graphics_pipeline_info.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_pipeline_info, nullptr, &graphicsPipeline) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Graphics Pipeline.");
    }

    vkDestroyShaderModule(device, frag_shader_module, nullptr);
    vkDestroyShaderModule(device, vert_shader_module, nullptr);
}

void zh::Vulkan::createFramebuffers()
{
    swapchainFramebuffers.resize(swapchainImageViews.size());

    for (size_t i = 0; i < swapchainImageViews.size(); ++i)
    {
        VkImageView attachments[] = {swapchainImageViews[i]};

        VkFramebufferCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = renderPass;
        create_info.attachmentCount = 1;
        create_info.pAttachments = attachments;
        create_info.width = swapchainExtent.width;
        create_info.height = swapchainExtent.height;
        create_info.layers = 1;

        if (vkCreateFramebuffer(device, &create_info, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create a Framebuffer.");
    }
}

void zh::Vulkan::createCommandPools()
{
    QueueFamilyIndices queue_family_indices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo command_pool_info{};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = queue_family_indices.getGraphicsFamily();

    if (vkCreateCommandPool(device, &command_pool_info, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create the Command Pool.");

    VkCommandPoolCreateInfo transient_command_pool_info{};
    transient_command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    transient_command_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    transient_command_pool_info.queueFamilyIndex = queue_family_indices.getGraphicsFamily();

    if (vkCreateCommandPool(device, &transient_command_pool_info, nullptr, &transientCommandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a transient Command Pool.");
}

void zh::Vulkan::createVertexBuffer()
{
    VkDeviceSize buffer_size = sizeof(vertices);

    VkBuffer staging_buffer;
    VmaAllocation staging_buffer_memory;

    createStagingBuffer(buffer_size, staging_buffer, staging_buffer_memory);

    void *data;

    vmaMapMemory(allocator, staging_buffer_memory, &data);
    std::memcpy(data, vertices, static_cast<size_t>(buffer_size));
    vmaUnmapMemory(allocator, staging_buffer_memory);

    createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                 VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT, vertexBuffer, vertexBufferMemory);

    copyBuffer(staging_buffer, vertexBuffer, buffer_size);

    vmaDestroyBuffer(allocator, staging_buffer, staging_buffer_memory);
}

void zh::Vulkan::createIndexBuffer()
{
    VkDeviceSize buffer_size = sizeof(indices);

    VkBuffer staging_buffer;
    VmaAllocation staging_buffer_memory;

    createStagingBuffer(buffer_size, staging_buffer, staging_buffer_memory);

    void *data;

    vmaMapMemory(allocator, staging_buffer_memory, &data);
    std::memcpy(data, indices, static_cast<size_t>(buffer_size));
    vmaUnmapMemory(allocator, staging_buffer_memory);

    createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                 VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT, indexBuffer, indexBufferMemory);

    copyBuffer(staging_buffer, indexBuffer, buffer_size);

    vmaDestroyBuffer(allocator, staging_buffer, staging_buffer_memory);
}

void zh::Vulkan::createUniformBuffers()
{
    VkDeviceSize buffer_size = sizeof(UniformBufferObject);

    uniformBuffers.resize(buffer_size);
    uniformBuffersMemory.resize(buffer_size);
    uniformBuffersMapped.resize(buffer_size);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        createBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                         VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
                     uniformBuffers[i], uniformBuffersMemory[i]);

        vmaMapMemory(allocator, uniformBuffersMemory[i], &uniformBuffersMapped[i]);
    }
}

void zh::Vulkan::createDescriptorPool()
{
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.poolSizeCount = 1;
    create_info.pPoolSizes = &pool_size;
    create_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    create_info.flags = 0;

    if (vkCreateDescriptorPool(device, &create_info, nullptr, &descriptorPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool.");
}

void zh::Vulkan::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptorPool;
    alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    alloc_info.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &alloc_info, descriptorSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor sets.");

    for (size_t i = 0; i < descriptorSets.size(); ++i)
    {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = uniformBuffers[i];
        buffer_info.offset = 0;
        buffer_info.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptor_write{};
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet = descriptorSets[i];
        descriptor_write.dstBinding = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo = &buffer_info;
        descriptor_write.pImageInfo = nullptr;
        descriptor_write.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
    }
}

void zh::Vulkan::createCommandBuffers()
{
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = commandPool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device, &allocate_info, commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate Command Buffers.");
}

void zh::Vulkan::createSyncObjects()
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (vkCreateSemaphore(device, &semaphore_info, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphore_info, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fence_info, nullptr, &inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create sync objects for a frame.");
        }
    }
}

void zh::Vulkan::recreateSwapchain()
{
    int width = 0, height = 0;

    glfwGetFramebufferSize(window, &width, &height);

    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    cleanupSwapchain();

    createSwapchain();
    createImageViews();
    createFramebuffers();
}

void zh::Vulkan::updateUniformBuffer(const uint32_t current_image)
{
    static auto start_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f),
                                static_cast<float>(swapchainExtent.width) / static_cast<float>(swapchainExtent.height),
                                0.1f, 10.f);
    ubo.proj[1][1] *= -1;
    std::memcpy(uniformBuffersMapped[current_image], &ubo, sizeof(UniformBufferObject));
}

void zh::Vulkan::cleanup()
{
    if (!cleaned)
    {
        if (device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(device);

            cleanupSwapchain();
            cleanupIndexBuffer();
            cleanupVertexBuffer();
            cleanupUniformBuffers();
            cleanupPipeline();
            cleanupDescriptorPool();
            cleanupDescriptorSetLayout();
            cleanupRenderPass();
            cleanupSyncObjects();
            cleanupCommandPools();
            cleanupMemoryAllocator();
            cleanupDevice();
            cleanupDebugUtils();
            cleanupSurface();
            cleanupWindow();
        }

        cleanupVulkanInstance();
        glfwTerminate();

        cleaned = true;
    }
}

void zh::Vulkan::cleanupSwapchain()
{
    for (auto &framebuffer : swapchainFramebuffers)
        vkDestroyFramebuffer(device, framebuffer, nullptr);

    for (auto &view : swapchainImageViews)
        vkDestroyImageView(device, view, nullptr);

    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void zh::Vulkan::cleanupIndexBuffer()
{
    vmaDestroyBuffer(allocator, indexBuffer, indexBufferMemory);
}

void zh::Vulkan::cleanupVertexBuffer()
{
    vmaDestroyBuffer(allocator, vertexBuffer, vertexBufferMemory);
}

void zh::Vulkan::cleanupUniformBuffers()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vmaUnmapMemory(allocator, uniformBuffersMemory[i]);
        vmaDestroyBuffer(allocator, uniformBuffers[i], uniformBuffersMemory[i]);
    }
}

void zh::Vulkan::cleanupDescriptorPool()
{
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void zh::Vulkan::cleanupDescriptorSetLayout()
{
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}

void zh::Vulkan::cleanupPipeline()
{
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

void zh::Vulkan::cleanupRenderPass()
{
    vkDestroyRenderPass(device, renderPass, nullptr);
}

void zh::Vulkan::cleanupSyncObjects()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }
}

void zh::Vulkan::cleanupCommandPools()
{
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyCommandPool(device, transientCommandPool, nullptr);
}

void zh::Vulkan::cleanupMemoryAllocator()
{
    vmaDestroyAllocator(allocator);
}

void zh::Vulkan::cleanupDevice()
{
    vkDestroyDevice(device, nullptr);
}

void zh::Vulkan::cleanupDebugUtils()
{
    if (enableValidationLayers)
        destroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);
}

void zh::Vulkan::cleanupSurface()
{
    vkDestroySurfaceKHR(vkInstance, surface, nullptr);
}

void zh::Vulkan::cleanupWindow()
{
    glfwDestroyWindow(window);
}

void zh::Vulkan::cleanupVulkanInstance()
{
    vkDestroyInstance(vkInstance, nullptr);
}

const int zh::Vulkan::rateDeviceSuitability(VkPhysicalDevice device) const
{
    int score = 0;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);
    bool extension_support = checkDeviceExtensionSupport(device);

    // Prefer discrete GPU over integrated GPU.
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }
    else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        score += 200;
    }

    // Max possible texture size to max graphics quality.
    score += properties.limits.maxImageDimension2D;

    // Require geometry shader.
    if (!features.geometryShader)
        score = -1;

    // Require a device with a valid queue familiy.
    if (!findQueueFamilies(device).isComplete())
        score = -1;

    // Require that swap chain is adequate
    if (extension_support)
    {
        SwapchainSupportDetails details = querySwapchainSupport(device);
        if (details.formats.empty() || details.presentModes.empty())
            score = -1;
    }

    // Require a device with extensions support.
    if (!extension_support)
        score = -1;

    return score;
}

const zh::QueueFamilyIndices zh::Vulkan::findQueueFamilies(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    for (int i = 0; i < queue_families.size(); ++i)
    {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

        if (present_support)
            indices.presentFamily = i;

        if (indices.isComplete())
            break;
    }

    return indices;
}

const zh::SwapchainSupportDetails zh::Vulkan::querySwapchainSupport(VkPhysicalDevice device) const
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

    if (format_count != 0)
    {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
    }

    uint32_t present_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_modes_count, nullptr);

    if (present_modes_count != 0)
    {
        details.presentModes.resize(format_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_modes_count, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR zh::Vulkan::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available_formats)
{
    for (auto const &format : available_formats)
    {
        if (format.format == VK_FORMAT_B8G8R8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return available_formats[0];
}

VkPresentModeKHR zh::Vulkan::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &available_modes)
{
    for (auto const &mode : available_modes)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D zh::Vulkan::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    int width, height;

    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    actual_extent.width =
        std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actual_extent.height =
        std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actual_extent;
}

const bool zh::Vulkan::checkDeviceExtensionSupport(VkPhysicalDevice device) const
{
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::set<std::string> required_extensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension : available_extensions)
        required_extensions.erase(extension.extensionName);

    return required_extensions.empty();
}

void zh::Vulkan::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &create_info)
{
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    create_info.pfnUserCallback = zh::Vulkan::debugCallback;
    create_info.pUserData = nullptr;
}

const bool zh::Vulkan::checkValidationLayerSupport()
{
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (const char *layer_name : validationLayers)
    {
        bool layer_found = false;

        for (const auto &layer_properties : available_layers)
        {
            if (strcmp(layer_name, layer_properties.layerName) == 0)
            {
                layer_found = true;
                break;
            }
        }

        if (!layer_found)
            return false;
    }

    return true;
}

VkShaderModule zh::Vulkan::createShaderModule(std::vector<char> &code)
{
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<uint32_t *>(code.data());

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a shader module.");

    return shader_module;
}

std::vector<const char *> zh::Vulkan::getRequiredExtensions()
{
    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions;

    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    if (enableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    return extensions;
}

void zh::Vulkan::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                              VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags, VkBuffer &buffer,
                              VmaAllocation &buffer_memory)
{
    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = memory_usage;
    alloc_info.flags = allocation_flags;
    alloc_info.requiredFlags = properties;

    vmaCreateBuffer(allocator, &create_info, &alloc_info, &buffer, &buffer_memory, nullptr);
}

void zh::Vulkan::createStagingBuffer(VkDeviceSize buffer_size, VkBuffer &buffer, VmaAllocation &buffer_memory)
{
    createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
                 buffer, buffer_memory);
}

void zh::Vulkan::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = transientCommandPool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    VkBufferCopy copy_region{};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy_region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(graphicsQueue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, transientCommandPool, 1, &command_buffer);
}

void zh::Vulkan::recordCommandBuffer(VkCommandBuffer command_buffer, const uint32_t image_index)
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording a Command Buffer.");

    VkClearValue clear_color = {{{0.f, 0.f, 0.f, 1.f}}};
    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = renderPass;
    render_pass_info.framebuffer = swapchainFramebuffers[image_index];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = swapchainExtent;
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(swapchainExtent.width);
    viewport.height = static_cast<float>(swapchainExtent.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};

    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(command_buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                            &descriptorSets[currentFrame], 0, nullptr);

    vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(sizeof(indices) / sizeof(uint32_t)), 1, 0, 0, 0);

    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record Command Buffer.");
}

std::vector<char> zh::Vulkan::readFile(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + filename);

    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    return buffer;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PRIVATE STATIC METHODS //////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

VKAPI_ATTR VkBool32 VKAPI_CALL zh::Vulkan::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                         VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                         const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                         void *pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        std::cout << "[ VL DIAL ] " << pCallbackData->pMessage << std::endl;

    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        std::cout << "[ VL INFO ] " << pCallbackData->pMessage << std::endl;

    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        std::cout << "[ VL WARN ] " << pCallbackData->pMessage << std::endl;

    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        std::cerr << "[ VL ERROR ] " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VkResult zh::Vulkan::createDebugUtilsMessengerEXT(VkInstance instance,
                                                  const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator,
                                                  VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (fn != nullptr)
        return fn(instance, pCreateInfo, pAllocator, pDebugMessenger);

    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void zh::Vulkan::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                               const VkAllocationCallbacks *pAllocator)
{
    auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (fn != nullptr)
        fn(instance, debugMessenger, pAllocator);
}

void zh::Vulkan::framebufferResizedCallback(GLFWwindow *window, int width, int height)
{
    auto app = reinterpret_cast<zh::Vulkan *>(glfwGetWindowUserPointer(window));
    app->setFramebufferResized(true);
}
