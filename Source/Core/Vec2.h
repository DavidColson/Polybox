// Copyright 2020-2022 David Colson. All rights reserved.

#pragma once

#include <string>

template<typename T>
struct Vec3;

template<typename T>
struct Vec4;

template<typename T>
struct Vec2
{
	//REFLECT()

	Vec2() : x(T()), y(T()) {}
	Vec2(T val) : x(val), y(val) {}
	Vec2(T x, T y) : x(x), y(y) {}

	T x;
	T y;

	/**
	* Add a scalar to each component of this vector and return the result
	*
	* @param  scalar The scalar to add
	* @return The result of adding the scalar
	**/
	inline Vec2 operator+(const T& scalar) const
	{
		return Vec2(x + scalar, y + scalar);
	}

	/**
	* Subtract a scalar from this vector and return the result
	*
	* @param  scalar The scalar to subtract
	* @return The result of subtracting the scalar
	**/
	inline Vec2 operator-(const T& scalar) const
	{
		return Vec2(x - scalar, y - scalar);
	}

	/**
	* Multiply each component by this vector and return the result
	*
	* @param  scalar The scalar to multiply
	* @return The result of multiplying by the scalar
	**/
	inline Vec2 operator*(const T& scalar) const
	{
		return Vec2(x * scalar, y * scalar);
	}

	/**
	* Divide each component by a scalar and return the result
	*
	* @param  scalar The scalar to divide with
	* @return The result of dividing the vector
	**/
	inline Vec2 operator/(const T& scalar) const
	{
		return Vec2(x / scalar, y / scalar);
	}

	/**
	* Add a vector to this vector and return the result
	*
	* @param  rhs The vector to add to this
	* @return The result of adding the two vectors
	**/
	inline Vec2 operator+(const Vec2& rhs) const
	{
		return Vec2(x + rhs.x, y + rhs.y);
	}

	/**
	* Subtract a vector from this vector and return the result
	*
	* @param  rhs The vector to take away from this
	* @return The result of the subtraction
	**/
	inline Vec2 operator-(const Vec2& rhs) const
	{
		return Vec2(x - rhs.x, y - rhs.y);
	}

	/**
	* Negate this vector and return the result
	*
	* @return The negated vector
	**/
	inline Vec2 operator-() const
	{
		return Vec2(-x, -y);
	}

	/**
	* Component multiply two vectors and return the result
	*
	* @param  lhs The left vector
	* @param  rhs The right vector
	* @return The result of the component multiplication
	**/
	inline static Vec2 CompMul(const Vec2& lhs, const Vec2& rhs)
	{
		return Vec2(lhs.x * rhs.x, lhs.y * rhs.y);
	}

	/**
	* Component divide two vectors and return the result
	*
	* @param  lhs The left vector
	* @param  rhs The right vector
	* @return The result of the component division
	**/
	inline static Vec2 CompDiv(const Vec2& lhs, const Vec2& rhs)
	{
		return Vec2(lhs.x / rhs.x, lhs.y / rhs.y);
	}

	/**
	* Take the dot product of two vectors and return the result
	*
	* @param  lhs The left vector
	* @param  rhs The right vector
	* @return The dot of the two vectors
	**/
	inline static T Dot(const Vec2& lhs, const Vec2& rhs)
	{
		return lhs.x * rhs.x + lhs.y * rhs.y;
	}

	/**
	* Calculate the cross product of two vectors. Note this is in 2D, so returns the magnitude of what would
	* have been the Z axis coming out of the X-Y plane
	*
	* @param  lhs The left vector
	* @param  rhs The right vector
	* @return The cross product of the two vectors
	**/
	inline static float Cross(const Vec2& lhs, const Vec2& rhs)
	{
		return lhs.x * rhs.y - lhs.y * rhs.x;
	}

	/**
	* Compare two vectors for exact equality
	*
	* @param  rhs The other vector to compare against
	* @return True if the vectors are exactly equal
	**/
	inline bool operator==(const Vec2& rhs) const
	{
		return x == rhs.x && y == rhs.y;
	}

	/**
	* Compare two vectors for inequality
	*
	* @param  rhs The other vector to compare against
	* @return True if the vectors are not equal
	**/
	inline bool operator!=(const Vec2& rhs) const
	{
		return x != rhs.x || y != rhs.y;
	}

	/**
	* Add a scalar to this vector, modifying this vector
	* 
	* @param  scalar The scalar to add
	* @return This vector after the addition
	**/
	inline Vec2 operator+=(const T& scalar)
	{
		x += scalar;
		y += scalar;
		return *this;
	}

	/**
	* Subtract a scalar to this vector, modifying this vector
	*
	* @param  scalar The scalar to subtract
	* @return This vector after the subtraction
	**/
	inline Vec2 operator-=(const T& scalar)
	{
		x -= scalar;
		y -= scalar;
		return *this;
	}

	/**
	* Multiply this vector by a scalar, modifying this vector
	*
	* @param  scalar The scalar to multiply by
	* @return This vector after the multiplication
	**/
	inline Vec2 operator*=(const T& scalar)
	{
		x *= scalar;
		y *= scalar;
		return *this;
	}

	/**
	* Divide this vector by a scalar, modifying this vector
	*
	* @param  scalar The scalar to divide by
	* @return This vector after the division
	**/
	inline Vec2 operator/=(const T& scalar)
	{
		x /= scalar;
		y /= scalar;
		return *this;
	}

	/**
	* Add another vector to this vector, modifying this vector
	*
	* @param  rhs The vector to to add
	* @return This vector after the addition
	**/
	inline Vec2 operator+=(const Vec2& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	/**
	* Subtract another vector from this vector, modifying this vector
	*
	* @param  rhs The vector to to subtract
	* @return This vector after the subtraction
	**/
	inline Vec2 operator-=(const Vec2& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}
	
	/**
	* Get the length of this vector
	*
	* @return The length
	**/
	inline T GetLength() const
	{
		return (T)sqrt(x * x + y * y);
	}

	/**
	* Get a normalized version of this vector
	*
	* @return The normalized vector
	**/
	inline Vec2 GetNormalized() const
	{
		return Vec2(x, y) / GetLength();
	}

	/**
	* Get a reference to an element of this vector by index
	*
	* @param  index The index to query
	* @return A reference to that element
	**/
	inline T& operator[](int index) 
	{
		//ASSERT(index < 2, "Out of bounds index for vector component");
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
		//ASSERT(index < 2, "Out of bounds index for vector component");
		return (&x)[index];
	}

	/**
	* Get a string representation of this vector (useful for debugging)
	*
	* @return The string
	**/
	inline std::string ToString() const
	{
		//eastl::string str;
		return ""; //str.sprintf("{ %.5f, %.5f }", x, y);
	}

	/**
	* Projects a 4D vector into 2D space
	*
	* @param  vec The target 4D vector
	* @return The 2D vector
	**/
	inline static Vec2 Project4D(const Vec4<T>& vec)
	{
		return Vec2(vec.x, vec.y);
	}

	/**
	* Projects a 3D vector into 2D space
	*
	* @param  vec The target 3D vector
	* @return The 2D vector
	**/
	inline static Vec2 Project3D(const Vec3<T>& vec)
	{
		return Vec2(vec.x, vec.y);
	}
};

typedef Vec2<int> Vec2i;
typedef Vec2<float> Vec2f;
typedef Vec2<double> Vec2d;