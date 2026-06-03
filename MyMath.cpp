#include "MyMath.h"
#include <cmath>

#pragma region 00-01
//// 加算
//Vector3 Add(const Vector3& v1, const Vector3& v2) {
//	return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
//}
//
//// 減算
//Vector3 Subtract(const Vector3& v1, const Vector3& v2) {
//	return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
//}
//
//// スカラー倍
//Vector3 Multiply(float scalar, const Vector3& v) {
//	return { v.x * scalar, v.y * scalar, v.z * scalar };
//}
//
//// 内積
//float Dot(const Vector3& v1, const Vector3& v2) {
//	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
//}
//
//// 長さ
//float Length(const Vector3& v) {
//	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
//}
//
//// 正規化
//Vector3 Normalize(const Vector3& v) {
//	float len = Length(v);
//	if (len != 0.0f) {
//		return { v.x / len, v.y / len, v.z / len };
//	}
//	return { 0.0f, 0.0f, 0.0f };
//}
#pragma endregion

#pragma region 00-02

//// 行列の加法
//Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2) {
//    Matrix4x4 result;
//    for (int row = 0; row < 4; ++row) {
//        for (int column = 0; column < 4; ++column) {
//            result.m[row][column] = m1.m[row][column] + m2.m[row][column];
//        }
//    }
//    return result;
//}
//
//// 行列の減法
//Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2) {
//    Matrix4x4 result;
//    for (int row = 0; row < 4; ++row) {
//        for (int column = 0; column < 4; ++column) {
//            result.m[row][column] = m1.m[row][column] - m2.m[row][column];
//        }
//    }
//    return result;
//}
//
//// 行列の積
//Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
//    Matrix4x4 result;
//    for (int row = 0; row < 4; ++row) {
//        for (int column = 0; column < 4; ++column) {
//            result.m[row][column] =
//                m1.m[row][0] * m2.m[0][column] +
//                m1.m[row][1] * m2.m[1][column] +
//                m1.m[row][2] * m2.m[2][column] +
//                m1.m[row][3] * m2.m[3][column];
//        }
//    }
//    return result;
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
//
//Vector3 Transfrom(const Vector3& vector, const Matrix4x4& matrix) {
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

//Matrix4x4 MakeOrthograhicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
//    Matrix4x4 result = { 0 };
//
//    result.m[0][0] = 2.0f / (right - left);
//    result.m[1][1] = 2.0f / (top - bottom); 
//    result.m[2][2] = 1.0f / (farClip - nearClip);
//    result.m[3][0] = (left + right) / (left - right);
//    result.m[3][1] = (top + bottom) / (bottom - top);
//    result.m[3][2] = nearClip / (nearClip - farClip);
//    result.m[3][3] = 1.0f;
//
//    return result;
//}
//
//Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth) {
//    Matrix4x4 result = { 0 };
//
//    result.m[0][0] = width / 2.0f;
//    result.m[1][1] = -height / 2.0f; 
//    result.m[2][2] = maxDepth - minDepth;
//    result.m[3][0] = left + (width / 2.0f);
//    result.m[3][1] = top + (height / 2.0f);
//    result.m[3][2] = minDepth;
//    result.m[3][3] = 1.0f;
//
//    return result;
//}
#pragma endregion

//Vector3 Cross(const Vector3& v1, const Vector3& v2) 
//{
//    Vector3 result;
//    result.x = v1.y * v2.z - v1.z * v2.y;
//    result.y = v1.z * v2.x - v1.x * v2.z;
//    result.z = v1.x * v2.y - v1.y * v2.x;
//    return result;
//}