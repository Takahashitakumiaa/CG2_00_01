#include "MyMath.h"

#pragma region 00-01
// 加算
Vector3 Add(const Vector3& v1, const Vector3& v2) {
	return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}

// 減算
Vector3 Subtract(const Vector3& v1, const Vector3& v2) {
	return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}

// スカラー倍
Vector3 Multiply(float scalar, const Vector3& v) {
	return { v.x * scalar, v.y * scalar, v.z * scalar };
}

// 内積
float Dot(const Vector3& v1, const Vector3& v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
//
// 長さ
float Length(const Vector3& v) {
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

// 正規化
Vector3 Normalize(const Vector3& v) {
	float len = Length(v);
	if (len != 0.0f) {
		return { v.x / len, v.y / len, v.z / len };
	}
	return { 0.0f, 0.0f, 0.0f };
}
#pragma endregion

#pragma region 00-02

// 行列の加法
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result;
	for (int row = 0; row < 4; ++row) {
		for (int column = 0; column < 4; ++column) {
			result.m[row][column] = m1.m[row][column] + m2.m[row][column];
		}
	}
	return result;
}

// 行列の減法
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result;
	for (int row = 0; row < 4; ++row) {
		for (int column = 0; column < 4; ++column) {
			result.m[row][column] = m1.m[row][column] - m2.m[row][column];
		}
	}
	return result;
}

//// 行列の積
//Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
//	Matrix4x4 result;
//	for (int row = 0; row < 4; ++row) {
//		for (int column = 0; column < 4; ++column) {
//			result.m[row][column] =
//				m1.m[row][0] * m2.m[0][column] +
//				m1.m[row][1] * m2.m[1][column] +
//				m1.m[row][2] * m2.m[2][column] +
//				m1.m[row][3] * m2.m[3][column];
//		}
//	}
//	return result;
//}
//
//// 転置行列
//Matrix4x4 Transpose(const Matrix4x4& m) {
//    Matrix4x4 result;
//    for (int row = 0; row < 4; row++) {
//        for (int column = 0; column < 4; column++) {
//            result.m[row][column] = m.m[column][row];
//        }
//    }
//    return result;
//}
//
//// 単位行列の作成
Matrix4x4 MakeIdentity4x4() {
	Matrix4x4 result;
	for (int row = 0; row < 4; ++row) {
		for (int column = 0; column < 4; ++column) {
			if (row == column) {
				result.m[row][column] = 1.0f;
			}
			else {
				result.m[row][column] = 0.0f;
			}
		}
	}
	return result;
}
//
// 逆行列
Matrix4x4 Inverse(const Matrix4x4& m) {
	float a[4][4];
	for (int r = 0; r < 4; ++r) {
		for (int c = 0; c < 4; ++c) {
			a[r][c] = m.m[r][c];
		}
	}

	Matrix4x4 inv = MakeIdentity4x4();

	for (int i = 0; i < 4; ++i) {
		int pivotRow = i;
		float maxVal = std::fabsf(a[i][i]);
		for (int r = i + 1; r < 4; ++r) {
			if (std::fabsf(a[r][i]) > maxVal) {
				maxVal = std::fabsf(a[r][i]);
				pivotRow = r;
			}
		}

		if (maxVal < 1e-5f) {
			return MakeIdentity4x4();
		}

		if (pivotRow != i) {
			for (int c = 0; c < 4; ++c) {
				std::swap(a[i][c], a[pivotRow][c]);
				std::swap(inv.m[i][c], inv.m[pivotRow][c]);
			}
		}

		float pivotVal = a[i][i];
		for (int c = 0; c < 4; ++c) {
			a[i][c] /= pivotVal;
			inv.m[i][c] /= pivotVal;
		}

		for (int r = 0; r < 4; ++r) {
			if (r != i) {
				float factor = a[r][i];
				for (int c = 0; c < 4; ++c) {
					a[r][c] -= factor * a[i][c];
					inv.m[r][c] -= factor * inv.m[i][c];
				}
			}
		}
	}

	return inv;
}

#pragma endregion

#pragma region 00-03
Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 result;
	// まずはベースとなる単位行列（斜めが1、他は0）を作る
	result.m[0][0] = 1.0f; result.m[0][1] = 0.0f; result.m[0][2] = 0.0f; result.m[0][3] = 0.0f;
	result.m[1][0] = 0.0f; result.m[1][1] = 1.0f; result.m[1][2] = 0.0f; result.m[1][3] = 0.0f;
	result.m[2][0] = 0.0f; result.m[2][1] = 0.0f; result.m[2][2] = 1.0f; result.m[2][3] = 0.0f;
	result.m[3][0] = translate.x; result.m[3][1] = translate.y; result.m[3][2] = translate.z; result.m[3][3] = 1.0f;
	return result;
}
//
Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
	Matrix4x4 result;
	// 斜めのラインにそれぞれの拡大率を入れる
	result.m[0][0] = scale.x;  result.m[0][1] = 0.0f;     result.m[0][2] = 0.0f;     result.m[0][3] = 0.0f;
	result.m[1][0] = 0.0f;     result.m[1][1] = scale.y;  result.m[1][2] = 0.0f;     result.m[1][3] = 0.0f;
	result.m[2][0] = 0.0f;     result.m[2][1] = 0.0f;     result.m[2][2] = scale.z;  result.m[2][3] = 0.0f;
	result.m[3][0] = 0.0f;     result.m[3][1] = 0.0f;     result.m[3][2] = 0.0f;     result.m[3][3] = 1.0f;
	return result;
}

//Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix) {
//	Vector3 result;
//
//	// 行列の「縦の列」とベクトルの掛け算
//	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + 1.0f * matrix.m[3][0];
//	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + 1.0f * matrix.m[3][1];
//	result.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + 1.0f * matrix.m[3][2];
//
//	// 4つ目の要素（w）の計算
//	float w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + 1.0f * matrix.m[3][3];
//
//	// wが0で割り算（フリーズ）してしまうのを防ぎつつ、1/wを各座標に掛けて元の次元に戻す
//	if (w != 0.0f) {
//		result.x /= w;
//		result.y /= w;
//		result.z /= w;
//	}
//
//	return result;
//}
#pragma endregion

#pragma region 00-04
Matrix4x4 MakeRotateXMatrix(float theta) {
	float c = cosf(theta);
	float s = sinf(theta);
	Matrix4x4 result;
	result.m[0][0] = 1.0f; result.m[0][1] = 0.0f; result.m[0][2] = 0.0f; result.m[0][3] = 0.0f;
	result.m[1][0] = 0.0f; result.m[1][1] = c;    result.m[1][2] = s;    result.m[1][3] = 0.0f;
	result.m[2][0] = 0.0f; result.m[2][1] = -s;   result.m[2][2] = c;    result.m[2][3] = 0.0f;
	result.m[3][0] = 0.0f; result.m[3][1] = 0.0f; result.m[3][2] = 0.0f; result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 MakeRotateYMatrix(float theta) {
	float c = cosf(theta);
	float s = sinf(theta);
	Matrix4x4 result;
	result.m[0][0] = c;    result.m[0][1] = 0.0f; result.m[0][2] = -s;   result.m[0][3] = 0.0f;
	result.m[1][0] = 0.0f; result.m[1][1] = 1.0f; result.m[1][2] = 0.0f; result.m[1][3] = 0.0f;
	result.m[2][0] = s;    result.m[2][1] = 0.0f; result.m[2][2] = c;    result.m[2][3] = 0.0f;
	result.m[3][0] = 0.0f; result.m[3][1] = 0.0f; result.m[3][2] = 0.0f; result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 MakeRotateZMatrix(float theta) {
	float c = cosf(theta);
	float s = sinf(theta);
	Matrix4x4 result;
	result.m[0][0] = c;    result.m[0][1] = s;    result.m[0][2] = 0.0f; result.m[0][3] = 0.0f;
	result.m[1][0] = -s;   result.m[1][1] = c;    result.m[1][2] = 0.0f; result.m[1][3] = 0.0f;
	result.m[2][0] = 0.0f; result.m[2][1] = 0.0f; result.m[2][2] = 1.0f; result.m[2][3] = 0.0f;
	result.m[3][0] = 0.0f; result.m[3][1] = 0.0f; result.m[3][2] = 0.0f; result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result;
	for (int row = 0; row < 4; ++row) {
		for (int column = 0; column < 4; ++column) {
			result.m[row][column] =
				m1.m[row][0] * m2.m[0][column] +
				m1.m[row][1] * m2.m[1][column] +
				m1.m[row][2] * m2.m[2][column] +
				m1.m[row][3] * m2.m[3][column];
		}
	}
	return result;
}
#pragma endregion

#pragma region 00-05
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {

	Matrix4x4 matScale = MakeScaleMatrix(scale);

	// 2. 回転行列の作成（前回の課題で作成した関数をそのまま使うのが確実で安全です）
	Matrix4x4 matX = MakeRotateXMatrix(rotate.x);
	Matrix4x4 matY = MakeRotateYMatrix(rotate.y);
	Matrix4x4 matZ = MakeRotateZMatrix(rotate.z);

	// 回転行列を合成 (X -> Y -> Z の順)
	Matrix4x4 matRotate = Multiply(matX, Multiply(matY, matZ));

	// 3. 平行移動行列の作成
	Matrix4x4 matTranslate = MakeTranslateMatrix(translate);

	// 4. すべてを合成（スケール × 回転 × 平行移動）
	Matrix4x4 matScaleRotate = Multiply(matScale, matRotate);
	Matrix4x4 result = Multiply(matScaleRotate, matTranslate);

	return result;
}
#pragma endregion

#pragma region 01-00
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
	Matrix4x4 result = { 0 };
	float tanHalfFovY = std::tan(fovY / 2.0f);

	result.m[0][0] = 1.0f / (aspectRatio * tanHalfFovY);
	result.m[1][1] = 1.0f / tanHalfFovY;
	result.m[2][2] = farClip / (farClip - nearClip);
	result.m[2][3] = 1.0f;
	result.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);
	result.m[3][3] = 0.0f;

	return result;
}

Matrix4x4 MakeOrthograhicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
	Matrix4x4 result = { 0 };

	result.m[0][0] = 2.0f / (right - left);
	result.m[1][1] = 2.0f / (top - bottom);
	result.m[2][2] = 1.0f / (farClip - nearClip);
	result.m[3][0] = (left + right) / (left - right);
	result.m[3][1] = (top + bottom) / (bottom - top);
	result.m[3][2] = nearClip / (nearClip - farClip);
	result.m[3][3] = 1.0f;

	return result;
}

Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth) {
	Matrix4x4 result = { 0 };

	result.m[0][0] = width / 2.0f;
	result.m[1][1] = -height / 2.0f;
	result.m[2][2] = maxDepth - minDepth;
	result.m[3][0] = left + (width / 2.0f);
	result.m[3][1] = top + (height / 2.0f);
	result.m[3][2] = minDepth;
	result.m[3][3] = 1.0f;

	return result;
}
#pragma endregion
#pragma region 01-01
Vector3 Cross(const Vector3& v1, const Vector3& v2)
{
	Vector3 result;
	result.x = v1.y * v2.z - v1.z * v2.y;
	result.y = v1.z * v2.x - v1.x * v2.z;
	result.z = v1.x * v2.y - v1.y * v2.x;
	return result;
}
#pragma endregion
#pragma region 02-00
float LengthSquared(const Vector3& v) {
	return Dot(v, v);
}

// 正射影ベクトルを求める関数
Vector3 Project(const Vector3& v1, const Vector3& v2) {
	float lenSq = LengthSquared(v2);
	if (lenSq < 1e-6f) return { 0.0f, 0.0f, 0.0f };
	float t = Dot(v1, v2) / lenSq;
	return Multiply(t, v2);
}

// 線分上の最近接点を求める関数
Vector3 ClosestPoint(const Vector3& point, const Segment& segment) {
	Vector3 v = Subtract(point, segment.origin);
	float lenSq = LengthSquared(segment.diff);
	if (lenSq < 1e-6f) return segment.origin;

	float t = Dot(v, segment.diff) / lenSq;

	// 線分(Segment)なので [0, 1] にクランプ
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;

	return Add(segment.origin, Multiply(t, segment.diff));
}

#pragma endregion
#pragma region 02-01
bool IsCollision(const Sphere& s1, const Sphere& s2) {
	// 2つの球の中心点間の距離の2乗を求める
	Vector3 diff = Subtract(s2.center, s1.center);
	float distanceSq = LengthSquared(diff);

	// 半径の合計の2乗を求める
	float radiusSum = s1.radius + s2.radius;
	float radiusSumSq = radiusSum * radiusSum;

	// 距離の2乗が半径の和の2乗以下であれば衝突
	return distanceSq <= radiusSumSq;
}
#pragma endregion
#pragma region 02-02
Vector3 Perpendicular(const Vector3& vector) {
	if (vector.x != 0.0f || vector.y != 0.0f) {
		return { -vector.y, vector.x, 0.0f };
	}
	return { 0.0f, -vector.z, vector.y };
}
bool IsCollision(const Sphere& sphere, const Plane& plane) {
    float distanceToPlane = Dot(sphere.center, plane.normal) - plane.distance;
    return std::abs(distanceToPlane) <= sphere.radius;
}
#pragma endregion
#pragma region 02-03
bool IsCollision(const Segment& segment, const Plane& plane) {
	// 1. 法線と線の方向ベクトルの内積を求める
	float dot = Dot(plane.normal, segment.diff);

	// 2. 垂直（平行）ならば、平面と交わることはない
	if (std::abs(dot) < 1e-6f) {
		return false;
	}

	// 3. t を求める
	float t = (plane.distance - Dot(segment.origin, plane.normal)) / dot;

	// 4. t の値によって線分が平面と衝突しているか判断する (0.0f <= t <= 1.0f)
	if (t >= 0.0f && t <= 1.0f) {
		return true;
	}

	return false;
}
#pragma endregion
#pragma region 02-04
bool IsCollision(const Triangle& triangle, const Segment& segment)
{
	// --- 1. 三角形の法線ベクトルを計算 ---
	Vector3 edge1 = Subtract(triangle.vertices[1], triangle.vertices[0]);
	Vector3 edge2 = Subtract(triangle.vertices[2], triangle.vertices[0]);
	Vector3 normal = Normalize(Cross(edge1, edge2));

	// --- 2. 線分と三角形平面の交点判定 ---
	float dot = Dot(normal, segment.diff);

	// 平行な場合は衝突しない
	if (std::abs(dot) < 1e-6f) {
		return false;
	}

	// 平面の方程式の距離d（原点から平面への最短距離）を計算
	float d = Dot(normal, triangle.vertices[0]);

	// 交点までのパラメータ t を計算
	float t = (d - Dot(segment.origin, normal)) / dot;

	// t が線分の範囲 [0, 1] になければ当たっていない
	if (t < 0.0f || t > 1.0f) {
		return false;
	}

	// --- 3. 交点 P の座標を求める ---
	Vector3 p = Add(segment.origin, Multiply(t, segment.diff));

	// --- 4. 内外判定（交点 P が三角形の内側にあるか） ---
	// 各辺のベクトルと、頂点から交点Pへのベクトルの外積を計算する

	// 辺v0->v1
	Vector3 v01 = Subtract(triangle.vertices[1], triangle.vertices[0]);
	Vector3 v0p = Subtract(p, triangle.vertices[0]);
	Vector3 cross0 = Cross(v01, v0p);

	// 辺v1->v2
	Vector3 v12 = Subtract(triangle.vertices[2], triangle.vertices[1]);
	Vector3 v1p = Subtract(p, triangle.vertices[1]);
	Vector3 cross1 = Cross(v12, v1p);

	// 辺v2->v0
	Vector3 v20 = Subtract(triangle.vertices[0], triangle.vertices[2]);
	Vector3 v2p = Subtract(p, triangle.vertices[2]);
	Vector3 cross2 = Cross(v20, v2p);

	// すべての外積ベクトルが法線と同じ方向（内積がプラス）を向いていれば内側にある
	if (Dot(cross0, normal) >= 0.0f &&
		Dot(cross1, normal) >= 0.0f &&
		Dot(cross2, normal) >= 0.0f) {
		return true; // 衝突！
	}

	return false;
}
#pragma endregion
#pragma region 02-05
bool IsCollision(const AABB& aabb1, const AABB& aabb2)
{
	// X軸、Y軸、Z軸のすべてで範囲が重なっているかチェックする
	if ((aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) && // X軸の重なり
		(aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) && // Y軸の重なり
		(aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z))   // Z軸の重なり
	{
		return true; // すべて重なっていれば衝突
	}

	return false;
}
#pragma endregion
#pragma region 02-06
bool IsCollision(const AABB& aabb1, const Sphere& sphere)
{
	Vector3 closestPoint;

	// 各軸ごとに球の中心座標をAABBの[min, max]の範囲内にクランプする
	closestPoint.x = (std::max)(aabb1.min.x, (std::min)(sphere.center.x, aabb1.max.x));
	closestPoint.y = (std::max)(aabb1.min.y, (std::min)(sphere.center.y, aabb1.max.y));
	closestPoint.z = (std::max)(aabb1.min.z, (std::min)(sphere.center.z, aabb1.max.z));

	// 球の中心と最近接点との距離の2乗を計算する
	Vector3 diff = Subtract(closestPoint, sphere.center);
	float distanceSq = LengthSquared(diff);

	// 距離の2乗が半径の2乗以下であれば衝突している
	return distanceSq <= (sphere.radius * sphere.radius);
}
#pragma endregion
#pragma region 02-07
bool IsCollision(const AABB& aabb, const Segment& segment)
{
	// 衝突する可能性のあるパラメータ t の範囲を [0.0, 1.0] (線分の範囲) で初期化
	float tNear = 0.0f;
	float tFar = 1.0f;

	// --- X軸のスラブ判定 ---
	if (std::abs(segment.diff.x) < 1e-6f) {
		// 線分がX軸に垂直（平行）な場合、線分の始点がAABBの外にあれば絶対に当たらない
		if (segment.origin.x < aabb.min.x || segment.origin.x > aabb.max.x) {
			return false;
		}
	}
	else {
		// 入り口と出口のパラメータ t を計算
		float t1 = (aabb.min.x - segment.origin.x) / segment.diff.x;
		float t2 = (aabb.max.x - segment.origin.x) / segment.diff.x;

		float tMin = (std::min)(t1, t2);
		float tMax = (std::max)(t1, t2);

		tNear = (std::max)(tNear, tMin);
		tFar = (std::min)(tFar, tMax);
	}

	// --- Y軸のスラブ判定 ---
	if (std::abs(segment.diff.y) < 1e-6f) {
		if (segment.origin.y < aabb.min.y || segment.origin.y > aabb.max.y) {
			return false;
		}
	}
	else {
		float t1 = (aabb.min.y - segment.origin.y) / segment.diff.y;
		float t2 = (aabb.max.y - segment.origin.y) / segment.diff.y;

		float tMin = (std::min)(t1, t2);
		float tMax = (std::max)(t1, t2);

		tNear = (std::max)(tNear, tMin);
		tFar = (std::min)(tFar, tMax);
	}

	// --- Z軸のスラブ判定 ---
	if (std::abs(segment.diff.z) < 1e-6f) {
		if (segment.origin.z < aabb.min.z || segment.origin.z > aabb.max.z) {
			return false;
		}
	}
	else {
		float t1 = (aabb.min.z - segment.origin.z) / segment.diff.z;
		float t2 = (aabb.max.z - segment.origin.z) / segment.diff.z;

		float tMin = (std::min)(t1, t2);
		float tMax = (std::max)(t1, t2);

		tNear = (std::max)(tNear, tMin);
		tFar = (std::min)(tFar, tMax);
	}

	// 入り口のタイミングが出口よりも前、かつ有効な範囲に収まっていれば衝突
	return tNear <= tFar;
}
#pragma endregion
#pragma region 02-8ex
bool IsCollision(const OBB& obb, const Sphere& sphere)
{
	// OBBの中心から球の中心へのベクトル
	Vector3 centerDiff = Subtract(sphere.center, obb.center);

	// OBBのローカル空間での「球の中心の座標」を格納する変数
	Vector3 localClosestPoint = { 0.0f, 0.0f, 0.0f };

	// X軸
	float x = Dot(centerDiff, obb.orientation[0]);
	localClosestPoint.x = std::clamp(x, -obb.size.x, obb.size.x);

	// Y軸
	float y = Dot(centerDiff, obb.orientation[1]);
	localClosestPoint.y = std::clamp(y, -obb.size.y, obb.size.y);

	// Z軸
	float z = Dot(centerDiff, obb.orientation[2]);
	localClosestPoint.z = std::clamp(z, -obb.size.z, obb.size.z);

	// ローカル空間の最近接点をワールド空間に戻す
	// ワールド最近接点 = OBB中心 + (各軸のクランプ値 * 各軸の方向ベクトル)
	Vector3 worldClosestPoint = obb.center;
	worldClosestPoint = Add(worldClosestPoint, Multiply(localClosestPoint.x, obb.orientation[0]));
	worldClosestPoint = Add(worldClosestPoint, Multiply(localClosestPoint.y, obb.orientation[1]));
	worldClosestPoint = Add(worldClosestPoint, Multiply(localClosestPoint.z, obb.orientation[2]));

	// ワールド空間上での「球の中心」と「最近接点」の距離の2乗を計算
	Vector3 diff = Subtract(worldClosestPoint, sphere.center);
	float distanceSq = LengthSquared(diff);

	// 半径の2乗以下なら衝突
	return distanceSq <= (sphere.radius * sphere.radius);
}
#pragma endregion
#pragma region 02-9ex
bool IsCollision(const Segment& segment, const OBB& obb)
{
	Vector3 diffCenter = Subtract(segment.origin, obb.center);
	Vector3 localOrigin;
	localOrigin.x = Dot(diffCenter, obb.orientation[0]);
	localOrigin.y = Dot(diffCenter, obb.orientation[1]);
	localOrigin.z = Dot(diffCenter, obb.orientation[2]);

	// 方向ベクトルは平行移動しないので、そのまま各軸との内積でローカル方向を求める
	Vector3 localDiff;
	localDiff.x = Dot(segment.diff, obb.orientation[0]);
	localDiff.y = Dot(segment.diff, obb.orientation[1]);
	localDiff.z = Dot(segment.diff, obb.orientation[2]);

	// ローカル空間での線分を作成
	Segment localSegment = { localOrigin, localDiff };

	// --- 2. OBBのサイズからローカルAABBを作成する ---
	AABB localAABB;
	localAABB.min = { -obb.size.x, -obb.size.y, -obb.size.z };
	localAABB.max = { obb.size.x,  obb.size.y,  obb.size.z };

	// --- 3. すでに用意されている AABB と Segment の衝突判定に丸投げする ---
	return IsCollision(localAABB, localSegment);
}
#pragma endregion
#pragma region 02-10ex
bool IsCollision(const OBB& obb1, const OBB& obb2)
{
	// 2つのOBBの中心間の差分ベクトル
	Vector3 D = Subtract(obb2.center, obb1.center);

	// 2つのOBBの各軸の回転行列（方向ベクトル）の組み合わせから、
	// お互いの軸の相対的な関係（内積）を計算して、後で何度も使うためキャッシュする
	float R[3][3];
	float AbsR[3][3];
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			R[i][j] = Dot(obb1.orientation[i], obb2.orientation[j]);
			// 影の長さを計算するため、絶対値を用意する（誤差対策にわずかな値を足す）
			AbsR[i][j] = std::abs(R[i][j]) + 1e-5f;
		}
	}

	float r1, r2; // 各OBBを軸に投影したときの「半径（半分の幅）」

	for (int i = 0; i < 3; ++i) {
		r1 = (i == 0) ? obb1.size.x : ((i == 1) ? obb1.size.y : obb1.size.z);
		r2 = obb2.size.x * AbsR[i][0] + obb2.size.y * AbsR[i][1] + obb2.size.z * AbsR[i][2];
		if (std::abs(Dot(D, obb1.orientation[i])) > r1 + r2) return false;
	}

	for (int i = 0; i < 3; ++i) {
		r1 = obb1.size.x * AbsR[0][i] + obb1.size.y * AbsR[1][i] + obb1.size.z * AbsR[2][i];
		r2 = (i == 0) ? obb2.size.x : ((i == 1) ? obb2.size.y : obb2.size.z);
		if (std::abs(Dot(D, obb2.orientation[i])) > r1 + r2) return false;
	}

	// L = A0 x B0
	r1 = obb1.size.y * AbsR[2][0] + obb1.size.z * AbsR[1][0];
	r2 = obb2.size.y * AbsR[0][2] + obb2.size.z * AbsR[0][1];
	if (std::abs(Dot(D, obb1.orientation[2]) * R[1][0] - Dot(D, obb1.orientation[1]) * R[2][0]) > r1 + r2) return false;

	// L = A0 x B1
	r1 = obb1.size.y * AbsR[2][1] + obb1.size.z * AbsR[1][1];
	r2 = obb2.size.x * AbsR[0][2] + obb2.size.z * AbsR[0][0];
	if (std::abs(Dot(D, obb1.orientation[2]) * R[1][1] - Dot(D, obb1.orientation[1]) * R[2][1]) > r1 + r2) return false;

	// L = A0 x B2
	r1 = obb1.size.y * AbsR[2][2] + obb1.size.z * AbsR[1][2];
	r2 = obb2.size.x * AbsR[0][1] + obb2.size.y * AbsR[0][0];
	if (std::abs(Dot(D, obb1.orientation[2]) * R[1][2] - Dot(D, obb1.orientation[1]) * R[2][2]) > r1 + r2) return false;

	// L = A1 x B0
	r1 = obb1.size.x * AbsR[2][0] + obb1.size.z * AbsR[0][0];
	r2 = obb2.size.y * AbsR[1][2] + obb2.size.z * AbsR[1][1];
	if (std::abs(Dot(D, obb1.orientation[0]) * R[2][0] - Dot(D, obb1.orientation[2]) * R[0][0]) > r1 + r2) return false;

	// L = A1 x B1
	r1 = obb1.size.x * AbsR[2][1] + obb1.size.z * AbsR[0][1];
	r2 = obb2.size.x * AbsR[1][2] + obb2.size.z * AbsR[1][0];
	if (std::abs(Dot(D, obb1.orientation[0]) * R[2][1] - Dot(D, obb1.orientation[2]) * R[0][1]) > r1 + r2) return false;

	// L = A1 x B2
	r1 = obb1.size.x * AbsR[2][2] + obb1.size.z * AbsR[0][2];
	r2 = obb2.size.x * AbsR[1][1] + obb2.size.y * AbsR[1][0];
	if (std::abs(Dot(D, obb1.orientation[0]) * R[2][2] - Dot(D, obb1.orientation[2]) * R[0][2]) > r1 + r2) return false;

	// L = A2 x B0
	r1 = obb1.size.x * AbsR[1][0] + obb1.size.y * AbsR[0][0];
	r2 = obb2.size.y * AbsR[2][2] + obb2.size.z * AbsR[2][1];
	if (std::abs(Dot(D, obb1.orientation[1]) * R[0][0] - Dot(D, obb1.orientation[0]) * R[1][0]) > r1 + r2) return false;

	// L = A2 x B1
	r1 = obb1.size.x * AbsR[1][1] + obb1.size.y * AbsR[0][1];
	r2 = obb2.size.x * AbsR[2][2] + obb2.size.z * AbsR[2][0];
	if (std::abs(Dot(D, obb1.orientation[1]) * R[0][1] - Dot(D, obb1.orientation[0]) * R[1][1]) > r1 + r2) return false;

	// L = A2 x B2
	r1 = obb1.size.x * AbsR[1][2] + obb1.size.y * AbsR[0][2];
	r2 = obb2.size.x * AbsR[2][1] + obb2.size.y * AbsR[2][0];
	if (std::abs(Dot(D, obb1.orientation[1]) * R[0][2] - Dot(D, obb1.orientation[0]) * R[1][2]) > r1 + r2) return false;

	// 15パターンの全ての軸において隙間が確認できなかったら、衝突している
	return true;
}
bool IsCollision(const AABB& aabb, const Vector3& point)
{
	if ((aabb.min.x <= point.x && aabb.max.x >= point.x) &&
		(aabb.min.y <= point.y && aabb.max.y >= point.y) &&
		(aabb.min.z <= point.z && aabb.max.z >= point.z))
	{
		return true;
	}
	return false;
}
Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t)
{
	return {
		v1.x + (v2.x - v1.x) * t,
		v1.y + (v2.y - v1.y) * t,
		v1.z + (v2.z - v1.z) * t
	};
}
#pragma endregion
#pragma region 03-02
Vector3 operator+(const Vector3& v1, const Vector3& v2) { return Add(v1, v2); }
Vector3 operator-(const Vector3& v1, const Vector3& v2) { return Subtract(v1, v2); }
Vector3 operator*(float s, const Vector3& v) { return Multiply(s, v); }
Vector3 operator*(const Vector3& v, float s) { return s * v; }
Vector3 operator/(const Vector3& v, float s) { return Multiply(1.0f / s, v); }
Vector3 operator-(const Vector3& v) { return{ -v.x,-v.y,-v.z }; }
Vector3  operator+(const Vector3& v) { return v; }

Vector3& operator+=(Vector3& v1, const Vector3& v2) {
	v1.x += v2.x;
	v1.y += v2.y;
	v1.z += v2.z;
	return v1;
}
Vector3& operator-=(Vector3& v1, const Vector3& v2) {
	v1.x -= v2.x;
	v1.y -= v2.y;
	v1.z -= v2.z;
	return v1;
}

Matrix4x4 operator+(const Matrix4x4& m1, const Matrix4x4& m2) { return Add(m1, m2); }
Matrix4x4 operator-(const Matrix4x4& m1, const Matrix4x4& m2) { return Subtract(m1, m2); }
Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2) { return Multiply(m1, m2); }
#pragma endregion

//------後期------

Vector3 ToCartesian(const Spherical& s)
{
	float rho = s.radius * std::cos(s.theta);

	return{
		rho * std::cos(s.phi),
		s.radius * std::sin(s.theta),
		rho * std::sin(s.phi)
	};
}

Spherical ToSpherical(const Vector3& p) {

	float r = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);

	if (r == 0.0f)
	{
		return{ 0.0f,0.0f,0.0f };
	}

	float sinTheta = std::clamp(p.y / r, -1.0f, 1.0f);
	float phi = 0.0f;

	if (p.x != 0.0f || p.z != 0.0f)
	{
		phi = std::atan2(p.z, p.x);
	}
	return { r,std::asin(sinTheta),phi };
}

Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs)
{
	return {
		lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y, // x
		lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x, // y
		lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w, // z
		lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z  // w
	};
}

Quaternion IdentityQuaternion()
{
	return { 0.0f, 0.0f, 0.0f, 1.0f };
}

Quaternion Conjugate(const Quaternion& quaternion)
{
	return { -quaternion.x, -quaternion.y, -quaternion.z, quaternion.w };
}

float Norm(const Quaternion& quaternion)
{
	return std::sqrt(
		quaternion.x * quaternion.x +
		quaternion.y * quaternion.y +
		quaternion.z * quaternion.z +
		quaternion.w * quaternion.w
	);
}

Quaternion Normalize(const Quaternion& quaternion)
{
	float norm = Norm(quaternion);
	if (norm != 0.0f) {
		return {
			quaternion.x / norm,
			quaternion.y / norm,
			quaternion.z / norm,
			quaternion.w / norm
		};
	}
	return IdentityQuaternion();
}

Quaternion Inverse(const Quaternion& quaternion)
{
	float normSq =
		quaternion.x * quaternion.x +
		quaternion.y * quaternion.y +
		quaternion.z * quaternion.z +
		quaternion.w * quaternion.w;

	if (normSq != 0.0f) {
		Quaternion conj = Conjugate(quaternion);
		return {
			conj.x / normSq,
			conj.y / normSq,
			conj.z / normSq,
			conj.w / normSq
		};
	}
	return IdentityQuaternion();
}

// 任意軸回転を表すQuaternionの生成
Quaternion MakeRotateAxisAngleQuaternion(const Vector3& axis, float angle) {
	Vector3 normAxis = Normalize(axis);
	float sinHalf = std::sin(angle / 2.0f);
	float cosHalf = std::cos(angle / 2.0f);

	return {
		normAxis.x * sinHalf,
		normAxis.y * sinHalf,
		normAxis.z * sinHalf,
		cosHalf
	};
}

// ベクトルをQuaternionで回転させた結果のベクトルを求める
Vector3 RotateVector(const Vector3& vector, const Quaternion& quaternion) {
	// ベクトルを純粋クォータニオン(w = 0)に変換
	Quaternion qv = { vector.x, vector.y, vector.z, 0.0f };
	// 共役クォータニオンを取得
	Quaternion qConj = Conjugate(quaternion);
	// q * v * q* の計算
	Quaternion qResult = Multiply(quaternion, Multiply(qv, qConj));

	return { qResult.x, qResult.y, qResult.z };
}

// Quaternionから回転行列を求める（Transform等の行ベクトル形式に対応）
Matrix4x4 MakeRotateMatrix(const Quaternion& quaternion) {
	float x = quaternion.x;
	float y = quaternion.y;
	float z = quaternion.z;
	float w = quaternion.w;

	Matrix4x4 result;

	result.m[0][0] = 1.0f - 2.0f * (y * y + z * z);
	result.m[0][1] = 2.0f * (x * y + w * z);
	result.m[0][2] = 2.0f * (x * z - w * y);
	result.m[0][3] = 0.0f;

	result.m[1][0] = 2.0f * (x * y - w * z);
	result.m[1][1] = 1.0f - 2.0f * (x * x + z * z);
	result.m[1][2] = 2.0f * (y * z + w * x);
	result.m[1][3] = 0.0f;

	result.m[2][0] = 2.0f * (x * z + w * y);
	result.m[2][1] = 2.0f * (y * z - w * x);
	result.m[2][2] = 1.0f - 2.0f * (x * x + y * y);
	result.m[2][3] = 0.0f;

	result.m[3][0] = 0.0f;
	result.m[3][1] = 0.0f;
	result.m[3][2] = 0.0f;
	result.m[3][3] = 1.0f;

	return result;
}

Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t)
{
	// 2つのQuaternionの内積を求める
	float dot = q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w;

	Quaternion q1_ = q1;
	// 内積が負の場合は、もう片方の回転（-q1）を利用して最短経路で補間する
	if (dot < 0.0f) {
		q1_ = { -q1.x, -q1.y, -q1.z, -q1.w };
		dot = -dot;
	}

	// 計算誤差によってdotが1.0fを超えるとstd::acosがNaNを返すためクランプする
	dot = std::clamp(dot, -1.0f, 1.0f);

	// なす角を求める
	float theta = std::acos(dot);

	// 2つの回転がほぼ同じ向き（thetaが0に近い）場合はゼロ除算を防ぐため線形補間を行う
	if (std::sin(theta) < 1e-6f) {
		return Normalize({
			(1.0f - t) * q0.x + t * q1_.x,
			(1.0f - t) * q0.y + t * q1_.y,
			(1.0f - t) * q0.z + t * q1_.z,
			(1.0f - t) * q0.w + t * q1_.w
			});
	}

	// 補間係数を求める
	float sinTheta = std::sin(theta);
	float scale0 = std::sin((1.0f - t) * theta) / sinTheta;
	float scale1 = std::sin(t * theta) / sinTheta;

	// それぞれの補間係数を掛けて足し合わせる
	return {
		scale0 * q0.x + scale1 * q1_.x,
		scale0 * q0.y + scale1 * q1_.y,
		scale0 * q0.z + scale1 * q1_.z,
		scale0 * q0.w + scale1 * q1_.w
	};
}