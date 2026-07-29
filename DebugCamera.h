#pragma once
#include "MyMath.h"
#include "keyInput.h"

class DebugCamera
{
public:

	void Initialize();

	void Update();

	const Matrix4x4& GetMatView() const { return matView_; }
	const Matrix4x4& GetMatProjection() const { return matProjection_; }

	void SetActive(bool active) { isActive_ = active; }
	bool IsActive() const { return isActive_; }

private:
	//累積回転行列
	Matrix4x4 matRot_ = MakeIdentity4x4();
	//ローカル座標
	Vector3 translation_ = { 0,0,-50 };
	//ビュー座標
	Matrix4x4 matView_ = MakeIdentity4x4();
	//射影行列
	Matrix4x4 matProjection_ = MakeIdentity4x4();

	//デバッグカメラ有効
	bool isActive_ = false;
};

