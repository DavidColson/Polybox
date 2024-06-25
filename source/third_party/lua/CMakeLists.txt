project(lua C)

set(SOURCES
	"src/lopcodes.c"
	"src/lapi.c"
	"src/lcode.c"
	"src/lctype.c"
	"src/ldebug.c"
	"src/ldo.c"
	"src/ldump.c"
	"src/lfunc.c"
	"src/lgc.c"
	"src/llex.c"
	"src/lmem.c"
	"src/lobject.c"
	"src/lparser.c"
	"src/lstate.c"
	"src/lstring.c"
	"src/ltable.c"
	"src/ltm.c"
	"src/lundump.c"
	"src/lvm.c"
	"src/lzio.c"
	"src/lauxlib.c"
	"src/lbaselib.c"
	"src/lcorolib.c"
	"src/ldblib.c"
	"src/liolib.c"
	"src/lmathlib.c"
	"src/loadlib.c"
	"src/loslib.c"
	"src/lstrlib.c"
	"src/ltablib.c"
	"src/lutf8lib.c"
	"src/linit.c"
    )
add_library (lua ${SOURCES})

target_include_directories(lua PRIVATE
    "src")

