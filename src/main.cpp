#include "stdafx.hpp"
#include "System/Pipeline.hpp"
#include "System/Model.hpp"

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
    zh::Model model(device, vertices, indices);

    while (!window.shouldClose())
    {
        window.pollEvents();
    }

    return 0;
}
