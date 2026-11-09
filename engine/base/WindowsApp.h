#pragma once
#include <Windows.h>
#include <cstdint>

class WindowsApp
{
public:

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	void Initialize();

	void Update();

	//終了
	void Finalize();

	//メッセージ処理
	bool ProcessMessage();

	//クライアント領域のサイズ
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

	//getter
	HWND GetHwnd()const { return hwnd; }

	HINSTANCE GetInstance()const { return wc.hInstance; }

private:
	//ウィンドウハンドル
	HWND hwnd = nullptr;

	//ウィンドウクラスの設定
	WNDCLASS wc{};
};

