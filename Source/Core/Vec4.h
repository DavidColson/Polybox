// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

template<typename T>
struct Vec3;

template<typename T>
struct Vec2;

template<typename T>
struct Vec4
{
//	REFLECT()

	Vec4() : x(T()), y(T()), z(T()), w(T()) {}
	Vec4(T val) : x(val), y(val), z(val), w(val) {}
	Vec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

	T x;
	T y;
	T z;
	T w;

	/**
	* Add a scalar to each component of this vector and return the result
	*
	* @param  scalar The scalar to add
	* @return The result of adding the scalar
	**/
	inline Vec4 operator+(const T& scalar) const
	{
		return Vec4(x + scalar, y + scalar, z + scalar, w + scalar);
	}

	/**
	* Subtract a scalar from this vector and return the result
	*
	* @param  scalar The scalar to subtract
	* @return The result of subtracting the scalar
	**/
	inline Vec4 operator-(const T& scalar) const
	{
		return Vec4(x - scalar, y - scalar, z - scalar, w - scalar);
	}

	/**
	* Multiply each component by this vector and return the result
	*
	* @param  scalar The scalar to multiply
	* @return The result of multiplying by the scalar
	**/
	inline Vec4 operator*(const T& scalar) const
	{
		return Vec4(x * scalar, y * scalar, z * scalar, w * scalar);
	}

	/**
	* Divide each component by a scalar and return the result
	*
	* @param  scalar The scalar to divide with
	* @return The result of dividing the vector
	**/
	inline Vec4 operator/(const T& scalar) const
	{
		return Vec4(x / scalar, y / scalar, z / scalar, w / scalar);
	}

	/**
	* Add a vector to this vector and return the result
	*
	* @param  rhs The vector to add to this
	* @return The result of adding the two vectors
	**/
	inline Vec4 operator+(const Vec4& rhs) const
	{
		return Vec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
	}

	/**
	* Subtract a vector from this vector and return the result
	*
	* @param  rhs The vector to take away from this
	* @return The result of the subtraction
	**/
	inline Vec4 operator-(const Vec4& rhs) const
	{
		return Vec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
	}

	/**
	* Negate this vector and return the result
	*
	* @return The negated vector
	**/
	inline Vec4 operator-() const
	{
		return Vec4(-x, -y, -z, -w);
	}

	/**
	* Component multiply two vectors and return the result
	*
	* @param  lhs The left vector
	* @param  rhs The right vector
	* @return The result of the component multiplication
	**/
	inline static Vec4 CompMul(const Vec4& lhs, const Vec4& rhs)
	{
		return Vec4(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
	}

	/**
	* Component divide two vectors and return the result
	*
	* @param  lhs The left vector
	* @param  rhs The right vector
	* @return The result of the component division
	**/
	inline static Vec4 CompDiv(const Vec4& lhs, const Vec4& rhs)
	{
		return Vec4(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w);
	}

	/**
	* Take the 3D dot product of two 4D vectors and return the result
	*
	* @param  lhs The left vector
	* @param  rhs The right vector
	* @return The dot of the two vectors
	**/
	inline static T Dot3(const Vec4& lhs, const Vec4& rhs)
	{
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
	}

	/**
	* Take the 4D dot product of two 4D vectors and return the result
	*
	* @param  lhs The left vector
	* @param  rhs The right vector
	* @return The dot of the two vectors
	**/
	inline static T Dot(const Vec4& lhs, const Vec4& rhs)
	{
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
	}

	/**
	* Calculate the 3D cross product of two vectors (the w component is ignored).
	*
	* @param  lhs The left vector
	* @param  rhs The right vector
	* @return The cross product of the two vectors
	**/
	inline static Vec4 Cross(const Vec4& lhs, const Vec4& rhs)
	{
		return Vec4(
			lhs.y * rhs.z - lhs.z * rhs.y,
			lhs.z * rhs.x - lhs.x * rhs.z,
			lhs.x * rhs.y - lhs.y * rhs.x,
			0.0f
		);
	}

	/**
	* Compare two vectors for exact equality
	*
	* @param  rhs The other vector to compare against
	* @return True if the vectors are exactly equal
	**/
	inline bool operator==(const Vec4& rhs) const
	{
		return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
	}

	/**
	* Compare two vectors for inequality
	*
	* @param  rhs The other vector to compare against
	* @return True if the vectors are not equal
	**/
	inline bool operator!=(const Vec4& rhs) const
	{
		return x != rhs.x && y != rhs.y && z != rhs.z && w != rhs.w;
	}

	/**
	* Add a scalar to this vector, modifying this vector
	*
	* @param  scalar The scalar to add
	* @return This vector after the addition
	**/
	inline Vec4 operator+=(const T& scalar)
	{
		x += scalar;
		y += scalar;
		z += scalar;
		w += scalar;
		return *this;
	}

	/**
	* Subtract a scalar to this vector, modifying this vector
	*
	* @param  scalar The scalar to subtract
	* @return This vector after the subtraction
	**/
	inline Vec4 operator-=(const T& scalar)
	{
		x -= scalar;
		y -= scalar;
		z -= scalar;
		w -= scalar;
		return *this;
	}

	/**
	* Multiply this vector by a scalar, modifying this vector
	*
	* @param  scalar The scalar to multiply by
	* @return This vector after the multiplication
	**/
	inline Vec4 operator*=(const T& scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		w *= scalar;
		return *this;
	}

	/**
	* Divide this vector by a scalar, modifying this vector
	*
	* @param  scalar The scalar to divide by
	* @return This vector after the division
	**/
	inline Vec4 operator/=(const T& scalar)
	{
		x /= scalar;
		y /= scalar;
		z /= scalar;
		w /= scalar;
		return *this;
	}

	/**
	* Add another vector to this vector, modifying this vector
	*
	* @param  rhs The vector to to add
	* @return This vector after the addition
	**/
	inline Vec4 operator+=(const Vec4& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		w += rhs.w;
		return *this;
	}

	/**
	* Subtract another vector from this vector, modifying this vector
	*
	* @param  rhs The vector to to subtract
	* @return This vector after the subtraction
	**/
	inline Vec4 operator-=(const Vec4& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		w -= rhs.w;
		return *this;
	}

	/**
	* Get the length of this vector
	*
	* @return The length
	**/
	inline T GetLength() const
	{
		return sqrt(x * x + y * y + z * z + w * w);
	}

	/**
	* Get a normalized version of this vector
	*
	* @return The normalized vector
	**/
	inline Vec4 GetNormalized() const
	{
		return Vec4(x, y, z, w) / GetLength();
	}

	/**
	* Get a reference to an element of this vector by index
	*
	* @param  index The index to query
	* @return A reference to that element
	**/
	inline T& operator[](int index)
	{
		//ASSERT(index < 4, "Out of bounds index for vector component");
		return (&x)[index];
	}

	/**
	* Get a copy of an element of this vector by index
	*
	* @param  index The index to query
	* @return A copy of that element
	**/
	inline T operator[](int index) const
	{
		//ASSERT(index < 4, "Out of bounds index for vector component");
		return (&x)[index];
	}

	/**
	* Get a string representation of this vector (useful for debugging)
	*
	* @return The string
	**/
	//inline String ToString() const
	//{
	//	//eastl::string str;
	//	return ""; //str.sprintf("{ %.5f, %.5f, %.5f, %.5f }", x, y, z, w);
	//}

	/**
	* Embeds a 3D vector into 4D space
	*
	* @param  vec The target 3D vector
	* @return The 4D vector
	**/
	inline static Vec4 Embed3D(const Vec3<T>& vec)
	{
		return Vec4(vec.x, vec.y, vec.z, 1.0f);
	}

	/**
	* Embeds a 2D vector into 4D space
	*
	* @param  vec The target 3D vector
	* @return The 4D vector
	**/
	inline static Vec4 Embed2D(const Vec2<T>& vec)
	{
		return Vec4(vec.x, vec.y, 1.0f, 1.0f);
	}
};

typedef Vec4<float> Vec4f;
typedef Vec4<double> Vec4d;