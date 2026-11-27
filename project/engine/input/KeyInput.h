#pragma once
#define DIRECTINPUT_VERSION 0x0800
#include "WindowsApp.h"
#include <dinput.h>
#include <Windows.h>
#include <wrl.h>
#include <cassert>
#include <array>
#include <cstdint>

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

class KeyInput
{
public:
	//namespace
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:

	void Initialize(WindowsApp* windows);

	void Update();

	bool PushKey(BYTE keyNumber)const;

	bool TriggerKey(BYTE keyNumber)const;

private:

	ComPtr<IDirectInputDevice8> keyboard;

	ComPtr<IDirectInput8> directInput;

	BYTE key[256] = {};
	//前回の全キーの状態
	BYTE keyPre[256] = {};

	//WindowsAPI
	WindowsApp* windowsApp_ = nullptr;
};