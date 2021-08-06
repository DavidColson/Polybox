#pragma once

template<typename T> struct Matrix;
template<typename T> struct Vec3;

template<typename T>
struct Quat
{
    //REFLECT()

    T x;
    T y;
    T z;
    T w;

    Quat() : x(T()), y(T()), z(T()), w(T(1.0)) {}
    Quat(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

    // @Incomplete: Angle axis
    Quat(Vec3<T> axis, T angle) 
    {
        float sa2 = sinf(angle * 0.5f);
        x = axis.x * sa2;
        y = axis.y * sa2;
        z = axis.z * sa2;
        w = cosf(angle * 0.5f);
    }
    
	inline static Quat Identity()
	{
		return Quat(0.0f, 0.0f, 0.0f, 1.0f);
	}

    inline static Quat MakeFromEuler(Vec3<T> v)
    {
        return MakeFromEuler(v.x, v.y, v.z);
    }

    inline static Quat MakeFromEuler(T x, T y, T z)
    {
        // This is a body 3-2-1 (z, then y, then x) rotation
        const float cx = cosf(x * 0.5f);
		const float sx = sinf(x * 0.5f);
		const float cy = cosf(y * 0.5f);
		const float sy = sinf(y * 0.5f);
		const float cz = cosf(z * 0.5f);
		const float sz = sinf(z * 0.5f);

		Quat res;
		res.x = sx*cy*cz - cx*sy*sz;
		res.y = cx*sy*cz + sx*cy*sz;
		res.z = cx*cy*sz - sx*sy*cz;
		res.w = cx*cy*cz + sx*sy*sz;
        return res;    
    }

    inline Matrix<T> ToMatrix() const
    {
        // https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation

        Matrix<T> mat;
		mat.m[0][0] = 1.0f - 2.0f*y*y - 2.0f*z*z;   mat.m[1][0] = 2.0f*x*y - 2.0f*z*w;    	    mat.m[2][0] = 2.0f*x*z + 2.0f*y*w;		    mat.m[3][0] = 0.0f;
		mat.m[0][1] = 2.0f*x*y + 2.0f*z*w;          mat.m[1][1] = 1.0f - 2.0f*x*x - 2.0f*z*z;	mat.m[2][1] = 2.0f*y*z - 2.0f*x*w;		    mat.m[3][1] = 0.0f;
		mat.m[0][2] = 2.0f*x*z - 2.0f*y*w;	        mat.m[1][2] = 2.0f*y*z + 2.0f*x*w;			mat.m[2][2] = 1.0f - 2.0f*x*x - 2.0f*y*y;	mat.m[3][2] = 0.0f;
		mat.m[0][3] = 0.0f;						    mat.m[1][3] = 0.0f;						    mat.m[2][3] = 0.0f;						    mat.m[3][3] = 1.0f;
		return mat;
    }

    inline Quat GetInverse() const
    {
        // Note this is actually the conjugate, but since we always have it be unit length the conjugate is equal to the inverse
        return Quat(-x, -y, -z, w);
    }

    // Normalize
    inline void Normalize()
    {
        float invNorm = 1.0f / sqrtf(x*x + y*y + z*z + w*w);
        x *= invNorm;
        y *= invNorm;
        z *= invNorm;
        w *= invNorm;
    }

    inline Quat GetNormalized()
    {
        Quat copy(*this);
        copy.Normalize();
        return copy;
    }

    // Quat * Quat
    inline Quat operator*(const Quat& rhs) const
    {
        return Quat(
            y * rhs.z - z * rhs.y + w * rhs.x + rhs.w * x,
            z * rhs.x - x * rhs.z + w * rhs.y + rhs.w * y,
            x * rhs.y - y * rhs.x + w * rhs.z + rhs.w * z,
            w * rhs.w - (x * rhs.x + y * rhs.y + z * rhs.z)
        );
    }

    inline Vec3<T> operator*(const Vec3<T> v) const
    {
        // Note this actually implements q*v'*q^1 with v' = (v, 0)

        // Was derived by working out qv'q^1 and simplifying using vector identities
        // q = (u,s)
        // v' = 2(u.v)u + (s*s - u.u)v + 2s(u X v)

        const float a = 2.0f * (x*v.x + y*v.y + z*v.z);
        const float b = w * w - (x*x + y*y + z*z);
        Vec3<T> s2uCrossv;
        s2uCrossv.x =  2.0f * w * (y * v.z - z * v.y);
        s2uCrossv.y =  2.0f * w * (z * v.x - x * v.z);
        s2uCrossv.z =  2.0f * w * (x * v.y - y * v.x);
			
		Vec3<T> res;
        res.x = a * x + b * v.x + s2uCrossv.x;
        res.y = a * y + b * v.y + s2uCrossv.y;
        res.z = a * z + b * v.z + s2uCrossv.z;
        return res;
    }

    inline Vec3<T> GetForwardVector()
    {
        return Vec3<T>(2 * (x*z + y*w), 2 * (y*z - x*w), z*z + w*w - x*x - y*y);
    }

    inline Vec3<T> GetRightVector()
    {
        return Vec3<T>(x*x + w*w - y*y - z*z, 2 * (x*y + z*w), 2 * (x*z - y*w));
    }

    inline Vec3<T> GetUpVector()
    {
        return Vec3<T>(2 * (x*y - z*w), y*y + w*w - x*x - z*z, 2 * (y*z + x*w));
    }

    inline Vec3<T> GetAxis()
    {
        return Vec3<T>(x, y, z).GetNormalized();
    }

    inline T GetAngle()
    {
        return 2 * acosf(w);
    }

    inline Vec3<T> GetEulerAngles()
    {
        Vec3<T> res;
        res.x = atan2(T(2.0) * (w*x + y*z), T(1.0) - T(2.0) * (x*x + y*y));
        res.y = asin(T(2.0) * (w*y - z*x));
        res.z = atan2(T(2.0) * (w*z + x*y), T(1.0) - T(2.0) * (y*y + z*z));
        return res;
    }

    // TO STRING
};

typedef Quat<float> Quatf;
typedef Quat<double> Quatd;