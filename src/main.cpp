#include "stdafx.hpp"
#include "System/Vulkan.hpp"
#include "System/Device.hpp"

int main()
{
    zh::Window window(846, 484, "Teste");

    while (!window.shouldClose())
    {
        window.pollEvents();
    }

    return 0;
}
