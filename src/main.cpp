#include "stdafx.hpp"
#include "Graphics/Window.hpp"
#include "System/AzhaCore.hpp"

int main()
{
    zh::AzhaCore core;

    zh::Window window(846, 484, "Vulkan!");

    do
    {
        window.pollEvents();

    }
    while (!window.shouldClose());

    return 0;
}
