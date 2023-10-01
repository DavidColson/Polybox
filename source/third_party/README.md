# Third Party Libraries

## bgfx

- Source: https://github.com/bkaradzic/bgfx
- Version: Pulled at change d95a6436

Stripped out examples, build scripts, github files

## bimg

- Source: https://github.com/bkaradzic/bimg
- Version: Pulled at change 8355d36b

Stripped out build scripts and github files

## bx

- Source: https://github.com/bkaradzic/bx
- Version: Pulled at change 51f25ba6

Stripped out tests and catch dependency, all build scripts and github files leaving just the code required to run and the license

## FreeType

- Source: https://www.freetype.org
- Version: 2.11.1

Files stripped down to just `include/`, `src/` except Jamfiles and tools subfolder, and license files. Have chosen FreeType License for this project. Custom premake lua file being used to build.

## ImGui

- Source: https://github.com/ocornut/imgui
- Version: 1.89.9

Removed backends we don't need, added the bgfx backent and then removed the examples folder, but otherwise same as upstream
