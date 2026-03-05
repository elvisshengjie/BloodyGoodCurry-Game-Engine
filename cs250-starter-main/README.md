# CS250 Assignments

For the class assignments you will make a series of graphics demo programs that showcases all the topics discussed in the course.

Each topic will have applicable sliders and/or modifiers to play around with the parameters which affect the topic.

The topics will be presented and implemented with OpenGL.

This will be a cross platform targeting Windows & Web platforms as a minimum and possibly the Ubuntu & Mac platforms.

## Quick Start

1. **Setup** your [Development Environment](docs/DevEnvironment.md)
2. **Build** the project:
   ```sh
   python scripts/scan_build_project.py
   ```

## Build Reference

### CMake Presets

CMake presets are predefined build configurations stored in `CMakePresets.json`. We have three build types:

| Preset | Description |
|--------|-------------|
| `debug` | No optimizations, full debug symbols. Best for development and debugging. |
| `developer-release` | Optimized build with `DEVELOPER_VERSION` macro defined. Enables developer debugging tools. |
| `release` | Full optimizations, no debug symbols. For final distribution. |

Each build type is combined with a platform target (e.g., `windows-debug`, `linux-release`, `web-developer-release`).

### Build Script Options

```sh
python scripts/scan_build_project.py                      # Scan and build all configurations
python scripts/scan_build_project.py --skip-todos         # Skip TODO/FIXME scanning
python scripts/scan_build_project.py --skip-opengl        # Skip OpenGL usage scanning
python scripts/scan_build_project.py --skip-build         # Scan only, no building
python scripts/scan_build_project.py --target web         # Build only web target
python scripts/scan_build_project.py --target windows --build-type debug  # Build specific target and type
```

### Manual CMake

```sh
# Windows
cmake --preset windows-debug && cmake --build --preset windows-debug
# Web
cmake --preset web-debug && cmake --build --preset web-debug
# Linux
cmake --preset linux-debug && cmake --build --preset linux-debug
```

### Build Output

| Platform | Location |
|----------|----------|
| Windows | `build/windows-{debug,developer-release,release}/graphics_fun.exe` |
| Linux | `build/linux-{debug,developer-release,release}/graphics_fun` |
| Web | `build/web-{debug,developer-release,release}/graphics_fun.html` |

## Run the Application

```sh
./graphics_fun           # Run last opened or default demo
./graphics_fun hello     # Run a specific demo
./graphics_fun shadow
```

**Web**: Optionally append `?start=<demo>` to the URL run a specific demo:
```
https://example.github.io/graphics_fun.html
https://example.github.io/graphics_fun.html?start=hello
https://example.github.io/graphics_fun.html?start=shadow
```

