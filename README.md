# A CMake Template For SDL3 Projects
A template that automatically compiles shaders into their respective binaries based on the target OS.

## Structure
The template structure looks like this:
```text
.
├── cmake
│   └── shader_config.h.in
├── include
│   ├── context.h
│   ├── geometry.h
│   └── shaders.h
├── src
│   ├── main.c
│   └── shaders.c
├── shaders
│   ├── color.frag.hlsl
│   └── position_color.vert.hlsl
├── README.md
├── CMakeLists.txt
├── CMakePresets.json
├── .gitignore
└── vcpkg.json
```

All the source files should be in `src/` and all headers in `include/`. The `cmake/` directory contains the shader config file which defines which backends were build. In shader source files are in the `shaders/` directory.

## Hello triangle
This template displays the triangle by default. It is based on [https://github.com/mohiji/gpu-by-example](https://github.com/mohiji/gpu-by-example).

## How to use
To use this just clone the repo, change the `MAIN_TARGET` in `CMakeLists.txt` and the `name` field in `vcpkg.json`.

### Dependencies
You will also need:
- `cmake` - of course
-  `vcpkg` and `$VCPKG_ROOT` environment variable pointing to the directory where vcpkg is installed. I recommend cloning the vcpkg repo, for example dnf installed just the binaries and not the cmake script needed.
- `ninja-build perl-FindBin perl-open autoconf autoconf-archive automake libtool` installed

#### On Linux:
I needed to install the SDL backend system dev libraries.

On Fedora it was:
```bash
sudo dnf install libx11-devel libxft-devel libxext-devel libwayland-devel libxkbcommon-devel libegl1-mesa-devel libibus-1.0-devel
```

### Build
For both Windows and Linux there are two configurations: debug and release.

Configure and build:
```bash
cmake --preset {platform}-{debug|release}
cmake --build --preset build-{platform}-{debug|release}
```

Then run it using:
```bash
./out/build/{platform}-{debug|release}/{MAIN_TARGET}
```

Or `{MAIN_TARGET}.exe` on Windows.