# Third Party Libraries

## FreeType

- Source: https://www.freetype.org
- Version: 2.11.1

Files stripped down to just `include/`, `src/` except Jamfiles and tools subfolder, and license files. Have chosen FreeType License for this project. Custom premake lua file being used to build.

## SDL2

- Source: https://github.com/libsdl-org/SDL
- Version: 2.0.8

Just includes and binaries

## Luau

- Source: https://github.com/luau-lang/luau
- Version: 0.636

Changed luau VM to use abort rather than exception handling

## Sokol

- Source: https://github.com/floooh/sokol
- Version: Pulled at change b1221d1

Removed everything except sokol_gfx

## Sokol Tools

- Source: https://github.com/floooh/sokol-tools-bin
- Version: Pulled at change 1528521

Removed the fips files and some extraneous text files

## stb

- Source: https://github.com/nothings/stb
- Version: Pulled at change f75e8d1

Just stb_image used here, everything else removed

## lz4

- Source: https://github.com/lz4/lz4/
- Version: v1.10.0

Just the base lz4 files here, everything else removed and a custom cmakelists for it
