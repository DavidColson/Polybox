// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include "Vec3.h"
#include "Matrix.h"

template<typename T>
struct AABB
{
    //REFLECT()

    Vec3<T> min;
    Vec3<T> max;

    // Probably want merge for mesh combining primitives?
};

template<typename T>
AABB<T> TransformAABB(AABB<T> a, Matrix<T> m)
{
    AABB<T> res;
    Vec3<T> sca = m.ExtractScaling();
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            float e = m.m[i][j] * a.min[j];
            float f = m.m[i][j] * a.max[j];
            if (e < f)
            {
                res.min[i] += e;
                res.max[i] += f;
            }
            else
            {
                res.min[i] += f;
                res.max[i] += e;
            }
        }
        res.min[i] *= sca[i];
        res.max[i] *= sca[i];
        res.min[i] += m.m[i][3];
        res.max[i] += m.m[i][3];
    }
    return res;
}

typedef AABB<float> AABBf;
typedef AABB<double> AABBd;