#include "DebugCamera.h"

void DebugCamera::Initialize()
{
	matRot_ = MakeIdentity4x4();

	//ローカル座標
	translation_ = { 0,0,-50 };
	//ビュー座標
	matView_ = MakeIdentity4x4();
	//射影行列
	/*matProjection_ = MakeIdentity4x4();*/
	matProjection_ = MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 1000.0f);
}

void DebugCamera::Update()
{
	if (!isActive_)
	{
		return;
	}

	const float speed = 0.5f;
	const float rotateSpeed = 0.02f;

	float deltaX = 0.0f;
	float deltaY = 0.0f;
	float deltaZ = 0.0f;

	//回転
  // X軸回転 (縦の首振り)
	if (keyInput::IsPress(DIK_UP))
	{
		deltaX -= rotateSpeed;
	}
	if (keyInput::IsPress(DIK_DOWN))
	{
		deltaX += rotateSpeed;
	}

	// Y軸回転 (横の首振り)
	if (keyInput::IsPress(DIK_LEFT))
	{
		deltaY -= rotateSpeed;
	}
	if (keyInput::IsPress(DIK_RIGHT))
	{
		deltaY += rotateSpeed;
	}

	// Z軸回転 (画面傾き・ロール)
	if (keyInput::IsPress(DIK_U))
	{
		deltaZ -= rotateSpeed;
	} // 左傾き
	if (keyInput::IsPress(DIK_O))
	{
		deltaZ += rotateSpeed;
	} //

	//追加回転分の回転行列を生成
	Matrix4x4 matRotDelta = MakeIdentity4x4();
	matRotDelta = Multiply(matRotDelta, MakeRotateXMatrix(deltaX));
	matRotDelta = Multiply(matRotDelta, MakeRotateYMatrix(deltaY));
	matRotDelta = Multiply(matRotDelta, MakeRotateZMatrix(deltaZ));

	//累積の回転行列を合成
	matRot_ = Multiply(matRotDelta, matRot_);

	//入力によるカメラの移動や回転
	//前後移動SS
	if (keyInput::IsPress(DIK_W))
	{
		//カメラ移動ベクトル
		Vector3 move = { 0,0,speed };
		//移動ベクトルを角度分だけ回転させる
		Vector3 rotatedMove;
		rotatedMove.x = move.x * matRot_.m[0][0] + move.y * matRot_.m[1][0] + move.z * matRot_.m[2][0];
		rotatedMove.y = move.x * matRot_.m[0][1] + move.y * matRot_.m[1][1] + move.z * matRot_.m[2][1];
		rotatedMove.z = move.x * matRot_.m[0][2] + move.y * matRot_.m[1][2] + move.z * matRot_.m[2][2];
		//移動ベクトル分だけ座標を加算する
		translation_.x += rotatedMove.x;
		translation_.y += rotatedMove.y;
		translation_.z += rotatedMove.z;
	}
	if (keyInput::IsPress(DIK_S))
	{
		Vector3 move = { 0, 0, -speed };

		Vector3 rotatedMove;
		rotatedMove.x = move.x * matRot_.m[0][0] + move.y * matRot_.m[1][0] + move.z * matRot_.m[2][0];
		rotatedMove.y = move.x * matRot_.m[0][1] + move.y * matRot_.m[1][1] + move.z * matRot_.m[2][1];
		rotatedMove.z = move.x * matRot_.m[0][2] + move.y * matRot_.m[1][2] + move.z * matRot_.m[2][2];

		translation_.x += rotatedMove.x;
		translation_.y += rotatedMove.y;
		translation_.z += rotatedMove.z;
	}
	//左右移動
	if (keyInput::IsPress(DIK_D))
	{
		//カメラ移動ベクトル
		Vector3 move = { speed,0,0 };
		//移動ベクトルを角度分だけ回転させる
		Vector3 rotatedMove;
		rotatedMove.x = move.x * matRot_.m[0][0] + move.y * matRot_.m[1][0] + move.z * matRot_.m[2][0];
		rotatedMove.y = move.x * matRot_.m[0][1] + move.y * matRot_.m[1][1] + move.z * matRot_.m[2][1];
		rotatedMove.z = move.x * matRot_.m[0][2] + move.y * matRot_.m[1][2] + move.z * matRot_.m[2][2];
		//移動ベクトル分だけ座標を加算する
		translation_.x += rotatedMove.x;
		translation_.y += rotatedMove.y;
		translation_.z += rotatedMove.z;
	}
	if (keyInput::IsPress(DIK_A))
	{
		Vector3 move = { -speed, 0,0 };

		Vector3 rotatedMove;
		rotatedMove.x = move.x * matRot_.m[0][0] + move.y * matRot_.m[1][0] + move.z * matRot_.m[2][0];
		rotatedMove.y = move.x * matRot_.m[0][1] + move.y * matRot_.m[1][1] + move.z * matRot_.m[2][1];
		rotatedMove.z = move.x * matRot_.m[0][2] + move.y * matRot_.m[1][2] + move.z * matRot_.m[2][2];

		translation_.x += rotatedMove.x;
		translation_.y += rotatedMove.y;
		translation_.z += rotatedMove.z;
	}

	//ビュー行列や更新
	//座標から平行移動行列を計算する
	Matrix4x4 matTranslate = MakeTranslateMatrix(translation_);
	//回転行列と平行移動行列からワールド座標を計算する
	Matrix4x4 cameraWorld = Multiply(matRot_, matTranslate);
	//ワールド座標の逆行列をビューに行列に代入する
	matView_ = Inverse(cameraWorld);
}