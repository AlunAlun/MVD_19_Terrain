//
//  linmath.h
//
//  Copyright © 2018 Alun Evans. All rights reserved.
//
#pragma once
#include <cmath> //for sqrt (square root) function
#define DEG2RAD 0.0174532925f

namespace lm {

	class vec2
	{
	public:
		union {
			struct { float x, y; };
			float value_[2];
		};

		vec2() { x = y = 0.0f; }
		vec2(float x, float y) { this->x = x; this->y = y; }

		float length() const { return sqrt(x*x + y*y); };
		vec2& normalize() { *this *= 1.0f / length(); return *this; }; //both normalizes object and return reference to it


		float distance(const vec2& v) const;
		float dot(const vec2& v) const;
		vec2 lerp(const vec2& v, float percent) const;

		void operator *= (float v) { x *= v; y *= v; }
	};

	class vec3
	{
	public:
		union {
			struct { float x, y, z; };
			struct { float r, g, b; };
			float value_[3];
		};

		vec3() { x = y = z = 0.0f; }
		vec3(float x, float y, float z) { this->x = x; this->y = y; this->z = z; }

		float length() const { return sqrt(x*x + y*y + z*z); };
		vec3& normalize() { *this *= (1.0f / length()); return *this; }; //both normalizes object and return reference to it

		float distance(const vec3& v) const;
		float dot(const vec3& v) const;
		vec3 cross(const vec3& v) const;
		vec3 lerp(const vec3& v, float percent) const;

		void operator *= (float v) { x *= v; y *= v; z *= v; }
	};

	// we only really use a vec4 for special cases using homogenous coordinates
	// so the class is much restricted compared to vec2 and vec3
	// note: vector is initialised with w set to 1;
	class vec4
	{
	public:
		union {
			struct { float x, y, z, w; };
			float value_[4];
		};

		vec4() { x = y = z = 0.0f; w = 1.0f; }
		vec4(float x, float y, float z, float w) { this->x = x; this->y = y; this->z = z; this->w = w; }
		

		vec4& normalize(); //divides 3 component vector by w and returns reference

		void operator *= (float v) { x *= v; y *= v; y *= v; w *= v; }
	};

	class quat
	{
	public:
		union {
			struct { float w, x, y, z; };
			float value_[4];
		};
		quat(float angle, vec3 axis);
		quat(float x, float y, float z);
		quat() { w = 1.0f; x = y = z = 0.0f; }
		quat(float w, float x, float y, float z) { this->w = w;		this->x = x; this->y = y; this->z = z; }
		
		
		float length() const { return sqrt(w*w + x*x + y*y + z*z); };
		quat conjugate() const { return quat(w, -x, -y, -z); };
		quat inverse() const;

		quat &normalize() { *this *= (1.0f / length()); return *this; }; //both normalizes object and return reference to it

		void operator *= (float v) { w *= v; x *= v; y *= v; y *= v;  }
	};

	class mat4 {
	public:
		// OpenGL and GLSL by default accept matrices in column-major format.
		// However, we are used to writing matrices in row-major format.
		// For example, take the translation matrix, which translates a 3D
		// point in X, Y, and Z. We usually write it like this:
		// 1 0 0 X
		// 0 1 0 Y
		// 0 0 1 Z
		// 0 0 0 1
		// OpenGL expects us to provide the matrix as array of floats
		// *ordered by columns*:
		// [1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  X, Y, Z, 1]
		union {
			float M[4][4]; //represent [column][row] of row-major matrix
			float m[16];
		};

		mat4();
		mat4(const float* v);

		//sets values of this matrix to parameter. Useful for classes which
		//inherit this class
		void set(mat4 m);

		mat4& clear();
		mat4& setIdentity();
		mat4& transpose();
		bool inverse();

		//get base vectors
		vec3 right() const { return vec3(m[0], m[1], m[2]); }
		vec3 top() const { return vec3(m[4], m[5], m[6]); }
		vec3 front() const { return vec3(m[8], m[9], m[10]); }
		vec3 position() const { return vec3(m[12], m[13], m[14]); }

		//functions that set base vectors, and orthogonalise matrix if required
		void front(float x, float y, float z);
		void front(vec3 f); //param not const as it will be normalised in function
		void position(float x, float y, float z) { m[12] = x; m[13] = y; m[14] = z; }
		void position(const vec3& p) { m[12] = p.x; m[13] = p.y; m[14] = p.z; }

		//make new transformation matrices
		void makeTranslationMatrix(float x, float y, float z);
		void makeTranslationMatrix(const vec3& t);
		void makeRotationMatrix(float angle_in_rad, const vec3& axis);
		void makeRotationMatrix(const quat& normalized_quat);
		void makeScaleMatrix(float x, float y, float z);
		void makeScaleMatrix(const vec3& t);

		//transform this matrix using world coordinates
		void translate(float x, float y, float z);
		void translate(const vec3& t);
		void rotate(float angle_in_rad, const vec3& axis);
		void scale(float x, float y, float z);
		void scale(const vec3& s);

		//transform this matrix using local coordinates
		void translateLocal(float x, float y, float z);
		void rotateLocal(float angle_in_rad, const vec3& axis);
		void scaleLocal(float x, float y, float z);

		//for cameras
	    void lookAt(const vec3& eye, const vec3& center, const vec3& up);
	    void perspective(float fov_rad, float aspect, float near_plane, float far_plane);
	    void orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane);

		vec3 operator * (const vec3& vec) const;
		vec4 operator * (const vec4& vec) const;
		mat4 operator * (const mat4& matrix) const;

	private:
		void orthogonalizeFromFront();
	};

	//vec2 operators
	vec2 operator * (const vec2& a, float v);
	vec2 operator + (const vec2& a, const vec2& b);
	vec2 operator - (const vec2& a, const vec2& b);
	//vec3 operators
	vec3 operator + (const vec3& a, const vec3& b);
	vec3 operator - (const vec3& a, const vec3& b);
	vec3 operator * (const vec3& a, float v);
	//vec4 operators
	vec4 operator + (const vec4& a, const vec4& b);
	vec4 operator - (const vec4& a, const vec4& b);
	vec4 operator * (const vec4& a, float v);
	//quat operators
	quat operator + (const quat& a, const quat& b);
	quat operator - (const quat& a, const quat& b);
	quat operator * (const quat& a, float v);
	quat operator * (const quat& a, const quat& b);

}