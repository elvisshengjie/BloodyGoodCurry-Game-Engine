
Our gameConcept is:
You play as Ming, an aspiring curry rice hawker. After hearing about the No. 1 curry
store owned by Hei Bang, Ming sneaks into Hei Bang’s HDB maisonette to steal the
secret recipe. But the horrifying truth is revealed: the secret ingredient is human
blood!

The game is a top-down hack-and-slash set in a familiar yet sinister Singaporean
environment. Players will fight enemies across multiple rooms, collect keys, and face
the final boss to 

Core Mechanics:

    Navigate a top-down environment

    Hack-and-slash combat against spawning enemies

    Keys are required to unlock the final room (the kitchen)

    Different enemy weaknesses (melee vs ranged attacks)

    Checkpoints when a room is cleared (HP restored to full)

    Lore items reward exploration

Game Loop

    Explore rooms in the HDB maisonette

    Defeat all enemies in a room → door unlocks

    Collect 2 keys (Bathroom + Master Bedroom)

    Unlock Kitchen’s secret room → Final Boss battle

Defeat final boss to complete game uncover the full mystery.

Team Roster
Elvis Lim (Technical lead)

Kong Yimo (programmer/Designer)

Erika Ishii (programmer/Designer)

Choo Jian Wei (Product manager/programmer)

Ho Jun (Design lead/programmer)

Chin Xin Jue (Artist Lead/designer)

Tay Wanxuan (Artist/ Audio Lead)

Movement & Combat

W/A/S/D: Move (intent goes into the rigid body; actual motion handled by physics).
Left Mouse Button: physical attack 
Right Mouse Button: range attack

Gizmos
R- Rotate
T - Translation
S - Scale
Control Z undo
Editor & Viewport

Keys
F10: Toggle Editor panels (dockspace & tools).
F11: Toggle fullscreen/windowed mode.
F1 Performance window
F (Editor camera): Frame current selection.
f9 crash game (debugging)
Delete by pressing on keyboard Delete the selected object
control Z to undo button

Enter: “Start” button equivalent
Used to skip cutscene and also to pause/resume 
P : TO enable FPS
Esc: Pause / Back equivalent
Used to pause during gameplay (when not in editor mode) and also to skip cutscene.

Using the In-Game Editor (real time)
Editor camera controls (stop mode)
Middle Mouse Button (hold + drag): Pan editor camera (only when cursor is inside the viewport)

Mouse Wheel (scroll): Zoom editor camera (zooms around the cursor)
Open the editor: press F10. Panels appear on the right (dockspace). 
Viewport Controls: small floating helper to toggle editor/viewport sizing, and play/stop sim.




Hierarchy Panel

Lists all live objects (Name/ID), supports selection and tooltips. 
Keeps the global selection valid and synchronized with the scene. Right click to and click delete to delete object



Content Browser (Assets)

Browse assets/, see thumbnails for textures, and import/replace files. Drag & drop from OS is supported; textures refresh in-scene. 
Replace texture with confirmation & status messages. 


JSON Editor (Level/Data)

Load and edit JSON files from your data folder; 
includes a selectable file list and dirty/modified state.

properties editor 

select and edit the object components

animation editor 

To edit the spritesheet and change the settings of the object

inspector window

To read the object component values

layer 
To spawn object at certain layer, check visibility of certain layer


Important!

Use the clean.bat file then run the run.bat file for editor Mode (Offline)
Go to build_game 
Use the clean.bat file run run_game.bat for non editor mode in release mode (Offline)

Run game in admin once to able to play 

You can also toggle Editor Enabled (F10) via the on-screen Viewport Controls
Crash logging test:
- Crash logs are written to logs/crash.log with UTC timestamps, the crash reason, 
the thread id, and a sanitized stack trace.
- To demonstrate the rubric, run the game and press F9 to trigger 
the built-in crash drill. The engine raises SIGABRT, which routes through the crash handlers and writes the stack trace before exiting.
- Relaunch the game after the forced crash and 
open logs/crash.log to review the new entry. On Android builds the same record is also mirrored to logcat under the ENGINE/CRASH tag for quick inspection
   use imgui to spawn, despawn object and make changes depend on component

Web Build and Run (Browser)

Install first (one-time setup):
- Visual Studio 2022 with "Desktop development with C++" workload
- CMake (add to PATH)
- Ninja (add to PATH)
- Python 3 (add to PATH)
- EMSDK (Emscripten SDK)

Quick checks:
- cmake --version
- ninja --version
- python --version
- em++ --version

Recommended install commands (Windows + winget):
- winget install Kitware.CMake
- winget install Ninja-build.Ninja
- winget install Python.Python.3.12

EMSDK setup example:
1) git clone https://github.com/emscripten-core/emsdk.git %USERPROFILE%\emsdk
2) cd %USERPROFILE%\emsdk
3) emsdk install latest
4) emsdk activate latest
5) emsdk_env.bat

Note:
- build_web_html.bat auto-loads %USERPROFILE%\emsdk\emsdk_env.bat if EMSDK is not already set.

1) Build web version:
   build_web_html.bat release-split reconfigure

2) Host output folder with a local server:
   python -m http.server 8000 -d build\web-release-split\Sandbox

3) Open in browser:
   http://localhost:8000/BloodyGoodCurry.html?v=1

4) After every rebuild, change v= number and hard refresh:
   Ctrl+Shift+R

Web notes:
- Right-click is used for ranged attack in game. Browser context menu is disabled in the web shell.
- Do not open the html file directly from disk. Always run through http.server.
- Web builds currently run with audio disabled (FMOD stubs), so browser version has no in-game sound.
   
