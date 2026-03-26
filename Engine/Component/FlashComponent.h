#pragma once

struct FlashComponent
{
    float frequency = 8.0f;
    float duration = 1.0f;
    bool start_visible = true;

    // runtime values
    float timer = 0.0f;
    bool visible = true;
};