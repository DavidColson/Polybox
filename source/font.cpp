// Copyright 2020-2022 David Colson. All rights reserved.

namespace FreeType {
FT_Library* Get();
}

// ***********************************************************************

Font::~Font() {
	if (pArena)
		ArenaFinished(pArena);
}

// ***********************************************************************

void FontTextureFreeCallback(void* ptr, void* userData) {
    char* pTextureData = (char*)ptr;
    delete[] pTextureData;  // TODO: replace with our allocators
}

// ***********************************************************************

void Font::Initialize(String path, bool antialiasing, f32 weight) {
	// temporary solution, the font data will eventually be stored in lua as a table
	characters.pArena = pArena;

    FT_Library freetype;
    FT_Error err = FT_Init_FreeType(&freetype);
    if (err) {
        Log::Warn("FreeType Error: %s", FT_Error_String(err));
    }

    FT_Face face;

    err = FT_New_Face(freetype, path.pData, 0, &face);
    if (err) {
        Log::Warn("FreeType Error: %s", FT_Error_String(err));
    }

    // Rasterize the entire font to a texture atlas
    // **********************************************

    // Texture data
    i32 texHeight = 512;
    i32 texWidth = 512;
    char* pTextureDataAsR8 { nullptr };
    pTextureDataAsR8 = new char[texHeight * texWidth];  // TODO: replace with our allocators
    memset(pTextureDataAsR8, 0, texHeight * texWidth);

    ResizableArray<Packing::Rect> rects(g_pArenaFrame);

    FT_Set_Char_Size(face, 32 * 64, 32 * 64, 0, 0);

    // Prepare rects for packing
    for (int i = 0; i < 128; i++) {
        FT_Load_Char(face, i, FT_LOAD_DEFAULT);
        Packing::Rect newRect;
        newRect.w = face->glyph->bitmap.width + 1;
        newRect.h = face->glyph->bitmap.rows + 1;
        rects.PushBack(newRect);
    }
    Packing::SkylinePackRects(g_pArenaFrame, rects, texWidth, texHeight);

    for (int i = 0; i < 128; i++) {
        Packing::Rect& rect = rects[i];

        FT_Int32 flags = antialiasing ? FT_LOAD_RENDER : FT_LOAD_RENDER | FT_LOAD_MONOCHROME;
        FT_Load_Char(face, i, flags);

        if (i == 65) {
            f32 bloop = 9.f;
            bloop += 1.0f;
        }

        // Create the character with all it's appropriate data
        Character character;
        character.size = Vec2i(face->glyph->bitmap.width, face->glyph->bitmap.rows);
        character.bearing = Vec2i(face->glyph->bitmap_left, face->glyph->bitmap_top);
        character.advance = (face->glyph->advance.x) >> 6;
        character.UV0 = Vec2f((f32)rect.x / (f32)texWidth, (f32)rect.y / (f32)texHeight);
        character.UV1 = Vec2f((f32)(rect.x + character.size.x) / (f32)texWidth, (f32)(rect.y + character.size.y) / (f32)texHeight);
        characters.PushBack(character);

        if (antialiasing == false) {
            // Blit the glyph's image into our texture atlas, but converting from 8 pixels per bit to 1 byte per pixel (monochrome)
            char* pDestination = pTextureDataAsR8 + rect.y * texWidth + rect.x;
            for (u64 y = 0; y < face->glyph->bitmap.rows; y++) {
                for (char byte_index = 0; byte_index < face->glyph->bitmap.pitch; byte_index++) {
                    char b = face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch + byte_index];
                    char bitsDone = byte_index * 8;
                    u32 rowStart = (u32)y * texWidth + bitsDone;

                    for (u64 bit_index = 0; bit_index < fmin(face->glyph->bitmap.width - bitsDone, 8); bit_index++) {
                        char bit = (b & (1 << (7 - bit_index)));
                        pDestination[rowStart + bit_index] = bit ? 255 : 0;
                    }
                }
            }
        } else {
            // Blit the glyph's image into our texture atlas
            char* pSourceData = (char*)face->glyph->bitmap.buffer;
            char* pDestination = pTextureDataAsR8 + rect.y * texWidth + rect.x;
            i32 sourceDataPitch = face->glyph->bitmap.pitch;
            for (u32 y = 0; y < face->glyph->bitmap.rows; y++, pSourceData += sourceDataPitch, pDestination += texWidth) {
                memcpy(pDestination, pSourceData, face->glyph->bitmap.width);
            }
        }

        i32 test = 7;
        test++;
    }

    // Convert texture atlas to RGBA8 format so it works with general 2D shaders
    char* pTextureDataAsRGBA8 { nullptr };
    const char dstBpp = 32;
    const u32 dstPitch = texWidth * dstBpp / 8;
    const u32 dstSize = texHeight * dstPitch;
    pTextureDataAsRGBA8 = new char[dstSize];
    memset(pTextureDataAsRGBA8, 0, dstSize);
    for (u64 i = 0; i < dstSize; i += 4) {
        pTextureDataAsRGBA8[i] = pTextureDataAsR8[i / 4];
        pTextureDataAsRGBA8[i + 1] = pTextureDataAsR8[i / 4];
        pTextureDataAsRGBA8[i + 2] = pTextureDataAsR8[i / 4];
        pTextureDataAsRGBA8[i + 3] = pTextureDataAsR8[i / 4];
    }

	sg_image_desc imageDesc = {
		.width = texWidth,
		.height = texHeight,
		.pixel_format = SG_PIXELFORMAT_RGBA8,
		.label = path.pData
	};

	sg_range pixelsData;
	pixelsData.ptr = (void*)pTextureDataAsRGBA8;
	pixelsData.size = texWidth * texHeight * 4 * sizeof(u8);
	imageDesc.data.subimage[0][0] = pixelsData;
	fontTexture.handle = sg_make_image(&imageDesc);
    fontTexture.height = texHeight;
    fontTexture.width = texWidth;

    delete[] pTextureDataAsR8;
    delete[] pTextureDataAsRGBA8;

    FT_Done_Face(face);
    FT_Done_FreeType(freetype);
}
