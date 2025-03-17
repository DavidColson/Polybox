# Polybox

Polybox is an in-development PS1 era fantasy console, a bit like Pico-8, but with simple 3D as well as 2D.

![polybox render](ReadmeImage.png)

It's still rather primitive and early in development, mostly it's just a dedicated PS1 renderer right now with an old fashioned fix function pipeline. You render use it like this:

```lua
function update()
    set_clear_color(0.25, 0.25, 0.25, 1.0)

	matrix_mode("Projection")
    identity()
    perspective(320, 240, 1, 20, 60)

    matrix_mode("View")
    identity()
    translate(-1.5, -1.5, -2)

	begin_object_3d("Triangles")
        color(1.0, 0.0, 0.0, 1.0)
        vertex(1.0, 1.0, 0.0)

        color(0, 1, 0, 1)
        vertex(1, 2, 0)

        color(0, 0, 1, 1)
        vertex(2.0, 2.0, 0.0)
    end_object_3d()
end
```

The above gives you the following output:

![polybox render](ReadmeImage2.png)

# Current features

- PS1 era fixed-function 3D rendering
- Basic flat and smooth shaded lighting
- Gltf model loading
- 2D sprite drawing
- Depth cueing fog
- 15 bit color space with dithering
- Write your games with Luau, with hot reloading
- Picotron style userdata system for handling raw data efficiently in Luau
- Universal data serialization system, stored in text, binary or compressed binary
- A virtual filesystem which sandboxes games
- Input system that emulates a PS1 like controller

# Roadmap

Required for initial alpha release of the console for use by others:
- Memory mapping userdata
- New renderer that more closely matches API of PS1, using memory mapped GPU state
- Color lookup table 4 and 8 bit textures
- System font rasterizer
- Simple render targets
- Limited support for user pixel shaders similar to Love2D
- Audio system
- Developer mode (for editing/making projects etc)
- Documentation

Post alpha release:
- Game exports
- CPU and GPU throughput limits
- Project explorer tool
- Code editor & Debugger
- Image Editor
- Sound effect editor
- WebGPU support

# Supported platforms

Currently only Windows 64 bit. I intend for it to support Web, Linux and MacOS at some point though.

# Documentation

Also too early for this, but the codebase is extremely simple and hopefully self documenting. The full API exposed to Luau is listed in the file cpu.cpp, so this is the best reference for now

# Compiling

The project is relatively simple to compile, all you need is cmake and the MSVC build tools, and then just run the build.bat file in the root of the repo, which will compile everything you need.
