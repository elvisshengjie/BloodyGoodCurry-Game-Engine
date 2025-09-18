#pragma once
//Input.h
// File: MyEngine/Input/Input.h
#pragma once

namespace input
{
    // Returns true while the given virtual-key is held down.
    // vk accepts Windows virtual key codes (e.g., 'Q', 'E', 'Z', 'X', 'R', VK_SHIFT, etc.).
    bool IsDown(int vk);
}
