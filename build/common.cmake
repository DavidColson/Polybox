
# Compiler settings
# --------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
elseif(MSVC)
	string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs- /EHc- /GR-")
endif()

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# Utilities
# --------------------------------

macro(GroupSources dir)
    file(GLOB_RECURSE sources RELATIVE ${dir} *.h *.hpp *.c *.cpp *.cc)
    foreach(source ${sources})
        get_filename_component(source_path "${source}" PATH)
        string(REPLACE "/" "\\" source_path_msvc "${source_path}")
        source_group("${source_path_msvc}" FILES "${source}")
    endforeach()
endmacro()

macro(BGFXCompat project)
	if( MSVC )
		target_include_directories( ${project}
			PUBLIC
			"${PROJ_ROOT_PATH}/source/third_party/bx/include/compat/msvc")
	elseif( MINGW )
		target_include_directories( ${project}
			PUBLIC
			"${PROJ_ROOT_PATH}/source/third_party/bx/include/compat/msvc")
	elseif( APPLE )
		target_include_directories( ${project}
			PUBLIC
			"${PROJ_ROOT_PATH}/source/third_party/bx/include/compat/msvc")
	endif()
endmacro()

function(target_compile_shaders TARGET_NAME)
	set(SHADER_FILES ${ARGN})

	list(LENGTH SHADER_FILES FILE_COUNT)
	if(FILE_COUNT EQUAL 0)
		message(FATAL_ERROR "Cannot create a shaders target without any source files")
	endif()

	set(SHADER_COMPILER_PATH "${PROJ_ROOT_PATH}/source/third_party/bgfx/tools/bin/shaderc.exe")
	set(COMPILED_SHADERS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")

	foreach(SHADER_FILE ${SHADER_FILES})
		get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
		get_filename_component(SHADER_EXT ${SHADER_FILE} EXT)
		if(SHADER_EXT STREQUAL ".vsc")
			set(COMPILED_SHADER_FILE "${COMPILED_SHADERS_OUTPUT_DIR}/${SHADER_NAME}.vbin")
			add_custom_command(
				OUTPUT ${COMPILED_SHADER_FILE}
				COMMAND ${SHADER_COMPILER_PATH} -f "${CMAKE_SOURCE_DIR}/shaders/${SHADER_FILE}" -o ${COMPILED_SHADER_FILE} --type v --varyingdef "${CMAKE_SOURCE_DIR}/shaders/varying.def.sc" -p vs_4_0 --platform windows
				DEPENDS ${CMAKE_SOURCE_DIR}/shaders/${SHADER_FILE}
				COMMENT "Compiling ${SHADER_FILE} to ${COMPILED_SHADER_FILE}"
			)
		elseif(SHADER_EXT STREQUAL ".fsc")    
			set(COMPILED_SHADER_FILE "${COMPILED_SHADERS_OUTPUT_DIR}/${SHADER_NAME}.fbin")
			add_custom_command(
				OUTPUT ${COMPILED_SHADER_FILE}
				COMMAND ${SHADER_COMPILER_PATH} -f "${CMAKE_SOURCE_DIR}/shaders/${SHADER_FILE}" -o ${COMPILED_SHADER_FILE} --type f --varyingdef "${CMAKE_SOURCE_DIR}/shaders/varying.def.sc" -p ps_4_0 --platform windows
				DEPENDS ${CMAKE_SOURCE_DIR}/shaders/${SHADER_FILE}
				COMMENT "Compiling ${SHADER_FILE} to ${COMPILED_SHADER_FILE}"
			)
		endif()
		list(APPEND COMPILED_SHADER_FILES ${COMPILED_SHADER_FILE})
	endforeach()

	add_custom_target(${TARGET_NAME}_compile_shaders ALL DEPENDS ${COMPILED_SHADER_FILES})
	add_dependencies(${TARGET_NAME} ${TARGET_NAME}_compile_shaders)

	add_custom_command(
		TARGET ${TARGET_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		${COMPILED_SHADERS_OUTPUT_DIR}
		"$<TARGET_FILE_DIR:${TARGET_NAME}>/shaders")
endfunction()
