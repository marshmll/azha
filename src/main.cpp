#include "stdafx.hpp"
#include "System/Vulkan.hpp"

int main()
{
    zh::Vulkan vulkan;

    zh::Window window = vulkan.createWindow(846, 484, "Vulkan");

    while (!window.shouldClose())
    {
        window.pollEvents();

        vulkan.drawFrameTemp();
    }

    return 0;
}
