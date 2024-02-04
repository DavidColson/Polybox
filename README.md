# Polybox

Polybox is an in-development PS1 era fantasy console, a bit like Pico-8, but with simple 3D as well as 2D.

![polybox render](ReadmeImage.png)

It's still rather primitive and early in development, mostly it's just a dedicated PS1 renderer right now with an old fashioned fix function pipeline. You render objects like this:

```lua
function Update(deltaTime)
    MatrixMode("Projection")
    Identity()
    Perspective(320, 240, 1, 20, 60)

    MatrixMode("View")
    Identity()
    Translate(-1.5, -1.5, -2)

    BeginObject3D("Triangles")
        Color(1.0, 0.0, 0.0, 1.0)
        Vertex(1.0, 1.0, 0.0)

        Color(0, 1, 0, 1)
        Vertex(1, 2, 0)

        Color(0, 0, 1, 1)
        Vertex(2.0, 2.0, 0.0)
    EndObject3D()
end
```

The above gives you the following output:

![polybox render](ReadmeImage2.png)

# Current features

- PS1 era fixed-function 3D rendering
- Special shaders for emulating PS1 era look
- Basic flat and smooth shaded lighting
- Gltf model loading
- 2D sprite drawing
- Depth cueing fog
- Simple 2D shape drawing
- Lua VM for running your game code
- Input system that emulates a PS1 like controller

# Polyscript programming language

The console currently uses Lua for scripting your actual games, but a project is currently underway to develop a custom scripting language, just for Polybox, called Polyscript. It's inspired by Jai, C, Go and a bunch of other languages. This is not really usable yet, but there is much progress and focus on it currently.

The motivations to make a custom language are as follows:

- I want scripting the VM to be similar to older, simpler times where memory is manually managed, and the programmer has more control over what is going on. But with lots of modern improvements in language design for a nicer development experience
- I want static typing, and a generally more strict compiler as I personally think this makes refactoring and working with code easier (than Lua)
- Better, custom integration with Polybox itself, with nicer debugging and development tools, better C++ binding
- A custom VM that can be made to emulate older hardware more easily
- Also lets be honest, it's fun to make a language 

Here's a little sample of what it's like:

```c
// Comments are the same as C++

// It's declaration syntax is inspired by Jai/Go, so one defines variables like so
myVar: float = 25.5;

// It supports type inference, so you can also declare like
myIntVar := 15;

// Assignment is simpler, and we support all the standard arithmetic stuff
myIntVar = 1 - (12 + 5)*(6/3);

// Blocks are used for scope control
{
    // Control flow is fairly typical too
    if myVar > 20 {
        print(true);
    } else if myVar > 10 {
        print(false);
    } else {
        print(1);
    }

    n := 10;
    while n > 0 {
        print(n);
        n = n-1;
    }
}

// Functions use the same declaration syntax as variables, and have a certain signature
add := func (a: i32, b: i32) -> i32 {
    return a + b;
}

// Calling said function is straight forward
print(add(5, 2)); // 7

// Structs also share the same declaration syntax, as you'd expect

MyStruct := struct {
    boolMember: bool;
    floatMember: float;

    // Can initialize struct members too
    intMember: i32 = 2;
}

// Access members like any common c-like language
instances: MyStruct;
print(instance.intMember); // 2

// One of the more exotic features is that types are first class values, so you can do stuff like the following, 
// though this feature is somewhat underdeveloped still

intType: Type = i32;
print(intType); // i32

```

# Planned features

- Polyscript language integration
- Audio System
- A development environment for making games with the console
- Development tools such as level editors made in the development environment
- Some sample games

# Supported platforms

Currently only Windows 64 bit. It shouldn't be that hard to port to other platforms though, as it uses bgfx for rendering and SDL for platform stuff. But it's early enough I have no interest in porting and testing on other platforms.

# Documentation

Also too early for this, but the codebase is extremely simple and hopefully self documenting. The main API that is of interest is the GraphicsChip.h file. The entire rendering API is contained there in a very small amount of code, so small I can list it here,

```cpp
void Init();
void DrawFrame(float w, float h);

// Basic draw 2D
void DrawSprite(const char* spritePath, float x, float y);

// Basic draw 3D
void BeginObject3D(EPrimitiveType type);
void EndObject3D();
void Vertex(Vec3f vec);
void Color(Vec4f col);
void TexCoord(Vec2f tex);
void Normal(Vec3f norm);

void SetClearColor(Vec3f color);

// Transforms
void MatrixMode(EMatrixMode mode);
void Perspective(f32 screenWidth, f32 screenHeight, f32 nearPlane, f32 farPlane, f32 fov);
void Translate(Vec3f translation);
void Rotate(Vec3f rotation);
void Scale(Vec3f scaling);
void Identity();

// Texturing
// Enable/Disable texturing
void BindTexture(const char* texturePath);
void UnbindTexture();

// Lighting
void NormalsMode(ENormalsMode mode);
void EnableLighting(bool enabled);
void Light(int id, Vec3f direction, Vec3f color);
void Ambient(Vec3f color);

// Depth Cueing
void EnableFog(bool enabled);
void SetFogStart(f32 start);
void SetFogEnd(f32 end);
void SetFogColor(Vec3f color);
```


# Compiling

The project uses premake5, which is packaged alongside the project, as well as all the dependencies you'll need. You simple run premake5.exe vs2019 (or another project type) and you'll get your project you can compile with
