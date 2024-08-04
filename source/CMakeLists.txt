
set(SOURCES 
	"bind_input.cpp"
	"bind_input.h"
	"bind_graphics.cpp"
	"bind_graphics.h"
	"bind_mesh.h"
	"bind_mesh.cpp"
	"bind_scene.h"
	"bind_scene.cpp"
	"main.h"
	"main.cpp"
	"cpu.h"
	"cpu.cpp"
	"font.h"
	"font.cpp"
	"input.h"
	"input.cpp"
	"graphics.h"
	"graphics.cpp"
	"sokol_impl_d3d11.cpp"
	"image.cpp"
	"image.h"
	"buffer.h"
	"buffer.cpp"
	"lua_common.h"
	"lua_common.cpp"
	"mesh.h"
	"mesh.cpp"
	"rect_packing.h"
	"rect_packing.cpp"
	"scene.h"
	"scene.cpp"
	"shapes.h"
	"shapes.cpp")

add_executable(polybox ${SOURCES})

target_link_libraries(polybox PUBLIC
	Luau.Compiler
	Luau.VM
	Luau.Analysis
	common_lib 
	FreeType 
	SDL2 
	SDL2main
	gdi32
	kernel32
	psapi)

target_include_directories(polybox PUBLIC 
	"${PROJ_ROOT_PATH}/source/"
	"${PROJ_ROOT_PATH}/source/third_party/FreeType/include/"
	"${PROJ_ROOT_PATH}/source/third_party/luau/Compiler/include/"
	"${PROJ_ROOT_PATH}/source/third_party/luau/Analysis/include/"
	"${PROJ_ROOT_PATH}/source/third_party/luau/VM/include/"
	"${PROJ_ROOT_PATH}/source/third_party/SDL2/include/"
	"${PROJ_ROOT_PATH}/source/third_party/sokol/"
	"${PROJ_ROOT_PATH}/source/third_party/stb/"
	"${PROJ_ROOT_PATH}/source/common_lib/source")

target_link_directories(polybox PUBLIC
	"${PROJ_ROOT_PATH}/source/third_party/SDL2/lib/x64")

target_compile_definitions(polybox PUBLIC
	WINDOWS_IGNORE_PACKING_MISMATCH
	__STDC_LIMIT_MACROS
	__STDC_FORMAT_MACROS
	__STDC_CONSTANT_MACROS)

target_compile_shaders(polybox
	core3d.shader
	compositor.shader
)

# Copy the necessary files to the output directory
# In future may want to move the exe to the assets rather than the inverse, since it'll always be smaller
add_custom_command(
        TARGET polybox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${PROJ_ROOT_PATH}/source/third_party/SDL2/lib/x64/SDL2.dll"
		"$<TARGET_FILE_DIR:polybox>")

add_custom_command(
        TARGET polybox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${PROJ_ROOT_PATH}/assets/"
		"$<TARGET_FILE_DIR:polybox>/assets/")

BGFXCompat(polybox)
