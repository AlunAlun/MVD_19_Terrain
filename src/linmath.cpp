//
//  linmath.cpp
//  A1-LinearMath
//
//  Copyright © 2018 Alun Evans. All rights reserved.
//
#include "linmath.h"
#include <math.h> //atan2
#include <utility> //for std::swap

namespace lm {

	//**************************************
	// vec2
	//**************************************

	float vec2::distance(const vec2& v) const
	{
		return float((v - *this).length());
	}

	float vec2::dot(const vec2& v) const
	{
		return x * v.x + y * v.y;
	}

	vec2 vec2::lerp(const vec2& end, float percent) const
	{
		return (*this + ((end - *this)*percent));
	}

	vec2 operator * (const vec2& a, float v) { return vec2(a.x * v, a.y * v); }
	vec2 operator + (const vec2& a, const vec2& b) { return vec2(a.x + b.x, a.y + b.y); }
	vec2 operator - (const vec2& a, const vec2& b) { return vec2(a.x - b.x, a.y - b.y); }

	//**************************************
	// vec3
	//**************************************

	float vec3::distance(const vec3& v) const
	{
		return float((v - *this).length());
	}

	vec3 vec3::cross(const vec3& b) const
	{
		return vec3(y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x);
	}

	float vec3::dot(const vec3& v) const
	{
		return x*v.x + y*v.y + z*v.z;
	}

	vec3 vec3::lerp(const vec3& end, float percent) const
	{
		return (*this + ((end - *this)*percent));
	}

	vec3 operator + (const vec3& a, const vec3& b) { return vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
	vec3 operator - (const vec3& a, const vec3& b) { return vec3(a.x - b.x, a.y - b.y, a.z - b.z); }
	vec3 operator * (const vec3& a, float v) { return vec3(a.x * v, a.y * v, a.z * v); }

	//**************************************
	// vec4
	//**************************************

	// Divides vector by value of 4th component
	vec4& vec4::normalize()
	{
		x = float(x / w);
		y = float(y / w);
		z = float(z / w);
		w = 1.0f;
		return *this;
	}
	vec4 operator + (const vec4& a, const vec4& b) { return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
	vec4 operator - (const vec4& a, const vec4& b) { return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
	vec4 operator * (const vec4& a, float v) { return vec4(a.x * v, a.y * v, a.z * v, a.w * v); }

	//**************************************
	// quat
	//**************************************

	//loads of useful code here:
	//http://www.ncsa.illinois.edu/People/kindr/emtc/quaternions/quaternion.c++

	//creates a quaternion from an angle and an axis
	quat::quat(float angle, vec3 axis) {
		w = cosf(angle / 2.0f);
		x = axis.x * sinf(angle / 2.0f);
		y = axis.y * sinf(angle / 2.0f);
		z = axis.z * sinf(angle / 2.0f);
		normalize();
	}

	//creates new quaternion from euler angles (x:pitch
	quat::quat(float x, float y, float z) {
		float sx = sinf(x * 0.5f);
		float sy = sinf(y * 0.5f);
		float sz = sinf(z * 0.5f);
		float cx = cosf(x * 0.5f);
		float cy = cosf(y * 0.5f);
		float cz = cosf(z * 0.5f);
		
		this->w = cx * cy * cz + sx * sy * sz;
		this->x = sx * cy * cz - cx * sy * sz;
		this->y = cx * sy * cz + sx * cy * sz;
		this->z = cx * cy * sz - sx * sy * cz;
	}

	quat quat::inverse() const {
		float norm = w*w + x*x + y*y + z*z;
		return (*this).conjugate() * (1/norm);
	}

	quat operator + (const quat& a, const quat& b) { return quat(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
	quat operator - (const quat& a, const quat& b) { return quat(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
	quat operator * (const quat& a, float v) { return quat(a.x * v, a.y * v, a.z * v, a.w * v); }
	quat operator * (const quat& a, const quat& b) {
		return quat(
			a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
			a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
			a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
			a.w * b.z + a.x  *b.y - a.y * b.x + a.z * b.w
		);
	}

	//**************************************
	// mat4
	//**************************************
	mat4::mat4()
	{
		setIdentity();
	}

	void mat4::set(mat4 m) {
		for (int i = 0; i < 16; i++) (*this).m[i] = m.m[i];
	}

	mat4& mat4::clear()
	{
		m[0] = 0; m[4] = 0; m[8] = 0; m[12] = 0;
		m[1] = 0; m[5] = 0; m[9] = 0; m[13] = 0;
		m[2] = 0; m[6] = 0; m[10] = 0; m[14] = 0;
		m[3] = 0; m[7] = 0; m[11] = 0; m[15] = 0;
		return *this;
	}

	mat4& mat4::setIdentity()
	{
		m[0] = 1; m[4] = 0; m[8] = 0; m[12] = 0;
		m[1] = 0; m[5] = 1; m[9] = 0; m[13] = 0;
		m[2] = 0; m[6] = 0; m[10] = 1; m[14] = 0;
		m[3] = 0; m[7] = 0; m[11] = 0; m[15] = 1;
		return *this;
	}

	mat4& mat4::transpose()
	{
		std::swap(m[1], m[4]); std::swap(m[2], m[8]); std::swap(m[3], m[12]);
		std::swap(m[6], m[9]); std::swap(m[7], m[13]); std::swap(m[11], m[14]);
		return *this;
	}

	bool mat4::inverse()
	{
		unsigned int i, j, k, swap;
		float t;
		mat4 temp, final;
		final.setIdentity();

		temp = (*this);

		unsigned int m, n;
		m = n = 4;

		for (i = 0; i < m; i++)
		{
			// Look for largest element in column
			swap = i;
			for (j = i + 1; j < m; j++)// m or n
			{
				if (fabs(temp.M[j][i]) > fabs(temp.M[swap][i]))
					swap = j;
			}

			if (swap != i)
			{
				// Swap rows.
				for (k = 0; k < n; k++)
				{
					std::swap(temp.M[i][k], temp.M[swap][k]);
					std::swap(final.M[i][k], final.M[swap][k]);
				}
			}
			// No non-zero pivot.  The CMatrix is singular, which shouldn't
			// happen.  This means the user gave us a bad CMatrix.
#define MATRIX_SINGULAR_THRESHOLD 0.00001 //change this if you experience problems with matrices
			if (fabsf(temp.M[i][i]) <= MATRIX_SINGULAR_THRESHOLD)
			{
				final.setIdentity();
				return false;
			}
#undef MATRIX_SINGULAR_THRESHOLD
			t = 1.0f / temp.M[i][i];
			for (k = 0; k < n; k++)//m or n
			{
				temp.M[i][k] *= t;
				final.M[i][k] *= t;
			}
			for (j = 0; j < m; j++) // m or n
			{
				if (j != i)
				{
					t = temp.M[j][i];
					for (k = 0; k < n; k++)//m or n
					{
						temp.M[j][k] -= (temp.M[i][k] * t);
						final.M[j][k] -= (final.M[i][k] * t);
					}
				}
			}
		}
		*this = final;

		return true;
	}

	// orthogonalizes right and top vector from the front vector
	// assumes new, normalized front vector has just been set
	void mat4::orthogonalizeFromFront() {

		vec3 f = this->front();

		vec3 r, t;
		r = this->right();

		//if right vector the same direction as new front vector
		//new right vector is cross of top and new front, then set
		//up to cross new right and new front
		if (abs(r.dot(f)) < 0.99998)
		{
			r = this->top().cross(f);
			t = f.cross(r);
		}
		else //do the same using top first
		{
			t = f.cross(r);
			r = t.cross(f);
		}

		r.normalize();
		t.normalize();

		m[4] = t.x;
		m[5] = t.y;
		m[6] = t.z;

		m[0] = r.x;
		m[1] = r.y;
		m[2] = r.z;
	}

	// sets new front vector and makes top and right orthogonal
	void mat4::front(float x, float y, float z) {
		vec3 f(x, y, z);
		f.normalize();
		//put the front vector in the matrix
		m[8] = f.x;
		m[9] = f.y;
		m[10] = f.z;

		orthogonalizeFromFront();

	}

	// sets new front vector and makes top and right orthogonal
	void mat4::front(vec3 f) {
		f.normalize();
		//put the front vector in the matrix
		m[8] = f.x;
		m[9] = f.y;
		m[10] = f.z;

		orthogonalizeFromFront();
	}

	// sets the values of this matrix to a translation matrix
	// representing the 3 components of the parameters
	void mat4::makeTranslationMatrix(float x, float y, float z)
	{
		setIdentity();
		m[12] = x;
		m[13] = y;
		m[14] = z;
	}

	// sets the values of this matrix to a translation matrix
	// representing the 3 components of the parameter
	void mat4::makeTranslationMatrix(const vec3& t)
	{
		setIdentity();
		m[12] = t.x;
		m[13] = t.y;
		m[14] = t.z;
	}

	// sets the values of this matrix to a rotation matrix
	// representing a rotation around the angle and axis of the parameters
	// see https://www.siggraph.org/education/materials/HyperGraph/modeling/mod_tran/3drota.htm
	void mat4::makeRotationMatrix(float angle_in_rad, const vec3& axis) {
		clear();
		vec3 axis_n = axis;
		axis_n.normalize();

		float c = cos(angle_in_rad);
		float s = sin(angle_in_rad);
		float t = 1 - c;

		M[0][0] = t * axis_n.x * axis_n.x + c;
		M[0][1] = t * axis_n.x * axis_n.y - s * axis_n.z;
		M[0][2] = t * axis_n.x * axis_n.z + s * axis_n.y;

		M[1][0] = t * axis_n.x * axis_n.y + s * axis_n.z;
		M[1][1] = t * axis_n.y * axis_n.y + c;
		M[1][2] = t * axis_n.y * axis_n.z - s * axis_n.x;

		M[2][0] = t * axis_n.x * axis_n.z - s * axis_n.y;
		M[2][1] = t * axis_n.y * axis_n.z + s * axis_n.x;
		M[2][2] = t * axis_n.z * axis_n.z + c;

		M[3][3] = 1.0f;
	}

	//
	//see https://www.cprogramming.com/tutorial/3d/quaternions.html
	void mat4::makeRotationMatrix(const quat& normalized_quat) {
		clear();

		float w, x, y, z; 
		w = normalized_quat.w;
		x = normalized_quat.x;
		y = normalized_quat.y;
		z = normalized_quat.z;

		m[0] = 1 - 2 * y*y - 2 * z*z;
		m[1] = 2 * x*y + 2 * w*z;
		m[2] = 2 * x*z - 2 * w*y;
		m[3] = 0;

		m[4] = 2 * x*y - 2 * w*z;
		m[5] = 1 - 2 * x*x - 2 * z*z;
		m[6] = 2 * y*z + 2 * w*x;
		m[7] = 0;

		m[8] = 2 * x*z + 2 * w*y;
		m[9] = 2 * y*z - 2 * w*x;
		m[10] = 1 - 2 * x*x - 2 * y*y;
		m[11] = 0;

		m[12] = m[13] = m[14] = 0; m[15] = 1;
	}

	// sets the values of this matrix to a scale matrix
	// representing the 3 components of the parameters
	void mat4::makeScaleMatrix(float x, float y, float z) {
		setIdentity();
		m[0] = x;
		m[5] = y;
		m[10] = z;
	}

	// sets the values of this matrix to a translation matrix
	// representing the 3 components of the parameters
	void mat4::makeScaleMatrix(const vec3& t) {
		setIdentity();
		m[0] = t.x;
		m[5] = t.y;
		m[10] = t.z;
	}

	// translates this matrix by the three components
	void mat4::translate(float x, float y, float z)
	{
		mat4 T;
		T.makeTranslationMatrix(x, y, z);
		*this = T * *this;
	}

	// translates this matrix by the three components
	void mat4::translate(const vec3& t)
	{
		mat4 T;
		T.makeTranslationMatrix(t.x, t.y, t.z);
		*this = T * *this;
	}

	// rotates this matrix around by the an angle, along an axis
	void mat4::rotate(float angle_in_rad, const vec3& axis)
	{
		mat4 R;
		R.makeRotationMatrix(angle_in_rad, axis);
		*this = R * *this;
	}

	// scales this matrix by the three components
	void mat4::scale(float x, float y, float z)
	{
		mat4 S;
		S.makeScaleMatrix(x, y, z);
		*this = S * *this;
	}

	// scales this matrix by the three components
	void mat4::scale(const vec3& s)
	{
		mat4 S;
		S.makeScaleMatrix(s.x, s.y, s.z);
		*this = S * *this;
	}

	// apply a 'local' translation i.e. translate object in local space before
	// apply existing transformations
	void mat4::translateLocal(float x, float y, float z)
	{
		mat4 T;
		T.makeTranslationMatrix(x, y, z);
		*this = *this * T;
	}

	// apply a 'local' rotation i.e. rotate object in local space before
	// apply existing transformations
	void mat4::rotateLocal(float angle_in_rad, const vec3& axis)
	{
		mat4 R;
		R.makeRotationMatrix(angle_in_rad, axis);
		*this = *this * R;
	}

	// apply a 'local' scale i.e. scale object in local space before
	// apply existing transformations
	void mat4::scaleLocal(float x, float y, float z)
	{
		mat4 R;
		R.makeScaleMatrix(x, y, z);
		*this = *this * R;
	}

	// converts a vec3 to a vec4, multiplies with a mat4, and returns
	// 3 component vector of the result
	vec3 mat4::operator*(const vec3& vec) const
	{
		vec4 v4(vec.x, vec.y, vec.z, 1.0);
		vec4 v4m = *this * v4;
		return vec3(v4m.x, v4m.y, v4m.z);
	}

	// multiplies a vec4 with a mat4
	vec4 mat4::operator*(const vec4& v) const
	{
		vec4 ret;

		ret.x = v.x*m[0] + v.y*m[4] + v.z*m[8] + v.w*m[12];
		ret.y = v.x*m[1] + v.y*m[5] + v.z*m[9] + v.w*m[13];
		ret.z = v.x*m[2] + v.y*m[6] + v.z*m[10] + v.w*m[14];
		ret.w = v.x*m[3] + v.y*m[7] + v.z*m[11] + v.w*m[15];

		return ret;
	}

	// multiplies column major matrices such that result = this * N
	mat4 mat4::operator*(const mat4& N) const
	{
		mat4 result;

		unsigned int i, j, k;
		for (i = 0; i < 4; i++) //column
		{
			for (j = 0; j < 4; j++) //row
			{
				result.M[i][j] = 0.0; //reset
				for (k = 0; k < 4; k++) {
					//k-j iterates row
					//i-k iterates column
					//this.row * N.column
					result.M[i][j] += N.M[i][k] * M[k][j];
				}
			}
		}

		return result;
	}

	// turns this matrix into a view matrix
	void mat4::lookAt(const vec3& eye, const vec3& center, const vec3& up) {
		//create coordinate system of camera
		vec3 front = (center - eye).normalize();
		vec3 right = front.cross(up).normalize();
		vec3 top = right.cross(front).normalize();

		//reset this matrix
		setIdentity();

		M[0][0] = right.x; M[0][1] = top.x; M[0][2] = -front.x;
		M[1][0] = right.y; M[1][1] = top.y; M[1][2] = -front.y;
		M[2][0] = right.z; M[2][1] = top.z; M[2][2] = -front.z;

		translateLocal(-eye.x, -eye.y, -eye.z);


	}

	// turns this matrix into a perspective projection matrix
	// parameter fov is the field of view passed in degrees
	void mat4::perspective(float fov_rad, float aspect, float near_plane, float far_plane) {
		
		setIdentity();

		float f = 1.0f / tan(fov_rad * 0.5f);

		M[0][0] = f / aspect;
		M[1][1] = f;
		M[2][2] = (far_plane + near_plane) / (near_plane - far_plane);

		M[2][3] = -1.0f;
		M[3][2] = 2.0f * (far_plane * near_plane) / (near_plane - far_plane);
		M[3][3] = 0.0f;
	}

	// turns this matrix into an orthographic projection matrix
	void mat4::orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane) {

		setIdentity();

		//scale
		M[0][0] = 2.0f / (right - left);
		M[1][1] = 2.0f / (top - bottom);
		M[2][2] = -2.0f / (far_plane - near_plane);

		//translate
		M[3][0] = -((right + left) / (right - left));
		M[3][1] = -((top + bottom) / (top - bottom));
		M[3][2] = -((far_plane + near_plane) / (far_plane - near_plane));
		M[3][3] = 1.0f;
	}

}