// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include <math.h>

template<typename T>
struct Vec4;

template<typename T>
struct Vec2;

template<typename T>
struct Vec3
{
	//REFLECT()

	Vec3() : x(T()), y(T()), z(T()) {}
	Vec3(T val) : x(val), y(val), z(val) {}
	Vec3(T x, T y, T z) : x(x), y(y), z(z) {}

	T x;
	T y;
	T z;

	/**
	* Add a scalar to each component of this vector and return the result
	*
	* @param  scalar The scalar to add
	* @return The result of adding the scalar
	**/
	inline Vec3 operator+(const T& scalar) const
	{
		return Vec3(x + scalar, y + scalar, z + scalar);
	}

	/**
	* Subtract a scalar from this vector and return the result
	*
	* @param  scalar The scalar to subtract
	* @return The result of subtracting the scalar
	**/
	inline Vec3 operator-(const T& scalar) const
	{
		return Vec3(x - scalar, y - scalar, z - scalar);
	}

	/**
	* Multiply each component by this vector and return the result
	*
	* @param  scalar The scalar to multiply
	* @return The result of multiplying by the scalar
	**/
	inline Vec3 operator*(const T& scalar) const
	{
		return Vec3(x * scalar, y * scalar, z * scalar);
	}

	/**
	* Divide each component by a scalar and return the result
	*
	* @param  scalar The scalar to divide with
	* @return The result of dividing the vector
	**/
	inline Vec3 operator/(const T& scalar) const
	{
		return Vec3(x / scalar, y / scalar, z / scalar);
	}

	/**
	* Add a vector to this vector and return the result
	*
	* @param  rhs The vector to add to this
	* @return The result of adding the two vectors
	**/
	inline Vec3 operator+(const Vec3& rhs) const
	{
		return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
	}

	/**
	* Subtract a vector from this vector and return the result
	*
	* @param  rhs The vector to take away from this
	* @return The result of the subtraction
	**/
	inline Vec3 operator-(const Vec3& rhs) const
	{
		return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
	}

	/**
	* Negate this vector and return the result
	*
	* @return The negated vector
	**/
	inline Vec3 operator-() const
	{
		return Vec3(-x, -y, -z);
	}

	/**
	* Component multiply two vectors and return the result
	*
	* @param  lhs The left vector
	* @param  rhs The right vector
	* @return The result of the component multiplication
	**/
	inline static Vec3 CompMul(const Vec3& lhs, const Vec3& rhs)
	{
		return Vec3(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
	}

	/**
	* Component divide two vectors and return the result
	*
	* @param  lhs The left vector
	* @param  rhs The right vector
	* @return The result of the component division
	**/
	inline static Vec3 CompDiv(const Vec3& lhs, const Vec3& rhs)
	{
		return Vec3(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z);
	}

	/**
	* Take the dot product of two vectors and return the result
	*
	* @param  lhs The left vector
	* @param  rhs The right vector
	* @return The dot of the two vectors
	**/
	inline static T Dot(const Vec3& lhs, const Vec3& rhs)
	{
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
	}

	/**
	* Calculate the cross product of two vectors. 
	*
	* @param  lhs The left vector
	* @param  rhs The right vector
	* @return The cross product of the two vectors
	**/
	inline static Vec3 Cross(const Vec3& lhs, const Vec3& rhs)
	{
		return Vec3(
			lhs.y * rhs.z - lhs.z * rhs.y,
			lhs.z * rhs.x - lhs.x * rhs.z,
			lhs.x * rhs.y - lhs.y * rhs.x
			);
	}

	/**
	* Determine if two vectors are almost the same
	*
	* @param  lhs The left vector
	* @param  rhs The right vector
	* @return true or false, are they equivalent
	**/
	inline static bool IsEquivalent(const Vec3& lhs, const Vec3& rhs)
	{
		return (lhs - rhs).GetLengthSquared() < 0.003f;
	}

	/**
	* Compare two vectors for exact equality
	*
	* @param  rhs The other vector to compare against
	* @return True if the vectors are exactly equal
	**/
	inline bool operator==(const Vec3& rhs) const
	{
		return x == rhs.x && y == rhs.y && z == rhs.z;
	}

	/**
	* Compare two vectors for inequality
	*
	* @param  rhs The other vector to compare against
	* @return True if the vectors are not equal
	**/
	inline bool operator!=(const Vec3& rhs) const
	{
		return x != rhs.x && y != rhs.y && z != rhs.z;
	}

	/**
	* Add a scalar to this vector, modifying this vector
	*
	* @param  scalar The scalar to add
	* @return This vector after the addition
	**/
	inline Vec3 operator+=(const T& scalar)
	{
		x += scalar;
		y += scalar;
		z += scalar;
		return *this;
	}

	/**
	* Subtract a scalar to this vector, modifying this vector
	*
	* @param  scalar The scalar to subtract
	* @return This vector after the subtraction
	**/
	inline Vec3 operator-=(const T& scalar)
	{
		x -= scalar;
		y -= scalar;
		z -= scalar;
		return *this;
	}

	/**
	* Multiply this vector by a scalar, modifying this vector
	*
	* @param  scalar The scalar to multiply by
	* @return This vector after the multiplication
	**/
	inline Vec3 operator*=(const T& scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		return *this;
	}

	/**
	* Divide this vector by a scalar, modifying this vector
	*
	* @param  scalar The scalar to divide by
	* @return This vector after the division
	**/
	inline Vec3 operator/=(const T& scalar)
	{
		x /= scalar;
		y /= scalar;
		z /= scalar;
		return *this;
	}

	/**
	* Add another vector to this vector, modifying this vector
	*
	* @param  rhs The vector to to add
	* @return This vector after the addition
	**/
	inline Vec3 operator+=(const Vec3& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}

	/**
	* Subtract another vector from this vector, modifying this vector
	*
	* @param  rhs The vector to to subtract
	* @return This vector after the subtraction
	**/
	inline Vec3 operator-=(const Vec3& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		return *this;
	}

	/**
	* Get the length of this vector
	*
	* @return The length
	**/
	inline T GetLength() const
	{
		return sqrtf(x * x + y * y + z * z);
	}

	/**
	* Get the length of this vector squared
	*
	* @return The length squared
	**/
	inline T GetLengthSquared() const
	{
		return x * x + y * y + z * z;
	}

	/**
	* Get a normalized version of this vector
	*
	* @return The normalized vector
	**/
	inline Vec3 GetNormalized() const
	{
		return Vec3(x, y, z) / GetLength();
	}

	/**
	* Get a reference to an element of this vector by index
	*
	* @param  index The index to query
	* @return A reference to that element
	**/
	inline T& operator[](int index)
	{
		//ASSERT(index < 3, "Out of bounds index for vector component");
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
		//ASSERT(index < 3, "Out of bounds index for vector component");
		return (&x)[index];
	}

	/**
	* Get a string representation of this vector (useful for debugging)
	*
	* @return The string
	**/
	//inline String ToString() const
	//{
	//	// Replace with c++ format
	//	// eastl::string str;
	//	return ""; //str.sprintf("{ %.5f, %.5f, %.5f }", x, y, z);
	//}

	/**
	* Projects a 4D vector into 3D space
	*
	* @param  vec The target 4D vector
	* @return The 3D vector
	**/
	inline static Vec3 Project4D(const Vec4<T>& vec)
	{
		return Vec3(vec.x, vec.y, vec.z);
	}

	/**
	* Embeds a 2D vector into 3D space
	*
	* @param  vec The target 2D vector
	* @param  z the desired z component
	* @return The 3D vector
	**/
	inline static Vec3 Embed2D(const Vec2<T>& vec, T _z = T())
	{
		return Vec3(vec.x, vec.y, _z);
	}
};

typedef Vec3<float> Vec3f;
typedef Vec3<double> Vec3d;