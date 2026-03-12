SofaSpuds ReadMe
================

Game Concept
------------
You play as Ming, an aspiring curry rice hawker. After hearing about the No. 1 curry
store owned by Hei Bang, Ming sneaks into Hei Bang's HDB maisonette to steal the
secret recipe. But the horrifying truth is revealed: the secret ingredient is human
blood.

The game is a top-down hack-and-slash set in a familiar yet sinister Singaporean
environment. Players fight enemies across multiple rooms, collect keys, and face the
final boss.

Core Mechanics
--------------
- Navigate a top-down environment.
- Hack-and-slash combat against spawning enemies.
- Keys are required to unlock the final room.
- Different enemy weaknesses.
- Checkpoints restore HP after room clear.
- Lore items reward exploration.

Game Loop
---------
- Explore rooms in the HDB maisonette.
- Defeat all enemies in a room to unlock the door.
- Collect 2 keys.
- Unlock the kitchen secret room.
- Defeat the final boss to complete the game.

Team Roster
-----------
- Elvis Lim (Technical Lead)
- Kong Yimo (Programmer/Designer)
- Erika Ishii (Programmer/Designer)
- Choo Jian Wei (Product Manager/Programmer)
- Ho Jun (Design Lead/Programmer)
- Chin Xin Jue (Artist Lead/Designer)
- Tay Wanxuan (Artist/Audio Lead)

Runtime Controls
----------------
- `W/A/S/D`: Move
- `Left Mouse Button`: Physical attack
- `Right Mouse Button`: Ranged attack
-  F to slow enemy
- `Enter`: Start / skip cutscene / pause-resume in some flows
- `Esc`: Pause / back / skip cutscene
- `P`: Toggle FPS

Editor Controls
---------------
- `F10`: Toggle editor panels
- `F11`: Toggle fullscreen
- `F1`: Performance window
- `F9`: Crash test
- `F`: Frame current selection
- `T / R / S`: Translate / Rotate / Scale gizmos
- `Delete`: Delete selected object
- `Ctrl+Z`: Undo
- `Middle Mouse Drag`: Pan editor camera
- `Mouse Wheel`: Zoom editor camera
- Press p for FPS counter

Editor Panels
-------------
- Hierarchy: lists live objects and supports selection.
- Content Browser: browse and import project assets.
- JSON Editor: edit files in the active project's data folder.
- Properties Editor: edit game-specific properties.
- Inspector: read live component values.
- Animation Editor: edit sprite sheet settings.
- Layer Panel: choose active layer and layer visibility.
- Spawn Panel: engine-owned prefab spawning and level utility panel.

New Game Project Workflow
-------------------------
The editor now supports real project creation instead of only creating empty content folders.

`File -> New Game`
- Creates a new content project under `Games/<ProjectName>`.
- Creates a matching code project under `Sandbox/<ProjectName>`.
- Does not automatically load the new project into the current session.

Project Layout
--------------
Each game project now has two parts:

- Code: `Sandbox/<ProjectName>`
- Content: `Games/<ProjectName>`

Inside `Games/<ProjectName>`:
- `Assets`: art, audio, and other source assets
- `Data`: levels, prefabs, window config, and editor-authored JSON
- `Saves`: runtime save data, settings, checkpoints, or other generated per-project files

Important:
- `New Game` only scaffolds a new project.
- To use the new project's code and content, build and run the matching project target.

Build Commands
--------------
Editor build and run:
- `run.bat BloodyGoodCurry`
- `run.bat NewGame`

Game build and run without editor:
- `run_game.bat BloodyGoodCurry`
- `run_game.bat NewGame`

Web build:
- `build_web_html.bat`
- `build_web_html.bat BloodyGoodCurry`
- `build_web_html.bat NewGame`

If no game name is passed, scripts use the default configured game.

Desktop build folders are now separated per game and per mode:
- `build_BloodyGoodCurry_editor`
- `build_BloodyGoodCurry_game`
- `build_NewGame_editor`
- `build_NewGame_game`

This allows multiple desktop projects to stay configured at the same time without overwriting
the same `build` or `build_game` folder.

Spawn Panel
-----------
The spawn panel is now owned by the engine.

Engine-owned spawn features:
- Prefab selection and search
- Spawn / clear / apply-to-existing
- Level save / refresh / load
- Engine component overrides for:
  - `TransformComponent`
  - `RenderComponent`
  - `CircleRenderComponent`
  - `SpriteComponent`
  - `RigidBodyComponent`

Game-owned spawn extensions:
- A game can register additional UI and apply callbacks for its own components.
- BloodyGoodCurry uses this for enemy/player/gate-specific spawn settings.
- Newly generated projects start with no custom spawn extension by default.

Testing NewGame Spawn Panel
---------------------------
For `NewGame`, prefabs that work immediately are those using engine components plus the
template `GlowComponent`.

Example test prefabs already added:
- `Games/NewGame/Data/Prefabs/TestBox.json`
- `Games/NewGame/Data/Prefabs/TestGlowBox.json`

Using the Editor
----------------
- Open the editor with `F10`.
- Use the Content Browser to manage assets.
- Use the JSON Editor for project data files.
- Use the Spawn Panel to spawn prefabs from `Data/Prefabs`.
- Use the Layer Panel to set the layer newly spawned objects go into.

Crash Logging Test
------------------
- Crash logs are written to `logs/crash.log`.
- Press `F9` to trigger the crash drill.
- Relaunch and inspect the log entry after the forced crash.

Offline Build Notes
-------------------
- Use `clean.bat` before rebuilding if you need a clean local build.
- Use `clean.bat NewGame2` to clean only that project's editor/game build folders and matching web outputs.
- Run the game once as admin if your local machine setup requires it for audio/runtime access.

Web Build and Run
-----------------
Install first:
- Visual Studio 2022 with Desktop development with C++
- CMake
- Ninja
- Python 3
- EMSDK

Quick checks:
- `cmake --version`
- `ninja --version`
- `python --version`
- `em++ --version`

Recommended install commands:
- `winget install Kitware.CMake`
- `winget install Ninja-build.Ninja`
- `winget install Python.Python.3.12`

EMSDK setup example:
1. `git clone https://github.com/emscripten-core/emsdk.git %USERPROFILE%\emsdk`
2. `cd %USERPROFILE%\emsdk`
3. `emsdk install latest`
4. `emsdk activate latest`
5. `emsdk_env.bat`

Default no-argument web flow:
- `build_web_html.bat`
- Builds `BloodyGoodCurry`
- Uses `release-split`
- Forces `reconfigure`
- Starts `python -m http.server 8000 -d build\web-release-split\Sandbox`

Explicit web build:
- `build_web_html.bat BloodyGoodCurry release-split reconfigure`
- `build_web_html.bat NewGame release-split reconfigure`

If you only want to host manually after a build:
- `python -m http.server 8000 -d build\web-release-split\Sandbox`

Open in browser:
- `http://localhost:8000/BloodyGoodCurry.html?v=1`

Web Notes
---------
- Browser context menu is disabled for right-click attack flow.
- Do not open the HTML file directly from disk.
- Always host through a local server.
- Web builds currently run with audio disabled.
