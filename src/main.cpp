#include "stdafx.hpp"
#include "System/Rendering/Descriptors.hpp"
#include "System/Rendering/Pipeline.hpp"
#include "System/Scene/Object.hpp"
#include "Graphics/Rendering/Renderer.hpp"

const Vertex VERTICES[] = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}},
};

const Index INDICES[] = {0, 1, 2, 2, 3, 0};

int main()
{
    std::vector<Vertex> vertices;
    std::vector<Index> indices = {0, 1, 2, 2, 3, 0};

    vertices.emplace_back(Vertex{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}});
    vertices.emplace_back(Vertex{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}});
    vertices.emplace_back(Vertex{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}});
    vertices.emplace_back(Vertex{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}});

    zh::Window window(846, 484, "Azha!");
    zh::Device device(window);
    zh::Swapchain swapchain(device, window);
    zh::Pipeline pipeline(device, swapchain, "Assets/Shaders/vert.spv", "Assets/Shaders/frag.spv");

    VkDescriptorPoolSize pool_size{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, zh::Swapchain::MAX_FRAMES_IN_FLIGHT};
    zh::DescriptorPool global_descriptor_pool(device, zh::Swapchain::MAX_FRAMES_IN_FLIGHT, 0, {pool_size});

    zh::Object object(device);
    object.loadModelFromData(vertices, indices);

    zh::Renderer renderer(device, window);

    while (!window.shouldClose())
    {
        window.pollEvents();
    }

    return 0;
}
