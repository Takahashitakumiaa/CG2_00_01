#pragma once
#include <cmath>

struct Matrix4x4
{
	float m[4][4];
};

struct Vector3
{
	float x, y, z;
};

#pragma region 00-01
//Vector3 Add(const Vector3& v1, const Vector3& v2);
//Vector3 Subtract(const Vector3& v1, const Vector3& v2);
//Vector3 Multiply(float scalar, const Vector3& v);
//float Dot(const Vector3& v1, const Vector3& v2);
//float Length(const Vector3& v);
//Vector3 Normalize(const Vector3& v);
//Vector3 Cross(const Vector3& v1, const Vector3& v2);
#pragma endregion

#pragma region 00-02
//Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);
//Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);
//Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);
//Matrix4x4 Transpose(const Matrix4x4& m);
Matrix4x4 Inverse(const Matrix4x4& m);
Matrix4x4 MakeIdentity4x4();
#pragma endregion

#pragma region 00-03
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

Matrix4x4 MakeScaleMatrix(const Vector3& scale);
//
//Vector3 Transfrom(const Vector3& vector, const Matrix4x4& matrix);
#pragma endregion
#pragma region 00-04
Matrix4x4 MakeRotateXMatrix(float theta);

Matrix4x4 MakeRotateYMatrix(float theta);

Matrix4x4 MakeRotateZMatrix(float theta);

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);
#pragma endregion
#pragma region 00-05
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);
#pragma endregion
#pragma region 01-00
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

Matrix4x4 MakeOrthograhicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);
//
//Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);
#pragma endregion

//Vector3 Cross(const Vector3& v1, const Vector3& v2);

