// Copyright 2020-2022 David Colson. All rights reserved.

#include "Font.h"

#include "rect_packing.h"

#include <bimg/bimg.h>
#include <bx/allocator.h>
#include <defer.h>
#include <freetype/ftmm.h>
#include <log.h>
#include <algorithm>


// ***********************************************************************

void FontTextureFreeCallback(void* ptr, void* userData) {
    uint8_t* pTextureData = (uint8_t*)ptr;
    delete[] pTextureData;  // TODO: replace with our allocators
}

// ***********************************************************************

Font::Font(String path, bool antialiasing, float weight) {
    FT_Library freetype;
    FT_Init_FreeType(&freetype);

    FT_Face face;

    FT_Error err = FT_New_Face(freetype, path.pData, 0, &face);
    if (err) {
        Log::Warn("FreeType Error: %s", FT_Error_String(err));
    }

    // Modify weight for variable fonts
    FT_MM_Var* pVar = new FT_MM_Var;  // TODO: Replace with our allocators
    FT_Get_MM_Var(face, &pVar);

    int weightIndex = -1;
    for (int i = 0; i < (int)pVar->num_axis; i++) {
        if (pVar->axis[i].name == "Weight")
            weightIndex = i;
    }

    if (weightIndex > -1) {
        FT_Fixed coords[16];
        FT_Get_Var_Design_Coordinates(face, pVar->num_axis, coords);
        coords[weightIndex] = (int)weight * 65536;
        FT_Set_Var_Design_Coordinates(face, pVar->num_axis, coords);
    }
    delete pVar;

    // Rasterize the entire font to a texture atlas
    // **********************************************

    // Texture data
    int texHeight = 512;
    int texWidth = 512;
    uint8_t* pTextureDataAsR8 { nullptr };
    pTextureDataAsR8 = new uint8_t[texHeight * texWidth];  // TODO: replace with our allocators
    memset(pTextureDataAsR8, 0, texHeight * texWidth);

    ResizableArray<Packing::Rect> rects;
    defer(rects.Free());

    FT_Set_Char_Size(face, 32 * 64, 32 * 64, 0, 0);

    // Prepare rects for packing
    for (int i = 0; i < 128; i++) {
        FT_Load_Char(face, i, FT_LOAD_DEFAULT);
        Packing::Rect newRect;
        newRect.w = face->glyph->bitmap.width + 1;
        newRect.h = face->glyph->bitmap.rows + 1;
        rects.PushBack(newRect);
    }
    Packing::SkylinePackRects(rects, texWidth, texHeight);

    for (int i = 0; i < 128; i++) {
        Packing::Rect& rect = rects[i];

        FT_Int32 flags = antialiasing ? FT_LOAD_RENDER : FT_LOAD_RENDER | FT_LOAD_MONOCHROME;
        FT_Load_Char(face, i, flags);

        if (i == 65) {
            float bloop = 9.f;
            bloop += 1.0f;
        }

        // Create the character with all it's appropriate data
        Character character;
        character.size = Vec2i(face->glyph->bitmap.width, face->glyph->bitmap.rows);
        character.bearing = Vec2i(face->glyph->bitmap_left, face->glyph->bitmap_top);
        character.advance = (face->glyph->advance.x) >> 6;
        character.UV0 = Vec2f((float)rect.x / (float)texWidth, (float)rect.y / (float)texHeight);
        character.UV1 = Vec2f((float)(rect.x + character.size.x) / (float)texWidth, (float)(rect.y + character.size.y) / (float)texHeight);
        characters.PushBack(character);

        if (antialiasing == false) {
            // Blit the glyph's image into our texture atlas, but converting from 8 pixels per bit to 1 byte per pixel (monochrome)
            uint8_t* pDestination = pTextureDataAsR8 + rect.y * texWidth + rect.x;
            for (size_t y = 0; y < face->glyph->bitmap.rows; y++) {
                for (uint8_t byte_index = 0; byte_index < face->glyph->bitmap.pitch; byte_index++) {
                    uint8_t byte = face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch + byte_index];
                    uint8_t bitsDone = byte_index * 8;
                    uint32_t rowStart = (uint32_t)y * texWidth + bitsDone;

                    for (size_t bit_index = 0; bit_index < fmin(face->glyph->bitmap.width - bitsDone, 8); bit_index++) {
                        uint8_t bit = (byte & (1 << (7 - bit_index)));
                        pDestination[rowStart + bit_index] = bit ? 255 : 0;
                    }
                }
            }
        } else {
            // Blit the glyph's image into our texture atlas
            uint8_t* pSourceData = face->glyph->bitmap.buffer;
            uint8_t* pDestination = pTextureDataAsR8 + rect.y * texWidth + rect.x;
            int sourceDataPitch = face->glyph->bitmap.pitch;
            for (uint32_t y = 0; y < face->glyph->bitmap.rows; y++, pSourceData += sourceDataPitch, pDestination += texWidth) {
                memcpy(pDestination, pSourceData, face->glyph->bitmap.width);
            }
        }

        int test = 7;
        test++;
    }

    // Convert texture atlas to RGBA8 format so it works with general 2D shaders
    uint8_t* pTextureDataAsRGBA8 { nullptr };
    const uint8_t dstBpp = bimg::getBitsPerPixel(bimg::TextureFormat::RGBA8);
    const uint32_t dstPitch = texWidth * dstBpp / 8;
    const uint32_t dstSize = texHeight * dstPitch;
    pTextureDataAsRGBA8 = new uint8_t[dstSize];
    memset(pTextureDataAsRGBA8, 0, dstSize);
    for (size_t i = 0; i < dstSize; i += 4) {
        pTextureDataAsRGBA8[i] = pTextureDataAsR8[i / 4];
        pTextureDataAsRGBA8[i + 1] = pTextureDataAsR8[i / 4];
        pTextureDataAsRGBA8[i + 2] = pTextureDataAsR8[i / 4];
        pTextureDataAsRGBA8[i + 3] = pTextureDataAsR8[i / 4];
    }

    const bgfx::Memory* pMem = bgfx::makeRef(pTextureDataAsRGBA8, dstSize, FontTextureFreeCallback, nullptr);
    fontTexture.m_handle = bgfx::createTexture2D(texWidth, texHeight, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE | BGFX_SAMPLER_POINT, pMem);
    fontTexture.m_height = texHeight;
    fontTexture.m_width = texWidth;

    delete[] pTextureDataAsR8;

    FT_Done_Face(face);
    FT_Done_FreeType(freetype);
}
