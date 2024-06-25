project(imgui)

set(SOURCES
    "imgui.cpp"
    "imgui_demo.cpp"
    "imgui_draw.cpp"
    "imgui_widgets.cpp"
    "imgui_tables.cpp"
    "imconfig.h"
    "imgui.h"
    "imgui_internal.h"
    "imstb_rectpack.h"
    "imstb_textedit.h"
    "imstb_truetype.h"
	"backends/imgui_impl_sdl2.cpp"
	"backends/imgui_impl_sdl2.h"
    )
add_library (imgui ${SOURCES})

target_include_directories(imgui PUBLIC
    "/"
    "../bgfx/include/"
    "../bimg/include/"
    "../SDL2/include"
    "../bx/include")

