//Input.cpp
// File: MyEngine/Input/Input.cpp
#include <Windows.h>
#include "Input.h"

namespace input
{
    bool IsDown(int vk)
    {
        // High-order bit set means the key is currently down.
        return (GetAsyncKeyState(vk) & 0x8000) != 0;
    }
}
