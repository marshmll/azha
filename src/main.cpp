#include "stdafx.hpp"
#include "System/Pipeline.hpp"

int main()
{
    zh::Window window(846, 484, "Azha!");
    zh::Device device(window);
    zh::Swapchain swapchain(device, window);
    zh::Pipeline pipeline(device, swapchain, "Assets/Shaders/vert.spv", "Assets/Shaders/frag.spv");

    while (!window.shouldClose())
    {
        window.pollEvents();
    }

    return 0;
}
