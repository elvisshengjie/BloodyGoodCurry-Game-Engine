/*********************************************************************************************
 \file      main.cpp
 \par       SofaSpuds
 \author    elvisshengjie.lim (elvisshengjie.lim@digipen.edu) - Primary Author, 100%

 \brief     Entry point for generated game projects.
 \details   Resolves the active project root, loads the project window configuration,
            wires the generated lifecycle callbacks into Core, and starts the main loop.

 \copyright
            All content © 2025 DigiPen Institute of Technology Singapore.
            All rights reserved.
*********************************************************************************************/

#include "Config/WindowConfig.h"
#include "Core/Core.hpp"
#include "Core/PathUtils.h"
#include "Core/ProjectContext.h"
#include "Game.hpp"

#include <filesystem>

#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

/*************************************************************************************
 \brief  Launch the generated game project executable.
 \return Process exit code.
*************************************************************************************/
int main()
{
#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    if (auto exeDir = Framework::GetExecutableDir(); !exeDir.empty())
    {
        std::error_code ec;
        std::filesystem::current_path(exeDir, ec);
    }

    Framework::InitializeProjectFromExecutableLayout();

    WindowConfig cfg = LoadWindowConfig(Framework::ResolveDataPath("window.json").string());
    if (cfg.width <= 0) cfg.width = 1280;
    if (cfg.height <= 0) cfg.height = 720;
    if (cfg.title.empty()) cfg.title = "__SOFASPUDS_PROJECT_NAME__";

    Core core(cfg.width, cfg.height, cfg.title.c_str(), cfg.fullscreen);
    core.SetCallbacks(mygame::init, mygame::update, mygame::draw, mygame::shutdown);
    core.SetSuspendCallback(mygame::onAppFocusChanged);
    core.Run();
    return 0;
}
