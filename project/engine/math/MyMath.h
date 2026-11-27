#pragma once
#include <cmath>
#include <vector>
#include <cstdint>
#include <string>
#include <algorithm>
#include <numbers>

struct Matrix3x3
{
	float m[3][3];
};

struct Matrix4x4
{
	float m[4][4];
};

struct Vector2
{
	float x, y;
};

struct Vector3
{
	float x, y, z;
};

struct Vector4
{
	float x, y, z, w;
};

struct VertexData
{
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct Transform
{
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

struct Material
{
	Vector4 color;
	int32_t enableLighing;
	float padding[3];
	Matrix4x4 uvTransform;
};

struct TransformationMatrix
{
	Matrix4x4 WVP;
	Matrix4x4 World;
};

struct DirectionalLight
{
	Vector4 color;//ライトの色
	Vector3 direction;//ライトの向き
	float intensity;//輝度
};

struct MaterialData
{
	std::string textureFilePath;
};

struct ModelData
{
	std::vector<VertexData> vertices;
	MaterialData material;
};

struct Sphere
{
	Vector3 center;//!<中心点
	float radius;//!<半径
};

struct Line
{
	Vector3 origin; //!< 始点
	Vector3 diff;   //!< 終点への差分ベクトル
};

struct Ray
{
	Vector3 origin; //!< 始点
	Vector3 diff;   //!< 終点への差分ベクトル
};

struct Segment
{
	Vector3 origin; //!< 始点
	Vector3 diff;   //!< 終点への差分ベクトル
};

// 平面構造体
struct Plane
{
	Vector3 normal;   //!< 法線
	float distance;   //!< 距離
};

struct Triangle
{
	Vector3 vertices[3];//!<頂点
};

struct AABB
{
	Vector3 min;//!<最小値
	Vector3 max;//!<最大値
};

struct OBB
{
	Vector3 center;//!<中心点
	Vector3 orientation[3];//!<座標軸。正規化・直交必須
	Vector3 size;//!<座標軸方向の長さの半分。中心から面までの距離
};

struct Spring
{
	//アンカー。固定された端の位置
	Vector3 anchor;
	float naturalLength;//自然長
	float stiffness;//剛性。バネ定数k
};

struct Ball
{
	Vector3 position;//ボールの位置
	Vector3 velocity;//ボールの速度
	Vector3 acceleration;//ボールの加速度
	float mass;//ボールの質量
	float radius;//ボールの半径
	unsigned int color;//ボールの色
};

//------後期------

struct Spherical
{
	float radius;//動径
	float theta;//仰角
	float phi;//方位角
};

struct Quaternion
{
	float x;
	float y;
	float z;
	float w;
};

//#pragma region 00-01
Vector3 Add(const Vector3& v1, const Vector3& v2);
Vector3 Subtract(const Vector3& v1, const Vector3& v2);
Vector3 Multiply(float scalar, const Vector3& v);
float Dot(const Vector3& v1, const Vector3& v2);
float Length(const Vector3& v);
Vector3 Normalize(const Vector3& v);
//Vector3 Cross(const Vector3& v1, const Vector3& v2);
#pragma endregion

#pragma region 00-02
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);
//Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);
//Matrix4x4 Transpose(const Matrix4x4& m);
Matrix4x4 Inverse(const Matrix4x4& m);
Matrix4x4 MakeIdentity4x4();
#pragma endregion

#pragma region 00-03
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

Matrix4x4 MakeScaleMatrix(const Vector3& scale);

//Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);
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

Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);
#pragma endregion
#pragma region 01-01
Vector3 Cross(const Vector3& v1, const Vector3& v2);
#pragma endregion
#pragma region 02-00
float LengthSquared(const Vector3& v);
// 正射影ベクトルを求める関数
Vector3 Project(const Vector3& v1, const Vector3& v2);

// 線分上の最近接点を求める関数
Vector3 ClosestPoint(const Vector3& point, const Segment& segment);
#pragma endregion
#pragma region 02-01
bool IsCollision(const Sphere& s1, const Sphere& s2);
#pragma endregion
#pragma region 02-02
Vector3 Perpendicular(const Vector3& vector);

bool IsCollision(const Sphere& sphere, const Plane& plane);
#pragma endregion
#pragma region 02-03
bool IsCollision(const Segment& segment, const Plane& plane);
#pragma endregion
#pragma region 02-04
bool IsCollision(const Triangle& triangle, const Segment& segment);
#pragma endregion
#pragma region 02-05
bool IsCollision(const AABB& aabb1, const AABB& aabb2);
#pragma endregion
#pragma region 02-06
bool IsCollision(const AABB& aabb1, const Sphere& sphere);
#pragma endregion
#pragma region 02-07
bool IsCollision(const AABB& aabb, const Segment& segment);
#pragma endregion
#pragma region 02-8ex
bool IsCollision(const OBB& obb, const Sphere& sphere);
#pragma endregion
#pragma region 02-9ex
bool IsCollision(const Segment& segment, const OBB& obb);
#pragma endregion
#pragma region 02-10ex
bool IsCollision(const OBB& obb1, const OBB& obb2);
#pragma endregion

bool IsCollision(const AABB& aabb, const Vector3& point);

#pragma region 03-00
Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t);
#pragma endregion
#pragma region 03-02
Vector3 operator+(const Vector3& v1, const Vector3& v2);
Vector3 operator-(const Vector3& v1, const Vector3& v2);
Vector3 operator*(const Vector3& v, float s);
Vector3 operator*(float s, const Vector3& v);
Vector3 operator/(const Vector3& v, float s);
Vector3 operator-(const Vector3& v);
Vector3 operator+(const Vector3& v);

Vector3& operator+=(Vector3& v1, const Vector3& v2);
Vector3& operator-=(Vector3& v1, const Vector3& v2);

Matrix4x4 operator+(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 operator-(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2);
#pragma endregion

//------後期------

//球面座標から直交座標への変換関数
Vector3 ToCartesian(const Spherical& s);

Spherical ToSpherical(const Vector3& p);

//Quaternionの積
Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs);
//乗法単位元を返す
Quaternion IdentityQuaternion();
//共役Quaternionを返す
Quaternion Conjugate(const Quaternion& quaternion);
//Quaternionのnormを返す
float Norm(const Quaternion& quaternion);
//正規化したQuaternionを返す
Quaternion Normalize(const Quaternion& quaternion);
//逆Quaternionを返す
Quaternion Inverse(const Quaternion& quaternion);

//任意軸回転を表すQuaternionの生成
Quaternion MakeRotateAxisAngleQuaternion(const Vector3& axis, float angle);
//ベクトルをQuaternionで回転させた結果のベクトルを求める
Vector3 RotateVector(const Vector3& vector, const Quaternion& quaternion);
//Quaternionから回転行列を求める
Matrix4x4 MakeRotateMatrix(const Quaternion& quaternion);

//球面線形補間
Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t);