// Copyright 2020-2022 David Colson. All rights reserved.

#include "Image.h"

#include <light_string.h>

#include <SDL_rwops.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


// ***********************************************************************

Image::Image(String path) {


	int n;
	u8* pData = stbi_load(path.pData, &width, &height, &n, 4);

	sg_image_desc imageDesc = {
		.width = width,
		.height = height,
		.pixel_format = SG_PIXELFORMAT_RGBA8,
		.label = path.pData
	};

	sg_range pixelsData;
	pixelsData.ptr = (void*)pData;
	pixelsData.size = width * height * 4 * sizeof(u8);
	imageDesc.data.subimage[0][0] = pixelsData;
	handle = sg_make_image(&imageDesc);

	stbi_image_free(pData);
}

// ***********************************************************************

Image::~Image() {
	if (refCount <= 0)
		sg_destroy_image(handle);
}
