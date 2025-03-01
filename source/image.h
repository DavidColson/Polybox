// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

struct Image : public LuaObject {
    Image() {}
    Image(String path);
    virtual ~Image();

	sg_image handle;
    i32 width;
    i32 height;
};
