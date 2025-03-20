#include "stdafx.hpp"
#include "System/Rendering/Descriptors.hpp"

zh::DescriptorSetLayout::Builder::Builder(Device &device) : device(device)
{
}

zh::DescriptorSetLayout::Builder &zh::DescriptorSetLayout::Builder::addBinding(const uint32_t binding,
                                                                               VkDescriptorType descriptor_type,
                                                                               VkShaderStageFlags stage_flags,
                                                                               const uint32_t count)
{
    assert(bindings.count(binding) == 0 && "zh::DescriptorSetLayout::Builder::addBinding: BINDING ALREADY IN USE");
    VkDescriptorSetLayoutBinding layout_binding{};
    layout_binding.binding = binding;
    layout_binding.descriptorType = descriptor_type;
    layout_binding.descriptorCount = count;
    layout_binding.stageFlags = stage_flags;
    bindings[binding] = layout_binding;

    return *this;
}

std::unique_ptr<zh::DescriptorSetLayout> zh::DescriptorSetLayout::Builder::build() const
{
    return std::make_unique<DescriptorSetLayout>(device, bindings);
}

zh::DescriptorSetLayout::DescriptorSetLayout(Device &device,
                                             const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> &bindings)
    : device(device), bindings(bindings)
{
    std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings;

    for (auto &[_, binding] : bindings)
        set_layout_bindings.push_back(binding);

    VkDescriptorSetLayoutCreateInfo descriptor_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    descriptor_info.bindingCount = static_cast<uint32_t>(bindings.size());
    descriptor_info.pBindings = set_layout_bindings.data();

    if (vkCreateDescriptorSetLayout(device.getLogicalDevice(), &descriptor_info, nullptr, &descriptorSetLayout) !=
        VK_SUCCESS)
        throw std::runtime_error(
            "zh::DescriptorSetLayout::DescriptorSetLayout: FAILED TO CREATE DESCRIPTOR SET LAYOUT");
}

zh::DescriptorSetLayout::~DescriptorSetLayout()
{
    vkDestroyDescriptorSetLayout(device.getLogicalDevice(), descriptorSetLayout, nullptr);
}

VkDescriptorSetLayout zh::DescriptorSetLayout::getDescriptorSetLayout() const
{
    return descriptorSetLayout;
}

const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> &zh::DescriptorSetLayout::getBindings() const
{
    return bindings;
}

VkDescriptorSetLayoutBinding &zh::DescriptorSetLayout::getBinding(const uint32_t binding)
{
    return bindings.at(binding);
}

zh::DescriptorPool::DescriptorPool(Device &device, const uint32_t max_sets, VkDescriptorPoolCreateFlags pool_flags,
                                   const std::vector<VkDescriptorPoolSize> &pool_sizes)
    : device(device)
{
    VkDescriptorPoolCreateInfo descriptor_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descriptor_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    descriptor_info.pPoolSizes = pool_sizes.data();
    descriptor_info.maxSets = max_sets;
    descriptor_info.flags = pool_flags;

    if (vkCreateDescriptorPool(device.getLogicalDevice(), &descriptor_info, nullptr, &descriptorPool) != VK_SUCCESS)
        throw std::runtime_error("zh::DescriptorPool::DescriptorPool: FAILED TO CREATE DESCRIPTOR POOL");
}

zh::DescriptorPool::~DescriptorPool()
{
    vkDestroyDescriptorPool(device.getLogicalDevice(), descriptorPool, nullptr);
}

const bool zh::DescriptorPool::allocateDescriptor(const VkDescriptorSetLayout descriptor_set_layout,
                                                  VkDescriptorSet &descriptor) const
{
    VkDescriptorSetAllocateInfo alloc_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.descriptorPool = descriptorPool;
    alloc_info.pSetLayouts = &descriptor_set_layout;
    alloc_info.descriptorSetCount = 1;

    if (vkAllocateDescriptorSets(device.getLogicalDevice(), &alloc_info, &descriptor) != VK_SUCCESS)
    {
        std::cerr << "zh::DescriptorPool::allocateDescriptor: FAILED TO ALLOCATE DESCRIPTOR SET" << std::endl;
        return false;
    }

    return true;
}

void zh::DescriptorPool::freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const
{
    vkFreeDescriptorSets(device.getLogicalDevice(), descriptorPool, static_cast<uint32_t>(descriptors.size()),
                         descriptors.data());
}

void zh::DescriptorPool::resetPool()
{
    vkResetDescriptorPool(device.getLogicalDevice(), descriptorPool, 0);
}

zh::DescriptorWriter::DescriptorWriter(Device &device, DescriptorSetLayout &descriptor_set_layout,
                                       DescriptorPool &descriptor_pool)
    : device(device), descriptorSetLayout(descriptor_set_layout), descriptorPool(descriptor_pool)
{
}

zh::DescriptorWriter &zh::DescriptorWriter::writeBuffer(const uint32_t binding, VkDescriptorBufferInfo *buffer_info)
{

    assert(descriptorSetLayout.getBindings().count(binding) == 1 &&
           "zh::DescriptorWriter::writeBuffer: LAYOUT DOES NOT CONTAIN SPECIFIED BINDING");

    auto &binding_description = descriptorSetLayout.getBinding(binding);

    assert(binding_description.descriptorCount == 1 &&
           "zh::DescriptorWriter::writeBuffer: BINDING SINGLE DESCRIPTOR INFO, BUT BINDING EXPECTS MULTIPLE");

    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.descriptorType = binding_description.descriptorType;
    write.dstBinding = binding;
    write.pBufferInfo = buffer_info;
    write.descriptorCount = 1;

    writes.push_back(write);
    return *this;
}

zh::DescriptorWriter &zh::DescriptorWriter::writeImage(const uint32_t binding, VkDescriptorImageInfo *image_info)
{
    assert(descriptorSetLayout.getBindings().count(binding) == 1 &&
           "zh::DescriptorWriter::writeBuffer: LAYOUT DOES NOT CONTAIN SPECIFIED BINDING");

    auto &binding_descriptor = descriptorSetLayout.getBinding(binding);

    assert(binding_descriptor.descriptorCount == 1 &&
           "zh::DescriptorWriter::writeBuffer: BINDING SINGLE DESCRIPTOR INFO, BUT BINDING EXPECTS MULTIPLE");

    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.descriptorType = binding_descriptor.descriptorType;
    write.dstBinding = binding;
    write.pImageInfo = image_info;
    write.descriptorCount = 1;

    writes.push_back(write);
    return *this;
}

const bool zh::DescriptorWriter::build(VkDescriptorSet &descriptor_set)
{
    if (!descriptorPool.allocateDescriptor(descriptorSetLayout.getDescriptorSetLayout(), descriptor_set))
        return false;

    overwrite(descriptor_set);
    return true;
}

void zh::DescriptorWriter::overwrite(VkDescriptorSet &descriptor_set)
{
    for (auto &write : writes)
        write.dstSet = descriptor_set;

    vkUpdateDescriptorSets(device.getLogicalDevice(), writes.size(), writes.data(), 0, nullptr);
}
