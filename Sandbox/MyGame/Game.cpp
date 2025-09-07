#include "Game.hpp"
#include "Graphics/Window.hpp"

namespace mygame {
void run() {
    gfx::Window win(800, 600, "MyGame");
    win.run();
}
}
