#include "../Engine/Core/Core.hpp"
#include "Game.hpp"
#include "Config/WindowConfig.h"

int main() {
   
    WindowConfig cfg = LoadWindowConfig("../../Data_Files/window.json");

    Core core(cfg.width, cfg.height, cfg.title.c_str());
    core.SetCallbacks(mygame::init, mygame::update, mygame::draw, mygame::shutdown);
    core.Run();
    return 0;
}
