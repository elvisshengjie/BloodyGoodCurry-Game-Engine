#include "../Engine/Core/Core.hpp"
#include "Game.hpp"
#include "Config/WindowConfig.h"
#define _CRTDBG_MAP_ALLOC



int main() {
   

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    WindowConfig cfg = LoadWindowConfig("../../Data_Files/window.json");

    Core core(cfg.width, cfg.height, cfg.title.c_str());
    core.SetCallbacks(mygame::init, mygame::update, mygame::draw, mygame::shutdown);
    core.Run();
    return 0;
}
