#pragma once
#include "WindowsApp.h"
#include "Logger.h"
#include "StringUtility.h"
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <array>
#include <dxcapi.h>

#ifdef USE_IMGUI
//#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
//extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif 

class DirectXBasics
{
public:

	//DirectX関連の初期化
	void Initialize(WindowsApp* windowsApp);
	//デバイスの初期化
	void DeviceInitialize();
	//コマンド関連の初期化
	void CommandInitialize();
	//スワップチェーンの生成
	void SwapChainInitialize();
	//深度バッファの生成
	void DepthBufferInitialize();
	//各種デスクリプタヒープの生成
	void VariousDescriptorHeapInitialize();
	//レンダーターゲットビューの初期化
	void RenderTargetViewInitialize();
	//深度ステンシルビューの初期化
	void DepthStencilViewInitialize();
	//フェンスの生成
	void FenceInitialize();
	//ビューポート矩形の初期化
	void ViewportRectangleInitialize();
	//シザリング矩形の初期化
	void ScissoringRectangleInitialize();
	//DXCコンパイラの生成
	void DXCCompilerInitialize();
	//ImGuiの初期化
	void ImGuiInitialize();
	
	//描画前処理
	void PreDraw();
	//描画後処理
	void PostDraw();

	//デスクリプタヒープを生成する
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	//指定番号のCPUデスクリプタハンドルを取得する
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);

	//指定番号のGPUデスクリプタハンドルを取得する
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

private:
	//DirectX12デバイス
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	//DXGIファクトリ
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;

	// コマンド関連
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;

	// スワップチェーン
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	//スワップチェーンリソース
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,2> swapChainResources;
	//リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource;

	// 各種デスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

	// フェンス関連
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	//フェンス値
	UINT64 fenceValue = 0;
	HANDLE fenceEvent = nullptr;

	//dxCompilerを初期化
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;

	// デスクリプタサイズ保持
	uint32_t descriptorSizeSRV = 0;
	uint32_t descriptorSizeRTV = 0;
	uint32_t descriptorSizeDSV = 0;

	// RTVハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2]{};

	//ビューポート矩形
	D3D12_VIEWPORT viewport{};
	//シザー矩形
	D3D12_RECT scissorRect{};

	//WindowsAPI
	WindowsApp* windowsApp = nullptr;

	//指定番号のCPUデスクリプタハンドルを取得する
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);

	//指定番号のGPUデスクリプタハンドルを取得する
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);
};