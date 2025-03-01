#define FT2_BUILD_LIBRARY

#ifdef _MSC_VER
#pragma push_macro("_CRT_SECURE_NO_WARNINGS")
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

/*
 * Sources
 *
 */

#include "src/autofit/autofit.c"
#include "src/bdf/bdf.c"
#include "src/cff/cff.c"
#include "src/base/ftbase.c"
#include "src/base/ftbitmap.c"
#include "src/cache/ftcache.c"
#include "src/base/ftdebug.c"
#include "src/base/ftfstype.c"
#include "src/base/ftgasp.c"
#include "src/base/ftglyph.c"
#include "src/base/ftinit.c"
#include "src/base/ftstroke.c"
#include "src/base/ftsystem.c"
#include "src/smooth/smooth.c"
#include "src/base/ftbdf.c"
#include "src/base/ftcid.c"
#include "src/base/ftgxval.c"
#include "src/base/ftotval.c"
#include "src/base/ftpatent.c"
#include "src/sdf/sdf.c"
#include "src/base/ftbbox.c"
#include "src/base/ftmm.c"
#include "src/base/ftpfr.c"
#include "src/base/ftsynth.c"
#include "src/base/fttype1.c"
#include "src/base/ftwinfnt.c"
#include "src/pcf/pcf.c"
#include "src/pfr/pfr.c"
#include "src/psaux/psaux.c"
#include "src/pshinter/pshinter.c"
#include "src/psnames/psnames.c"
#include "src/raster/raster.c"
#include "src/sfnt/sfnt.c"
#include "src/truetype/truetype.c"
#include "src/type1/type1.c"
#include "src/cid/type1cid.c"
#include "src/type42/type42.c"
#include "src/winfonts/winfnt.c"

/*
 * GZip
 *
 */

#ifdef FT_CONFIG_OPTION_SYSTEM_ZLIB
#undef FT_CONFIG_OPTION_SYSTEM_ZLIB
#endif
#undef NO_DUMMY_DECL
#define NO_DUMMY_DECL
#undef USE_ZLIB_ZCALLOC
#undef MY_ZCALLOC
#define MY_ZCALLOC /* prevent all zcalloc() & zfree() in zutils.c */
#include "src/gzip/zlib.h"
#undef NO_DUMMY_DECL
#undef MY_ZCALLOC

#include "src/gzip/ftgzip.c"

/*
 * LZW
 *
 */

#include "src/lzw/ftlzw.c"

