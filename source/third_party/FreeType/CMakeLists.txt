project(FreeType C)

set(SOURCES
	"include/ft2build.h"
	"include/freetype/freetype.h"
	"include/freetype/ftadvanc.h"
	"include/freetype/ftbbox.h"
	"include/freetype/ftbdf.h"
	"include/freetype/ftbitmap.h"
	"include/freetype/ftbzip2.h"
	"include/freetype/ftcache.h"
	"include/freetype/ftchapters.h"
	"include/freetype/ftcid.h"
	"include/freetype/ftcolor.h"
	"include/freetype/ftdriver.h"
	"include/freetype/fterrdef.h"
	"include/freetype/fterrors.h"
	"include/freetype/ftfntfmt.h"
	"include/freetype/ftgasp.h"
	"include/freetype/ftglyph.h"
	"include/freetype/ftgxval.h"
	"include/freetype/ftgzip.h"
	"include/freetype/ftimage.h"
	"include/freetype/ftincrem.h"
	"include/freetype/ftlcdfil.h"
	"include/freetype/ftlist.h"
	"include/freetype/ftlogging.h"
	"include/freetype/ftlzw.h"
	"include/freetype/ftmac.h"
	"include/freetype/ftmm.h"
	"include/freetype/ftmodapi.h"
	"include/freetype/ftmoderr.h"
	"include/freetype/ftotval.h"
	"include/freetype/ftoutln.h"
	"include/freetype/ftparams.h"
	"include/freetype/ftpfr.h"
	"include/freetype/ftrender.h"
	"include/freetype/ftsizes.h"
	"include/freetype/ftsnames.h"
	"include/freetype/ftstroke.h"
	"include/freetype/ftsynth.h"
	"include/freetype/ftsystem.h"
	"include/freetype/fttrigon.h"
	"include/freetype/fttypes.h"
	"include/freetype/ftwinfnt.h"
	"include/freetype/t1tables.h"
	"include/freetype/ttnameid.h"
	"include/freetype/tttables.h"
	"include/freetype/tttags.h"
	"include/freetype/config/ftconfig.h"
	"include/freetype/config/ftheader.h"
	"include/freetype/config/ftmodule.h"
	"include/freetype/config/ftoption.h"
	"include/freetype/config/ftstdlib.h"
	"include/freetype/config/integer-types.h"
	"include/freetype/config/mac-support.h"
	"include/freetype/config/public-macros.h"
	"src/autofit/autofit.c"
	"src/base/ftbase.c"
	"src/base/ftbbox.c"
	"src/base/ftbdf.c"
	"src/base/ftbitmap.c"
	"src/base/ftcid.c"
	"src/base/ftfstype.c"
	"src/base/ftgasp.c"
	"src/base/ftglyph.c"
	"src/base/ftgxval.c"
	"src/base/ftinit.c"
	"src/base/ftmm.c"
	"src/base/ftotval.c"
	"src/base/ftpatent.c"
	"src/base/ftpfr.c"
	"src/base/ftstroke.c"
	"src/base/ftsynth.c"
	"src/base/fttype1.c"
	"src/base/ftwinfnt.c"
	"src/bdf/bdf.c"
	"src/bzip2/ftbzip2.c"
	"src/cache/ftcache.c"
	"src/cff/cff.c"
	"src/cid/type1cid.c"
	"src/gzip/ftgzip.c"
	"src/lzw/ftlzw.c"
	"src/pcf/pcf.c"
	"src/pfr/pfr.c"
	"src/psaux/psaux.c"
	"src/pshinter/pshinter.c"
	"src/psnames/psnames.c"
	"src/raster/raster.c"
	"src/sdf/sdf.c"
	"src/sfnt/sfnt.c"
	"src/smooth/smooth.c"
	"src/truetype/truetype.c"
	"src/type1/type1.c"
	"src/type42/type42.c"
	"src/winfonts/winfnt.c"
	"src/base/ftdebug.c"
	"src/base/ftsystem.c")

add_library (FreeType ${SOURCES})

target_include_directories(FreeType PRIVATE
    "include/"
	"include/freetype")

target_compile_definitions(FreeType PRIVATE
	FT2_BUILD_LIBRARY
	_CRT_SECURE_NO_WARNINGS
	_CRT_NONSTDC_NO_WARNINGS)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(FreeType PRIVATE -w)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(FreeType PRIVATE /w)
endif()


