#include "keyInput.h"

void KeyInput::Initialize(HINSTANCE hInstance, HWND hwnd)
{
	HRESULT result;

	//DirectInputの初期化
	/*Microsoft::WRL::ComPtr<IDirectInput8> directInput = nullptr;*/
	result = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(result));

	//キーボードデバイスの生成
	/*Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard = nullptr;*/
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));
	//入力データの形式のセット
	result = keyboard->SetDataFormat(&c_dfDIKeyboard);//標準形式
	assert(SUCCEEDED(result));
	//排他制御レベルのセット
	result = keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));
}

void KeyInput::Update()
{
	HRESULT result;

	//前回のキー入力を保存
	memcpy(keyPre, key, sizeof(key));

	//キーボード情報の取得開始
	result = keyboard->Acquire();
	//全キーの入力情報の取得する
	result = keyboard->GetDeviceState(sizeof(key), key);
}

bool KeyInput::PushKey(BYTE keyNumber) const
{
	return (key[keyNumber] & 0x80) != 0;
}

bool KeyInput::TriggerKey(BYTE keyNumber) const
{
	return (key[keyNumber] & 0x80) && !(keyPre[keyNumber] & 0x80);
}