# Environment Setup

## Windows OS Setup

Install the following tools:

1. [**Visual Studio**](https://visualstudio.microsoft.com/downloads/)
   - Select C++-related packages during installation.
2. [**CMake**](https://cmake.org/download/)
   - Ensure it is available on the command line (`PATH`) if prompted.
3. [**Git SCM**](https://git-scm.com/downloads)
   - Ensure it is available on the command line (`PATH`) if prompted.
4. [**Ninja Build**](https://github.com/ninja-build/ninja/releases)
   - Manually add it to the command line (`PATH`).

## Web / Emscripten Setup

### Important Notice

**Do not use the Windows version of Emscripten for production.** Due to line-ending differences, generated output files may not work properly when uploaded to a web server. Instead, use Emscripten on a Linux-based OS such as macOS or Ubuntu (_[WSL](https://learn.microsoft.com/en-us/windows/wsl/install) works too_).

### Installation Steps

1. Review [Emscripten Platform-Specific Notes](https://emscripten.org/docs/getting_started/downloads.html#platform-notes-installation-instructions-sdk) for any prerequisites, such as Python 3.
2. Open a terminal and run:

```sh
    # Got to Home Directory
    cd ~

   # Clone the emsdk repository
   git clone https://github.com/emscripten-core/emsdk.git

   # Navigate to the directory
   cd emsdk

   # Fetch the latest version of the SDK (if needed)
   git pull

   # Install the SDK tools
   ./emsdk install 5.0.0

   # Activate this version for the current user
   ./emsdk activate 5.0.0
```

## Ubuntu / Debian OS Setup

### Install Required Tools

You need **GNU development tools** such as `make`, `git`, and other dependencies:

```sh
sudo apt install build-essential git cmake ninja-build libsdl2-dev libglew-dev
```

## macOS Setup

### Install Xcode

Xcode is the integrated development environment (IDE) for macOS and includes necessary C++ tools.

1. Open the **App Store** and search for "Xcode" to install it.
2. Launch Xcode after installation to complete the setup.
3. Open **Terminal** (Applications > Utilities or use Spotlight Search).
4. Run the following command to install Command Line Tools:

   ```sh
   xcode-select --install
   ```
5. Follow the on-screen instructions.

### Install Development Tools & Libraries

1. **Install Homebrew** (if not already installed):
   - Visit [Homebrew's website](https://brew.sh) and follow the installation instructions.

2. **Install CMake, SDL2, and GLEW using Homebrew**:
   ```sh
   brew install cmake ninja sdl2 glew
   ```

This will install the required development tools and dependencies automatically.

---

Your environment should now be set up and ready for development!
