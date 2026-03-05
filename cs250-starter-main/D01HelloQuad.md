# CS250 HW - Hello Quad

## Overview
In this assignment, you will implement a setup the foundational rendering features to display and animate a quad using OpenGL. You will also document your work by creating a portfolio page showcasing your project.

[Preview Demo](https://rudy-castan-digipen-teaching.github.io/computer-graphics-demos/cs250s24.kr/D01HelloQuad/graphics_fun.html?start=hello)

## Assignment Steps

### 1. Set Up the Project
- Look for all `TODO` comments in the starter code, GLSL code, and CMake scripts. Complete them, then **remove the TODO comments after implementation**.

### 2. Customize the Window
- Modify `Title.hpp` to set the **window title to have your name**.

### 3. Implement Required Classes
- Implement the following classes:
  1. `IndexBuffer`
  2. `Texture`
  3. `VertexArray`
  4. `VertexBuffer`
- **Use OpenGL function wrappers from `GL.hpp/.cpp` instead of calling OpenGL functions directly** to help automate error checking.
- Ensure compatibility with **OpenGL 3.3**, **OpenGL ES 3.0**, and **WebGL 2**.

### 4. Add Animation Logic
- Modify `D01HelloQuad` to include **custom animation logic**:
  - Add animations to **both** the vertex and fragment shaders.
  - Update the demo logic to control the animations.
  - Animations may be based on **time, frame count, mouse position, etc.**
  - Try out the **asset reloading feature** by modifying shader code or textures while the executable is running.

### 5. Create a Custom Texture
- Update `paint_me.png` with a **drawing of your own making**.

### 6. Create a Portfolio Page
Your portfolio page should include:
- **Project Overview**: A brief description of the project, its purpose, and objectives.
- **Tasks Completed**: A detailed breakdown of your contributions and technologies used.
- **Reflection**: A discussion of challenges faced, solutions implemented, and lessons learned.
- **Live Demo**: Embed a working `web-release` build in an `iframe`.

## Submission Requirements
- **Submit a URL**: Provide a link to your portfolio page showcasing this assignment.
- **Submit a Repository Snapshot**:
  - Download a ZIP archive of your project from GitHub and submit it.
  - Ensure all necessary source code, data, and project files are included.
  - **Remove unnecessary files** before submission (e.g., auto-generated `build/` folder from CMake, `.vs/` folders, temporary files, or compiled binaries).

  ![Download Zip From GitHub](dl_code_from_github.png)

## Grading Rubric

### Core Requirements
✅ Window title updated to include your name.  
✅ Utilize OpenGL 3.3 / OpenGL ES 3.0 / OpenGL WebGL 2 Compatible functions  
✅ OpenGL calls utilize the `GL::` wrappers.  
✅ Repository contains all necessary files and is properly structured.  
✅ Project builds and runs successfully on **Windows x64** and **Web**.  
✅ `main` branch is up to date with the completed assignment.  
✅ Any modified or created files _(code, glsl, CMake, etc.)_ include a proper **header comment** with authors, course name, term, DigiPen copyright  
✅ Portfolio page includes a working **interactive** `web-release` version.  

### Additional Requirements
- Portfolio page contains well-structured **Project Overview, Tasks Completed, and Reflection** sections.
- All TODOs in the project **have been fully implemented and removed**.
- Vertex shader includes **animated vertices**.
- Fragment shader includes **animated colors** that cycle between texture colors and quad vertex colors.
- Animations utilize `environment` variables from `window/Environment.hpp`.
- Code compiles **without warnings or errors**.
- Code is formatted using `clang-format`.

### Grading Scale

| Score | Description |
|-------|------------|
| **F** | No work attempted |
| **D** | Work does not meet all Core requirements |
| **C** | Work meets all Core requirements |
| **B** | Work meets all Core + most additional requirements |
| **A** | Work meets all Core + all additional requirements |
| **S** | Work exceeds expectations with high-quality implementation |


