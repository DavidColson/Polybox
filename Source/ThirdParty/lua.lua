
group "ThirdParty/lua"

project "lua"
    kind "StaticLib"
    language "C"
    warnings "off"
    files
    {
        "lua/src/lopcodes.c",
        "lua/src/lapi.c",
        "lua/src/lcode.c",
        "lua/src/lctype.c",
        "lua/src/ldebug.c",
        "lua/src/ldo.c",
        "lua/src/ldump.c",
        "lua/src/lfunc.c",
        "lua/src/lgc.c",
        "lua/src/llex.c",
        "lua/src/lmem.c",
        "lua/src/lobject.c",
        "lua/src/lparser.c",
        "lua/src/lstate.c",
        "lua/src/lstring.c",
        "lua/src/ltable.c",
        "lua/src/ltm.c",
        "lua/src/lundump.c",
        "lua/src/lvm.c",
        "lua/src/lzio.c",
        "lua/src/lauxlib.c",
        "lua/src/lbaselib.c",
        "lua/src/lcorolib.c",
        "lua/src/ldblib.c",
        "lua/src/liolib.c",
        "lua/src/lmathlib.c",
        "lua/src/loadlib.c",
        "lua/src/loslib.c",
        "lua/src/lstrlib.c",
        "lua/src/ltablib.c",
        "lua/src/lutf8lib.c",
        "lua/src/linit.c"
    }
    includedirs
    {
        "lua/src/"
    }

group ""