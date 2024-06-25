
set(SOURCES 
	"bind_game_chip.cpp"
	"bind_game_chip.h"
	"bind_graphics_chip.cpp"
	"bind_graphics_chip.h"
	"bind_mesh.h"
	"bind_mesh.cpp"
	"bind_scene.h"
	"bind_scene.cpp"
	"engine.h"
	"engine.cpp"
	"font.h"
	"font.cpp"
	"game_chip.h"
	"game_chip.cpp"
	"graphics_chip.h"
	"graphics_chip.cpp"
	"image.cpp"
	"image.h"
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

target_link_libraries(polybox 
	common_lib 
	bgfx 
	bimg 
	bimg_decode 
	bx 
	FreeType 
	lua 
	SDL2 
	SDL2main
	gdi32
	kernel32
	psapi)

target_include_directories(polybox PUBLIC 
	"${PROJ_ROOT_PATH}/source/"
	"${PROJ_ROOT_PATH}/source/third_party/bgfx/include/"
	"${PROJ_ROOT_PATH}/source/third_party/bimg/include/"
	"${PROJ_ROOT_PATH}/source/third_party/bx/include/"
	"${PROJ_ROOT_PATH}/source/third_party/FreeType/include/"
	"${PROJ_ROOT_PATH}/source/third_party/lua/src/"
	"${PROJ_ROOT_PATH}/source/third_party/SDL2/include/"
	"${PROJ_ROOT_PATH}/source/common_lib/source")

target_link_directories(polybox PUBLIC
	"${PROJ_ROOT_PATH}/source/third_party/SDL2/lib/x64")

target_compile_definitions(polybox PUBLIC
	WINDOWS_IGNORE_PACKING_MISMATCH
	__STDC_LIMIT_MACROS
	__STDC_FORMAT_MACROS
	__STDC_CONSTANT_MACROS)

target_compile_shaders(polybox 
	core2d.vsc
	core3d.vsc
	fullscreen.vsc
	core2d.fsc
	core2d_textured.fsc
	core3d.fsc
	core3d_textured.fsc
	fullscreen.fsc
	crt.fsc
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
