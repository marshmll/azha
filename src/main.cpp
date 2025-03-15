#include "stdafx.hpp"
#include "System/Vulkan.hpp"

int main()
{
    zh::Vulkan vulkan;

    zh::Window window = vulkan.createWindow(846, 484, "Vulkan");

    do
    {
        window.pollEvents();
    } while (!window.shouldClose());


    return 0;
}
