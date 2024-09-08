project(lz4 C)

set(SOURCES
	"lz4.h"
	"lz4.c")

add_library (lz4 ${SOURCES})

target_include_directories(lz4 PRIVATE "/")

target_compile_definitions(lz4 PRIVATE
	_CRT_SECURE_NO_WARNINGS
	_CRT_NONSTDC_NO_WARNINGS)


