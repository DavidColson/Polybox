
group "third_party/FreeType"

project "FreeType"
    kind "StaticLib"
    language "C"
    defines { "FT2_BUILD_LIBRARY", "$<$<CONFIG:Debug>:FT_CONFIG_OPTION_ERROR_STRINGS>" }
    warnings "off"
    files
    {
        "FreeType/include/ft2build.h",
        "FreeType/include/freetype/*.h",
        "FreeType/include/freetype/config/*.h",
        "FreeType/include/freetype/internal/*.h",
        "FreeType/src/autofit/autofit.c",
        "FreeType/src/base/ftbase.c",
        "FreeType/src/base/ftbbox.c",
        "FreeType/src/base/ftbdf.c",
        "FreeType/src/base/ftbitmap.c",
        "FreeType/src/base/ftcid.c",
        "FreeType/src/base/ftfstype.c",
        "FreeType/src/base/ftgasp.c",
        "FreeType/src/base/ftglyph.c",
        "FreeType/src/base/ftgxval.c",
        "FreeType/src/base/ftinit.c",
        "FreeType/src/base/ftmm.c",
        "FreeType/src/base/ftotval.c",
        "FreeType/src/base/ftpatent.c",
        "FreeType/src/base/ftpfr.c",
        "FreeType/src/base/ftstroke.c",
        "FreeType/src/base/ftsynth.c",
        "FreeType/src/base/fttype1.c",
        "FreeType/src/base/ftwinfnt.c",
        "FreeType/src/bdf/bdf.c",
        "FreeType/src/bzip2/ftbzip2.c",
        "FreeType/src/cache/ftcache.c",
        "FreeType/src/cff/cff.c",
        "FreeType/src/cid/type1cid.c",
        "FreeType/src/gzip/ftgzip.c",
        "FreeType/src/lzw/ftlzw.c",
        "FreeType/src/pcf/pcf.c",
        "FreeType/src/pfr/pfr.c",
        "FreeType/src/psaux/psaux.c",
        "FreeType/src/pshinter/pshinter.c",
        "FreeType/src/psnames/psnames.c",
        "FreeType/src/raster/raster.c",
        "FreeType/src/sdf/sdf.c",
        "FreeType/src/sfnt/sfnt.c",
        "FreeType/src/smooth/smooth.c",
        "FreeType/src/truetype/truetype.c",
        "FreeType/src/type1/type1.c",
        "FreeType/src/type42/type42.c",
        "FreeType/src/winfonts/winfnt.c",
        "FreeType/src/base/ftdebug.c",
        "FreeType/src/base/ftsystem.c",
    }
    includedirs
    {
        "FreeType/include/",
        "FreeType/include/freetype/"
    }
    filter { "system:windows", "configurations:Debug*" }
        buildoptions { "/fsanitize=address" }
        flags { "NoIncrementalLink" }
        editandcontinue "Off"
    filter "action:vs*"
        defines { "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_WARNINGS" }

group ""