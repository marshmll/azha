#include "stdafx.hpp"
#include "System/Pipeline.hpp"

zh::Pipeline::Pipeline(Device &device, Swapchain &swapchain, const std::string &vertex_shader_path,
                       const std::string &fragment_shader_path)
    : device(device), swapchain(swapchain)
{
    createPipeline(vertex_shader_path, fragment_shader_path);
}

void zh::Pipeline::createPipeline(const std::string &vertex_shader_path, const std::string &fragment_shader_path)
{
    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;

    {
        auto vert_shader_bytecode = readFile(vertex_shader_path);
        auto frag_shader_bytecode = readFile(fragment_shader_path);

        vert_shader_module = createShaderModule(vert_shader_bytecode);
        frag_shader_module = createShaderModule(frag_shader_bytecode);
    }

    // Shader States
    VkPipelineShaderStageCreateInfo vert_shader_stage_info{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

    // Dynamic States
    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state_info.pDynamicStates = dynamic_states.data();

    // Vertex Input State
    auto binding_description = Vertex::getBindingDescription();
    auto attribute_descriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_state_info{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input_state_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_state_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_state_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    // Input Assembly State
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;

    // Viewport State (dynamic)
    VkPipelineViewportStateCreateInfo viewport_state_info{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport_state_info.viewportCount = 1;
    viewport_state_info.scissorCount = 1;

    // Rasterization State
    VkPipelineRasterizationStateCreateInfo rasterization_state_info{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
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

    VkPipelineColorBlendStateCreateInfo color_blending_state{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    color_blending_state.logicOpEnable = VK_FALSE;
    color_blending_state.logicOp = VK_LOGIC_OP_COPY;
    color_blending_state.attachmentCount = 1;
    color_blending_state.pAttachments = &color_blending_attachment_state;
    color_blending_state.blendConstants[0] = 0.0f;
    color_blending_state.blendConstants[1] = 0.0f;
    color_blending_state.blendConstants[2] = 0.0f;
    color_blending_state.blendConstants[3] = 0.0f;

    // Pipeline Layout
    VkPipelineLayoutCreateInfo pipeline_layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &descriptorSetLayout;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device.getLogicalDevice(), &pipeline_layout_info, nullptr, &pipelineLayout) !=
        VK_SUCCESS)
        throw std::runtime_error("zh::Pipeline::createPipeline: FAILED TO CREATE PIPELINE LAYOUT");

    // Graphics Pipeline
    VkGraphicsPipelineCreateInfo graphics_pipeline_info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
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
    graphics_pipeline_info.renderPass = swapchain.getRenderPass();
    graphics_pipeline_info.subpass = 0;
    graphics_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    graphics_pipeline_info.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(device.getLogicalDevice(), VK_NULL_HANDLE, 1, &graphics_pipeline_info, nullptr,
                                  &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("zh::Pipeline::createPipeline: FAILED TO CREATE GRAPHICS PIPELINE");
    }

    vkDestroyShaderModule(device.getLogicalDevice(), frag_shader_module, nullptr);
    vkDestroyShaderModule(device.getLogicalDevice(), vert_shader_module, nullptr);
}

const std::vector<uint8_t> zh::Pipeline::readFile(const std::string &path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("zh::Pipeline::readFile: FAILED TO OPEN FILE: " + path);

    size_t f_size = file.tellg();
    file.seekg(0);

    std::vector<uint8_t> f_data(f_size);
    file.read(reinterpret_cast<char *>(f_data.data()), f_size);

    file.close();

    return f_data;
}

VkShaderModule zh::Pipeline::createShaderModule(std::vector<uint8_t> &code)
{
    VkShaderModule shader_module;

    VkShaderModuleCreateInfo shader_module_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    shader_module_info.codeSize = code.size();
    shader_module_info.pCode = reinterpret_cast<uint32_t *>(code.data());

    if (vkCreateShaderModule(device.getLogicalDevice(), &shader_module_info, nullptr, &shader_module) != VK_SUCCESS)
    {
        throw std::runtime_error(
            "zh::Pipeline::createShaderModule: FAILED TO CREATE SHADER MODULE FROM CODE WITH SIZE " +
            std::to_string(code.size()));
    }

    return shader_module;
}
