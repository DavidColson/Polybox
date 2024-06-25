project(bimg)

# bimg
#---------------------------------

file(GLOB BIMG_SOURCES "include/bimg/*.h" "src/*.h" "3rdparty/astc-codec/src/decoder/*")
list(APPEND BIMG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/image.cpp")
list(APPEND BIMG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/image_gnf.cpp")

add_library(bimg ${BIMG_SOURCES})
target_include_directories(bimg PUBLIC
	"../bx/include"
	"include"
	"3rdparty/astc-codec/include"
	"3rdparty/astc-codec")

BGFXCompat(bimg)

target_compile_definitions(bimg PUBLIC _HAS_EXCEPTIONS=0 _CRT_SECURE_NO_WARNINGS)

# bimg_decode
#---------------------------------
file(GLOB BIMG_DECODE_SOURCES "include/bimg/*.h")
list(APPEND BIMG_DECODE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/image_decode.cpp")

add_library(bimg_decode ${BIMG_DECODE_SOURCES})
target_include_directories(bimg_decode PUBLIC
	"../bx/include"
	"include"
	"3rdparty")

BGFXCompat(bimg_decode)

target_compile_definitions(bimg_decode PUBLIC _HAS_EXCEPTIONS=0 _CRT_SECURE_NO_WARNINGS)
