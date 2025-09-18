//Input.cpp
// File: MyEngine/Input/Input.cpp
#include <Windows.h>
#include "Input.h"
#include <iostream>
#include <string>

namespace input
{
    bool IsDown(int vk)
    {
        // High-order bit set means the key is currently down.
        std::cout << 123 << std::endl;
        return (GetAsyncKeyState(vk) & 0x8000) != 0;
    }
}
