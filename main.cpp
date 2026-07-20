#include "MyMath.h"
#include <Windows.h>
#include <cstdint>
#include <string>
//ファイルやディレクトリに関する操作を行うライブラリ
#include <filesystem>
//ファイルを書いたり読んだりするライブラリ
#include <fstream>
//時間を扱うライブラリ
#include <chrono>
#include <format>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <dbghelp.h>
#include <strsafe.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <wrl.h>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif 

#pragma comment(lib,"d3d12.lib") 
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"Dbghelp.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")

//ウィンドウプロシージャ
LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
#ifdef USE_IMGUI
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return true;
	}
#endif // USE_IMGUI

	//メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
		//ウィンドウが放棄された
	case WM_DESTROY:
		//OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}
	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Log(const std::string& message)
{
	OutputDebugStringA(message.c_str());
}

std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}
void Log(const std::wstring& message)
{
	Log(ConvertString(message));
}

void Log(std::ostream& os, const std::string& message)
{
	os << message << std::endl;
	OutputDebugStringA(message.c_str());
}

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception)
{
	//時刻を取得して、時刻を名前に入れたファイルを作成。Dumosディレクトリ以下に出力
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	//processId(このexeのId)とクラッシュ（例外）の発生したthreadIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadID = GetCurrentThreadId();
	//設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInfomation{ 0 };
	minidumpInfomation.ThreadId = threadID;
	minidumpInfomation.ExceptionPointers = exception;
	minidumpInfomation.ClientPointers = true;
	//Dumpを出力。MinidumpNomalは最低限の情報を出力するフラグ
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInfomation, nullptr, nullptr);
	//他に関連づけられているSEH例外ハンドラがあれば実行。通常はプロセスを終了する
	return EXCEPTION_EXECUTE_HANDLER;
}

Microsoft::WRL::ComPtr <IDxcBlob> CompileShader(
	//CompilerするShaderファイルへのパス
	const std::wstring& filePath,
	//Compilerに使用するporfile
	const wchar_t* profile,
	//初期化で生成したものを3つ
	Microsoft::WRL::ComPtr <IDxcUtils> dxcUtils,
	Microsoft::WRL::ComPtr <IDxcCompiler3> dxcCompiler,
	Microsoft::WRL::ComPtr <IDxcIncludeHandler> includeHandler)
{
	//hlslファイルを読む
	//これからシェーダーをコンパイルする旨をログに出す
	Log(ConvertString(std::format(L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile)));
	//hlslファイルを読む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	//読めなかったら止める
	assert(SUCCEEDED(hr));
	//読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;//UTF8の文字コードである事を通知
	//Compilerする
	LPCWSTR arguments[] = {
		filePath.c_str(),//コンパイル対象のhlslファイル名
		L"-E", L"main",//エントリーポイントの指定。基本的にmain以外にはしない
		L"-T", profile,//ShaderProfileの設定
		L"-Zi", L"-Qembed_debug",//デバッグ用の情報を埋め込む
		L"-Od",//最適化を外しておく
		L"-Zpr",//メモリレイアウトは行優先
	};
	//実際にshaderをコンパイルする
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,//読み込んだファイル
		arguments,//コンパイルオプション
		_countof(arguments),//コンパイルの数
		includeHandler.Get(),//includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult)//コンパイル結果
	);
	//コンパイルエラーではなくdxcが起動出来ないなど致命的な状況
	assert(SUCCEEDED(hr));
	//警告・エラーが出ていないか確認する
		//警告・エラーが出てたらログに出して止める
	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0)
	{
		Log(shaderError->GetStringPointer());
		//警告・エラーダメ絶対
		assert(false);
	}
	//Compiler結果を受け取って返す
	//コンパイル結果から実行用のバイナリ部分を取得
	Microsoft::WRL::ComPtr <IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	//成功したログを出す
	Log(ConvertString(std::format(L"Compile Succeeded,path:{},profile:{}\n", filePath, profile)));
	//もう使わないリソースを解放
	shaderSource->Release();
	shaderResult->Release();
	//実行用のバイナリを返却
	return shaderBlob;
}

Microsoft::WRL::ComPtr <ID3D12Resource> CreateBufferResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, size_t sizeInBytes) {
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;//UploadHeapを使う
	//頂点リソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	//バッファリソース。テクスチャの場合はまた別の設定をする
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;//リソースのサイズ。今回はVector4を3頂点分
	//バッファの場合はこれらは１にする決まり
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	//頂点リソース用のヒープの設定
	//バッファの場合はこれにする決まり
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	//実際に頂点リソースを作る
	Microsoft::WRL::ComPtr <ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}

Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> CreateDescriptorHeap(const Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

DirectX::ScratchImage LoadTexture(const std::string& filePath)
{
	//テクスチャファイルを読み込んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);

	//ミップマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	//ミップマップ付きのデータを返す
	return mipImages;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const DirectX::TexMetadata& metadata)
{
	//metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);//Textureの幅
	resourceDesc.Height = UINT(metadata.height);//Textureの高さ
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);//mipmapの数
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);//奥行or配列Textureの配列数
	resourceDesc.Format = metadata.format;//TextureのFormat
	resourceDesc.SampleDesc.Count = 1;//サンプリングカウント。１固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);//Textureの次元数。普段使っているのは２次元
	//利用するHeapの設定。非常に特殊な運用。02－04EXで一般的なケース版あり
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;//細かい設定を行う
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;//WriteBackポリシーでCPUアクセス可能
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;//プロセッサの近くに配置

	//Resourceの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,//Heapの設定
		D3D12_HEAP_FLAG_NONE,//Heapの特殊な設定。
		&resourceDesc,//初回のResource。Textureは基本読むだけ
		D3D12_RESOURCE_STATE_COPY_DEST,//データ転送される設定
		nullptr,//Clear最適値。使わないのでnullptr
		IID_PPV_ARGS(&resource));//作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));
	return resource;
}
//データを転送する
[[nodiscard]]//属性
Microsoft::WRL::ComPtr <ID3D12Resource> UploadTextureData(const Microsoft::WRL::ComPtr <ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages, const Microsoft::WRL::ComPtr<ID3D12Device> device, ID3D12GraphicsCommandList* commandList)
{
	//中間リソース
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));
	Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource = CreateBufferResource(device, intermediateSize);
	//コマンドに積む
	UpdateSubresources(commandList, texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}

Microsoft::WRL::ComPtr <ID3D12Resource> CreateDepthStencilTextureResource(const Microsoft::WRL::ComPtr <ID3D12Device>& device, int32_t width, int32_t height)
{
	//生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;//Textureの幅
	resourceDesc.Height = height;//Textureの高さ
	resourceDesc.MipLevels = 1;//mipmapの数
	resourceDesc.DepthOrArraySize = 1;//奥行きor配列Textureの配列数
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//DepthStencilとして利用可能なフォーマット
	resourceDesc.SampleDesc.Count = 1;//サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;//2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//DepthStencilとして使う通知

	//利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;//VRAMに作る

	//深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;//1.0f(最大級)でクリア
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//フォーマット。Resourceと合わせる

	//Resourceの生成
	Microsoft::WRL::ComPtr <ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,//Heapの設定
		D3D12_HEAP_FLAG_NONE,//Heapの特殊な設定。特に無し
		&resourceDesc,//Resourceの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE,//深度値を書き込む状態にしておく
		&depthClearValue,//Clear最適値
		IID_PPV_ARGS(&resource));//作成するResourceポインタのポインタ
	assert(SUCCEEDED(hr));

	return resource;
}

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename)
{
	//中で必要になる変数の宣言
	MaterialData materialData;//構築するMaterialData
	std::string line;//ファイルから読んだ１行を構築するもの
	//ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);//ファイルを開く
	assert(file.is_open());//とりあえず開けなかった事にする
	//実際にファイルを読み、MaterialDataを構築していく
	while (std::getline(file, line))
	{
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		//identifier
		if (identifier == "map_Kd")
		{
			std::string textureFilename;
			s >> textureFilename;
			//
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	//MaterialDataを返す
	return materialData;
}

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename)
{
	//中で必要となる変数の宣言
	ModelData modelData;//構築するModelData
	std::vector<Vector4> positions;//位置
	std::vector<Vector3> normals;//法線
	std::vector<Vector2>texcords;//テクスチャ座標
	std::string line;//ファイルから読んだ１行を格納するもの
	//ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);//ファイルを開く
	assert(file.is_open());//とりあえず開けなかったら止める
	//実際にファイルを読み、ModelDataを構築していく
	while (std::getline(file, line))
	{
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;//先頭の識別子を読む
		//identifierに応じた処理
		if (identifier == "v")
		{
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.x *= -1.0f;
			position.w = 1.0f;
			positions.push_back(position);
		}
		else if (identifier == "vt")
		{
			Vector2 texcord;
			s >> texcord.x >> texcord.y;
			texcord.y = 1.0f - texcord.y;
			texcords.push_back(texcord);
		}
		else if (identifier == "vn")
		{
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1.0f;
			normals.push_back(normal);
		}
		else if (identifier == "f")
		{
			VertexData triangle[3];
			//面は三角形限定。その他は未対応
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex)
			{
				std::string vertexDefinition;
				s >> vertexDefinition;
				//頂点の要素へのindexは「位置/UV/法線」で格納されているので、分解してindexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; element++)
				{
					std::string index;
					std::getline(v, index, '/');//区切りでインデックスを読んでいく
					elementIndices[element] = std::stoi(index);
				}
				//要素へのindexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcord = texcords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];
				/*VertexData vertex = { position,texcord,normal };
				modelData.vertices.push_back(vertex);*/
				triangle[faceVertex] = { position,texcord,normal };
			}
			//頂点を逆順で登録することで、周り順を逆にする
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		}
		else if (identifier == "mtllib")
		{
			//MaterialTeplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			//基本的にdbjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}
	//ModelDataを返す
	return modelData;
}

struct D3DResourceLeakChecker
{
	~D3DResourceLeakChecker()
	{
		//リソースリークチェック
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
		{
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};

//Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	CoInitializeEx(0, COINIT_MULTITHREADED);

	//誰も補足しなかった場合に（Unhandled）、補足する関数
	SetUnhandledExceptionFilter(ExportDump);

	D3DResourceLeakChecker leakCheck;

#ifdef _DEBUG

	Microsoft::WRL::ComPtr <ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		//デバッグレイヤーを有効化する
		debugController->EnableDebugLayer();
		//さらにGPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif 
	{/*
		Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
		Microsoft::WRL::ComPtr<ID3D12Device> device;*/

#pragma region ログ関連
		//ログのディレクトリを用意
		std::filesystem::create_directory("logs");
		//現在時刻を取得（UTC時刻）
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		//ログファイルの名前にコンマ何秒はいらないので、削って秒にする
		std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
		//日本時間（PCの設定時間）に変換
		std::chrono::zoned_time localTime{ std::chrono::current_zone(),nowSeconds };
		//formatを使って年月日_時分秒の文字列に変換
		std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
		//時刻を使ってファイル名を決定
		std::string logFilePath = std::string("logs/") + dateString + ".log";
		//ファイルを作って書き込み準備
		std::ofstream logStream(logFilePath);
#pragma endregion 

		WNDCLASS wc{};
		//ウィンドウプロシージャ
		wc.lpfnWndProc = windowProc;
		//ウィンドウクラス名
		wc.lpszClassName = L"CG2WindowClass";
		//インスタンスハンドル
		wc.hInstance = GetModuleHandle(nullptr);
		//カーソル
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		//ウィンドウクラスを登録する
		RegisterClass(&wc);
		//クライアント領域のサイズ
		const int32_t kClientWidth = 1280;
		const int32_t kClientHeight = 720;
		//ウィンドウを表す構造体にクライアント領域を入れる
		RECT wrc = { 0,0,kClientWidth,kClientHeight };
		//クライアント領域を元に実際のサイズにwrcを変更してもらう
		AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

		HWND hwnd = CreateWindow(
			wc.lpszClassName,//利用するクラス名
			L"CG2",//タイトルバーの文字
			WS_OVERLAPPEDWINDOW,//よく見るウィンドウスタイル
			CW_USEDEFAULT,//表示X座標
			CW_USEDEFAULT,//表示Y座標
			wrc.right - wrc.left,//ウィンドウ横幅
			wrc.bottom - wrc.top,//ウィンドウ縦幅
			nullptr,//親ウィンドウハンドル
			nullptr,//メニューハンドル
			wc.hInstance,//インスタンスハンドル
			nullptr//オプション
		);
		//ウィンドウを表示する
		ShowWindow(hwnd, SW_SHOW);

		MSG msg{};

		//DXGIファクトリーの生成
		Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory = nullptr;
		//関数が成功したかどうかをSUCCEEDEDマクロで判定出来る
		HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
		//初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、どうにも場合が多いのでassertにしておく
		assert(SUCCEEDED(hr));
		//使用するアダプタ用の変数。最初にnullptrを入れておく
		Microsoft::WRL::ComPtr < IDXGIAdapter4> useAdapter = nullptr;
		//良い順にアダプタを頼む
		for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			//アダプタの情報を取得する
			DXGI_ADAPTER_DESC3 adapterDesc{};
			hr = useAdapter->GetDesc3(&adapterDesc);
			assert(SUCCEEDED(hr));//取得出来ないのは一大事
			//ソフトウェアアダプタでなければ採用
			if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
			{
				//採用したアダプタの情報をログに主力。wstringの方なので注意
				Log(ConvertString(std::format(L"Use Adapate:{}\n", adapterDesc.Description)));
				break;
			}
			useAdapter = nullptr;//ソフトウェアアダプタの場合は見なかったことにする
		}
		//適切なアダプタが見つからなかったので起動できない
		assert(useAdapter != nullptr);

		Microsoft::WRL::ComPtr <ID3D12Device> device = nullptr;
		//機能レベルとログ出力用の文字列
		D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
		};
		const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
		//高い順に生成出来るか試していく
		for (size_t i = 0; i < _countof(featureLevels); i++)
		{
			//採用したアダプタでデバイスを生成
			hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));
			//指定した機能レベルでデバイスを生成できたかを確認
			if (SUCCEEDED(hr))
			{
				//生成出来たのでログ出力を行ってループを抜ける
				Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
				break;
			}
		}
		//デバイスの生成が上手く行かなかったので起動できない
		assert(device != nullptr);
		Log("Complete create D3D12Device!!!\n");//初期化完了のログを出す

#ifdef _DEBUG

		Microsoft::WRL::ComPtr <ID3D12InfoQueue> infoQueue = nullptr;
		if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
		{
			//ヤバいエラー時に止まる
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			//エラー時に止まる
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			//警告時に止まる
			/*infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);*/

			//抑制するメッセージのID
			D3D12_MESSAGE_ID denyIds[] = {
				//Windows１１でのDXGIデバッグレイヤーとDX１２デバッグレイヤーの相互作用バグによるエラーメッセージ
				D3D12_MESSAGE_ID_RESOURCE_BARRIER_INVALID_COMMAND_LIST_TYPE
			};
			//抑制するレベル
			D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
			D3D12_INFO_QUEUE_FILTER filter{};
			filter.DenyList.NumIDs = _countof(denyIds);
			filter.DenyList.pIDList = denyIds;
			filter.DenyList.NumSeverities = _countof(severities);
			filter.DenyList.pSeverityList = severities;
			//指定したメッセージの表示を抑制する
			infoQueue->PushStorageFilter(&filter);
		}

#endif 

#pragma region 画面関連
		Microsoft::WRL::ComPtr <ID3D12CommandQueue> commandQueue = nullptr;
		D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
		hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
		//コマンドキューの生成が上手くいかなかったので起動できない
		assert(SUCCEEDED(hr));
		//コマンドアロケータを生成する
		Microsoft::WRL::ComPtr <ID3D12CommandAllocator> commandAllocator = nullptr;
		hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
		//コマンドアロケータを生成が上手くいかなかったので起動できない
		assert(SUCCEEDED(hr));

		//コマンドリストを生成する
		Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> commandList = nullptr;
		hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
		//コマンドリストの生成がうまくいかなかったので起動できない
		assert(SUCCEEDED(hr));
		//スワップチェーンを生成する
		Microsoft::WRL::ComPtr <IDXGISwapChain4> swapChain = nullptr;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
		swapChainDesc.Width = kClientWidth;//画面の幅。ウィンドウのクライアント領域を同じものにしておく
		swapChainDesc.Height = kClientHeight;//画面の高さ。ウィンドウのクライアント領域を同じものにしておく
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//色の形式
		swapChainDesc.SampleDesc.Count = 1;//マルチサンプルしない
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;//描画のターゲットとして利用する
		swapChainDesc.BufferCount = 2;//ダブルバッファ
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;//モニタに移したら、中身を廃棄
		//コマンドキュー、ウィンドウハンドル、設定を渡して生成する
		hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
		assert(SUCCEEDED(hr));

		//SwapChainからResourceを引っ張ってくる
		Microsoft::WRL::ComPtr <ID3D12Resource> swapChainResources[2] = { nullptr };
		hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
		//うまく取得できなければ起動しない
		assert(SUCCEEDED(hr));
		hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
		assert(SUCCEEDED(hr));

		//RTV用のヒープでディスクリプタの数は２。RTVはShader内で触るものではないので、ShaderVisibleはfalse
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
		//SRV用のヒープでディスクリプタの数は１２８。SRVはShaderないで触るものなのでShaderVisibleはtrue
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

		//RTVの設定
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//出力結果をSRGBに変換して書き込む
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;//2dテクスチャとして読み込む
		//ディスクリプタの先頭を取得する
		D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		//RTVを二つ作るのでディスクリプタを２つ用意
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
		//まず一つ目を作る。一つ目は最初の所に作る。作る場所をこちらで指定してあげる必要がある
		rtvHandles[0] = rtvStartHandle;
		device->CreateRenderTargetView(swapChainResources[0].Get(), &rtvDesc, rtvHandles[0]);
		//２つ目のディスクリプタハンドルを得る【自力で）
		rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		//２つ目を作る
		device->CreateRenderTargetView(swapChainResources[1].Get(), &rtvDesc, rtvHandles[1]);

		//これから書き込むバックアップのインデックスを取得
		UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

		//初期値0でFenceを作る
		Microsoft::WRL::ComPtr <ID3D12Fence> fence = nullptr;
		uint64_t fenceValue = 0;
		hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		assert(SUCCEEDED(hr));

		//FenceのSignalを待つためのイベントを作成する
		HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		assert(fenceEvent != nullptr);

		//dxCompilerを初期化
		Microsoft::WRL::ComPtr <IDxcUtils> dxcUtils = nullptr;
		Microsoft::WRL::ComPtr <IDxcCompiler3> dxcCompiler = nullptr;
		hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
		assert(SUCCEEDED(hr));
		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
		assert(SUCCEEDED(hr));

		//現時点でincludeはしないが、includeに対応するための設定を行っておく
		Microsoft::WRL::ComPtr <IDxcIncludeHandler> includeHandler = nullptr;
		hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
		assert(SUCCEEDED(hr));

		//RootSignature作成
		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
		descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
		descriptorRange[0].BaseShaderRegister = 0;//0から始まる
		descriptorRange[0].NumDescriptors = 1;//数は1つ
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//SRVを使う
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;//offsetを自動計算

		//RootSignature作成。複数設定出来るので配列。今回は結果が一つだけなので長さ１の配列
		D3D12_ROOT_PARAMETER rootParameters[4] = {};
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//CBVを使う
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderで使う
		rootParameters[0].Descriptor.ShaderRegister = 0;//レジスタ番
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//CBVを使う
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;//VertexShaderで使う
		rootParameters[1].Descriptor.ShaderRegister = 0;//レジスタ番号０とバインド
		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;//DescriptorTableを使う
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderで使う
		rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;//Tableの中身配列を指定
		rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);//Tableで利用する数
		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//CBVを使う
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderで使う
		rootParameters[3].Descriptor.ShaderRegister = 1;//レジスタ番号1を使う
		descriptionRootSignature.pParameters = rootParameters;//ルートパラメータ配列へのポインタ
		descriptionRootSignature.NumParameters = _countof(rootParameters);//配列の長さ

		D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
		staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;//バイリニアフィルタ
		staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//０～１なお範囲外をリピート
		staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//比較しない
		staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;//ありったけのMipmapを使う
		staticSamplers[0].ShaderRegister = 0;//レジスタ番号０うぃ使う
		staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderで使う
		descriptionRootSignature.pStaticSamplers = staticSamplers;
		descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

		//シリアライズしてバイナリにする
		Microsoft::WRL::ComPtr <ID3D10Blob> signatureBlob = nullptr;
		Microsoft::WRL::ComPtr <ID3D10Blob> errorBlob = nullptr;
		hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		if (FAILED(hr))
		{
			Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
			assert(false);
		}
		//バイナリを元に生成
		Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature = nullptr;
		hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
		assert(SUCCEEDED(hr));

		//InputLayout
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
		inputElementDescs[0].SemanticName = "POSITION";
		inputElementDescs[0].SemanticIndex = 0;
		inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElementDescs[1].SemanticName = "TEXCOORD";
		inputElementDescs[1].SemanticIndex = 0;
		inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElementDescs[2].SemanticName = "NORMAL";
		inputElementDescs[2].SemanticIndex = 0;
		inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		D3D12_INPUT_LAYOUT_DESC inputLayiutDesc{};
		inputLayiutDesc.pInputElementDescs = inputElementDescs;
		inputLayiutDesc.NumElements = _countof(inputElementDescs);

		//BlendStateの設定
		D3D12_BLEND_DESC blendDesc{};
		//全ての色要素を書き込む
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		//RasterizeStateの設定
		D3D12_RASTERIZER_DESC rasterizeDesc{};
		//裏面（時計周り）を表示しない
		rasterizeDesc.CullMode = D3D12_CULL_MODE_BACK;
		//三角形の中を塗りつぶす
		rasterizeDesc.FillMode = D3D12_FILL_MODE_SOLID;

		//Shaderをコンパイルする
		Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = CompileShader(L"Object3d.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
		assert(vertexShaderBlob != nullptr);

		Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob = CompileShader(L"Object3d.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
		assert(pixelShaderBlob != nullptr);

		//PSOを生成する
		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
		graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();//RootSignature
		graphicsPipelineStateDesc.InputLayout = inputLayiutDesc;//nputLayout
		graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),vertexShaderBlob->GetBufferSize() };//vertexShader
		graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),pixelShaderBlob->GetBufferSize() };//pixelShader
		graphicsPipelineStateDesc.BlendState = blendDesc;//BlendState
		graphicsPipelineStateDesc.RasterizerState = rasterizeDesc;//RasterizeState
		//書き込むRTVの情報
		graphicsPipelineStateDesc.NumRenderTargets = 1;
		graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		//利用するトポロジ（形状）のタイプ。三角形
		graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		//どのように画面に色を打ち込むかの設定（気にすんな）
		graphicsPipelineStateDesc.SampleDesc.Count = 1;
		graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		//DepthStencilStateの設定
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
		//Depthの機能を有効化する
		depthStencilDesc.DepthEnable = true;
		//書き込みます
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		//比較関数はLessEqual。つまり、近ければ描画される
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		//DepthStencilの設定
		graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
		graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		//実際に生成
		Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicsPipelineState = nullptr;
		hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
		assert(SUCCEEDED(hr));

		//----------
		//三角形
		//----------

		//モデル読み込み
		ModelData modelData = LoadObjFile("resources", "axis.obj");
		//頂点リソースを作る
		Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource = CreateBufferResource(device.Get(), sizeof(VertexData) * modelData.vertices.size());

		//頂点バッファビューを生成する
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
		//リソースの先頭のアドレスから使う
		vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
		//使用するリソースのサイズは頂点のサイズ
		vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
		//１頂点あたりのサイズ
		vertexBufferView.StrideInBytes = sizeof(VertexData);

		//頂点リソースにデータを書き込む
		VertexData* vertexData = nullptr;
		//書き込むためのアドレスを取得
		vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
		//頂点データをリリースにコピー
		std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());

		vertexResource->Unmap(0, nullptr);

		////左下
		//vertexData[0].position = { -0.5f,-0.5f,0.0f,1.0f };
		//vertexData[0].texcoord = { 0.0f,1.0f };
		////上
		//vertexData[1].position = { 0.0f,0.5f,0.0f,1.0f };
		//vertexData[1].texcoord = { 0.5f,0.0f };
		////右下
		//vertexData[2].position = { 0.5f,-0.5f,0.0f,1.0f };
		//vertexData[2].texcoord = { 1.0f,1.0f };

		////左下2
		//vertexData[3].position = { -0.5f,-0.5f,0.5f,1.0f };
		//vertexData[3].texcoord = { 0.0f,1.0f };
		////上2
		//vertexData[4].position = { 0.0f,0.0f,0.0f,1.0f };
		//vertexData[4].texcoord = { 0.5f,0.0f };
		////右下2
		//vertexData[5].position = { 0.5f,-0.5f,-0.5f,1.0f };
		//vertexData[5].texcoord = { 1.0f,1.0f };

		//----------
		//球
		//----------

		const uint32_t kSubdivision = 16; // 分割数
		const uint32_t kVertexCountSphere = (kSubdivision + 1) * (kSubdivision + 1);// 重複を排除した頂点数 (289)

		Microsoft::WRL::ComPtr <ID3D12Resource> vertexResourceSphere = CreateBufferResource(device.Get(), sizeof(VertexData) * kVertexCountSphere);

		D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSphereMesh{};
		vertexBufferViewSphereMesh.BufferLocation = vertexResourceSphere->GetGPUVirtualAddress();
		vertexBufferViewSphereMesh.SizeInBytes = sizeof(VertexData) * kVertexCountSphere;
		vertexBufferViewSphereMesh.StrideInBytes = sizeof(VertexData);

		VertexData* vertexDataSphere = nullptr;
		vertexResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSphere));

		const float pi = 3.1415926535f;
		// 1. まず重複のない頂点データをすべて生成して詰める
		for (uint32_t latIndex = 0; latIndex <= kSubdivision; ++latIndex) {
			float lat = -pi / 2.0f + (pi / float(kSubdivision)) * latIndex;
			for (uint32_t lonIndex = 0; lonIndex <= kSubdivision; ++lonIndex) {
				float lon = (pi * 2.0f / float(kSubdivision)) * lonIndex;

				// 頂点インデックスの算出
				uint32_t vIndex = latIndex * (kSubdivision + 1) + lonIndex;

				// 座標と法線
				Vector4 pos = { cos(lat) * cos(lon), sin(lat), cos(lat) * sin(lon), 1.0f };
				vertexDataSphere[vIndex].position = pos;
				vertexDataSphere[vIndex].normal = { pos.x, pos.y, pos.z };

				// UV座標
				float u = float(lonIndex) / float(kSubdivision);
				float v = 1.0f - (float(latIndex) / float(kSubdivision));
				vertexDataSphere[vIndex].texcoord = { u, v };
			}
		}

		//----------
		//四角形
		//----------

		Microsoft::WRL::ComPtr <ID3D12Resource> vertexResourceSprite = CreateBufferResource(device.Get(), sizeof(VertexData) * 4);

		//頂点バッファビューを生成する
		D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
		//リソースの先頭のアドレスから使う
		vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
		//使用するリソースのサイズは頂点３つ分のサイズ
		vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 4;
		//１頂点あたりのサイズ
		vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

		//頂点リソースにデータを書き込む
		VertexData* vertexDataSprite = nullptr;
		//書き込むためのアドレスを取得
		vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
		//左下
		vertexDataSprite[0].position = { 0.0f,360.0f,0.0f,1.0f };
		vertexDataSprite[0].texcoord = { 0.0f,1.0f };
		//左上
		vertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f };
		vertexDataSprite[1].texcoord = { 0.0f,0.0f };
		//右下
		vertexDataSprite[2].position = { 640.0f,360.5f,0.0f,1.0f };
		vertexDataSprite[2].texcoord = { 1.0f,1.0f };
		//右上
		vertexDataSprite[3].position = { 640.0f,0.0f,0.0f,1.0f };
		vertexDataSprite[3].texcoord = { 1.0f,0.0f };

		//Sprite用のtransformationMatrix用のリソースを作る。Matrix4x4　１つ分のサイズを用意する
		Microsoft::WRL::ComPtr <ID3D12Resource> transformationMatrixResourceSprite = CreateBufferResource(device.Get(), sizeof(TransformationMatrix));
		//データを書き込む
		TransformationMatrix* transformationMatrixDataSprite = nullptr;
		//書き込むためのアドレスを取得
		transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
		//単位行列を書き込んでいく
		transformationMatrixDataSprite->WVP = MakeIdentity4x4();
		transformationMatrixDataSprite->World = MakeIdentity4x4();

		//マテリアル用リソースを作る。今回はcolor１つ分のサイズを用意する
		Microsoft::WRL::ComPtr <ID3D12Resource> materialResource = CreateBufferResource(device.Get(), sizeof(Material));
		//マテリアルにデータを読み込む
		Material* materialData = nullptr;
		//書き込むためのアドレスを取得
		materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
		//今回は赤を書き込んでみる
		materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		materialData->enableLighing = true;

		//ビューポート
		D3D12_VIEWPORT viewport{};
		//クライアント領域のサイズと一緒にして画面全体に表示
		viewport.Width = kClientWidth;
		viewport.Height = kClientHeight;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		//シザー矩形
		D3D12_RECT scissorRect{};
		//基本的にはビューポートと同じ矩形が構成されるようになる
		scissorRect.left = 0;
		scissorRect.right = kClientWidth;
		scissorRect.top = 0;
		scissorRect.bottom = kClientHeight;

		//WVP用のリソースを作る。Matrix4x4　１つ分のサイズを用意する
		Microsoft::WRL::ComPtr <ID3D12Resource> wvpResource = CreateBufferResource(device.Get(), sizeof(TransformationMatrix));
		//データを書き込む
		TransformationMatrix* wvpData = nullptr;
		//書き込むためのアドレスを取得
		wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
		//単位行列を書き込んでおく
		wvpData->WVP = MakeIdentity4x4();
		wvpData->World = MakeIdentity4x4();

		Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };

		Transform transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,0.0f} };

		Transform transformSphere{ {0.3f, 0.3f, 0.3f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

		Transform cameraTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,-5.0f} };

		Transform uvTransformSprite{
			{1.0f,1.0f,1.0f},
			{0.0f,0.0f,0.0f},
			{0.0f,0.0f,0.0f},
		};

		//Textureを読んで転送する
		DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
		const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = CreateTextureResource(device, metadata);
		Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource = UploadTextureData(textureResource.Get(), mipImages, device.Get(), commandList.Get());

		//2枚目のTextureを読んで転送する
		DirectX::ScratchImage mipImages2 = LoadTexture(modelData.material.textureFilePath/*"resources/monsterBall.png"*/);
		const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 = CreateTextureResource(device, metadata2);
		Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource2 = UploadTextureData(textureResource2.Get(), mipImages2, device.Get(), commandList.Get());

		//コマンドリストの内容確定させる。すべてのコマンドを積んでからCloseすること
		hr = commandList->Close();
		assert(SUCCEEDED(hr));

		//GPUにコマンドリストの実行を行わせる
		ID3D12CommandList* commandLists[] = { commandList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);

		//Fenceの値を更新
		fenceValue++;
		//GPUがここまでたどりついたときに、Fenceの値を指定した値に代入するようにSignalを送る
		commandQueue->Signal(fence.Get(), fenceValue);

		//Fenceの値が指定したSignal値にたどり着いているか確認する
		//GetCompletedValueFenceの初期値はFence作成時に渡した初期値
		if (fence->GetCompletedValue() < fenceValue)
		{
			//指定したSignalにたどり着いでいないので、たどり着くまで待つようにイベントを設定する
			fence->SetEventOnCompletion(fenceValue, fenceEvent);
			//イベントを待つ
			WaitForSingleObject(fenceEvent, INFINITE);
		}

		//次のフレーム用のコマンドリストを準備
		hr = commandAllocator->Reset();
		assert(SUCCEEDED(hr));
		hr = commandList->Reset(commandAllocator.Get(), nullptr);
		assert(SUCCEEDED(hr));

		//metDataを基にSRVの設定
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDescc{};
		srvDescc.Format = metadata.format;
		srvDescc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDescc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
		srvDescc.Texture2D.MipLevels = UINT(metadata.mipLevels);

		//metDataを基にSRVの設定2
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDescc2{};
		srvDescc2.Format = metadata2.format;
		srvDescc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDescc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
		srvDescc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

		const uint32_t desriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		const uint32_t desriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		const uint32_t desriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		GetCPUDescriptorHandle(rtvDescriptorHeap, desriptorSizeRTV, 0);

		// 1枚目（uvChecker）は index 1 に配置（index 0 は ImGui 用）
		D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = GetCPUDescriptorHandle(srvDescriptorHeap, desriptorSizeSRV, 1);
		D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = GetGPUDescriptorHandle(srvDescriptorHeap, desriptorSizeSRV, 1);
		device->CreateShaderResourceView(textureResource.Get(), &srvDescc, textureSrvHandleCPU);

		// 2枚目（monsterBall）は index 2 に配置
		D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescriptorHeap, desriptorSizeSRV, 2);
		D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescriptorHeap, desriptorSizeSRV, 2);
		device->CreateShaderResourceView(textureResource2.Get(), &srvDescc2, textureSrvHandleCPU2);

		//DepthStencilTextureをウィンドウのサイズ
		Microsoft::WRL::ComPtr <ID3D12Resource> depthStencilResource = CreateDepthStencilTextureResource(device, kClientWidth, kClientHeight);

		//DSV
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

		//DSV
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//Format。基本的にはREcourceに合わせる
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2dTexture
		//DSVHeapの先頭にDSVを作る
		device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		//球
		Microsoft::WRL::ComPtr <ID3D12Resource> wvpResourceSphere = CreateBufferResource(device.Get(), sizeof(TransformationMatrix));
		TransformationMatrix* wvpDataSphere = nullptr;
		wvpResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&wvpDataSphere));

		bool useMonsterBall = true;

		//Sprite用のマテリアルリソースを作る
		Microsoft::WRL::ComPtr <ID3D12Resource> materialResourceSprite = CreateBufferResource(device.Get(), sizeof(Material));

		//Mapしてデータを書き込む
		Material* materialDataSprite = nullptr;
		materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
		//色は白を設定
		materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		//SpriteはLighingしないのでfalseを設定する
		materialDataSprite->enableLighing = false;

		// ライト用のリソースを作成
		Microsoft::WRL::ComPtr <ID3D12Resource> directionalLightResource = CreateBufferResource(device.Get(), sizeof(DirectionalLight));

		// データを書き込むためのポインタ
		DirectionalLight* directionalLightData = nullptr;
		directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

		//ライトの初期値を設定する
		directionalLightData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);        // 白い光
		Vector3 lightDir = { 1.0f, -1.0f, 1.0f }; // 斜め下を向く光の例
		directionalLightData->direction = Normalize(lightDir);  // 真上から真下へ照らす
		directionalLightData->intensity = 1.0f;                               // 輝度1.0

		Microsoft::WRL::ComPtr <ID3D12Resource> indexResourceSprite = CreateBufferResource(device.Get(), sizeof(uint32_t) * 6);
		D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
		//リソースの先端のアドレスから使う
		indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
		//使用するリソースのサイズはインデックス6つ分のサイズ
		indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
		//インデックスはuint32_tとする
		indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

		//インデックスリソースにデータを書き込む
		uint32_t* indexDataSprite = nullptr;
		indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
		indexDataSprite[0] = 0;
		indexDataSprite[1] = 1;
		indexDataSprite[2] = 2;
		indexDataSprite[3] = 1;
		indexDataSprite[4] = 3;
		indexDataSprite[5] = 2;

		//-------------
		//球
		//-------------
		const uint32_t kIndexCountSphere = kSubdivision * kSubdivision * 6;// 球体の全頂点数(1536)

		Microsoft::WRL::ComPtr <ID3D12Resource> indexResourceSphere = CreateBufferResource(device.Get(), sizeof(uint32_t) * kIndexCountSphere);
		D3D12_INDEX_BUFFER_VIEW indexBufferViewSphere{};
		indexBufferViewSphere.BufferLocation = indexResourceSphere->GetGPUVirtualAddress();
		indexBufferViewSphere.SizeInBytes = sizeof(uint32_t) * kIndexCountSphere;
		indexBufferViewSphere.Format = DXGI_FORMAT_R32_UINT;

		uint32_t* indexDataSphere = nullptr;

		indexResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSphere));

		uint32_t iIndex = 0;
		for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
			for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
				uint32_t lb = latIndex * (kSubdivision + 1) + lonIndex;
				uint32_t rb = lb + 1;
				uint32_t lt = (latIndex + 1) * (kSubdivision + 1) + lonIndex;
				uint32_t rt = lt + 1;

				indexDataSphere[iIndex++] = lb;
				indexDataSphere[iIndex++] = lt;
				indexDataSphere[iIndex++] = rb;

				indexDataSphere[iIndex++] = rb;
				indexDataSphere[iIndex++] = lt;
				indexDataSphere[iIndex++] = rt;
			}
		}

		materialData->uvTransform = MakeIdentity4x4();
		materialDataSprite->uvTransform = MakeIdentity4x4();

#pragma endregion
#pragma region IMGUI
#ifdef USE_IMGUI
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX12_Init(device.Get(),
			swapChainDesc.BufferCount,
			rtvDesc.Format,
			srvDescriptorHeap.Get(),
			srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->Build();
#endif // USE_IMGUI
#pragma endregion

		//ウィンドウのｘボタンが押されるまでループ
		while (msg.message != WM_QUIT)
		{

			//Windowにメッセージが来てたら最優先で処理させる
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				//ゲームの処理
				/*uint32_t* p = nullptr;
				*p = 100;*/
#ifdef USE_IMGUI
				ImGui_ImplDX12_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();

				ImGui::Begin("Object Control");
				// 位置調整
				ImGui::SliderFloat2("Sprite Position", &transformSprite.translate.x, 0.0f, 1280.0f);
				ImGui::SliderFloat3("Sphere Position", &transformSphere.translate.x, -5.0f, 5.0f);

				ImGui::SliderAngle("OBJ Rotate X", &transform.rotate.x);
				ImGui::SliderAngle("OBJ Rotate Y", &transform.rotate.y);
				ImGui::SliderAngle("OBJ Rotate Z", &transform.rotate.z);

				ImGui::Checkbox("useMonsterBall", &useMonsterBall);
				static float lightDirection[3] = { 1.0f, -1.0f, 1.0f };
				if (ImGui::SliderFloat3("Light Direction", lightDirection, -1.0f, 1.0f)) {
					// スライダーが動いたら、正規化（Normalize）してデータを更新
					Vector3 rawDir = { lightDirection[0], lightDirection[1], lightDirection[2] };
					directionalLightData->direction = Normalize(rawDir);
				}

				// 2. ライトの輝度（幅・強さ）
				ImGui::SliderFloat("Light Intensity", &directionalLightData->intensity, 0.0f, 5.0f);

				ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
				ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
				ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);

				ImGui::End();

				//開発用UIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
				/*ImGui::ShowDemoWindow();*/

				//ImGuiの内部のコマンドを生成する
				ImGui::Render();
#endif // USE_IMGUI

				//描画用のDescriptorHeapの設定
				ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap.Get() };
				commandList->SetDescriptorHeaps(1, descriptorHeaps);

#pragma region 三角形の処理

				/*transform.rotate.y += 0.03f;*/
				Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

				Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
				Matrix4x4 viewMatrix = Inverse(cameraMatrix);
				Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
				Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
				wvpData->WVP = worldViewProjectionMatrix;
				wvpData->World = worldMatrix;

				//Sprite用のworldViewProjectionMatrixを作る
				Matrix4x4 worldMattrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
				Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
				Matrix4x4 projectionMatrixSprite = MakeOrthograhicMatrix(0.0f, 0.0f, float(kClientWidth), float(kClientHeight), 0.0f, 100.0f);
				Matrix4x4 worldViewProjectionMatrixSprite = Multiply(worldMattrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));
				transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;
				transformationMatrixDataSprite->World = MakeIdentity4x4();

				//これから書き込むバックアップのインデックスを取得
				UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

				//TransitionBarrierの設定
				D3D12_RESOURCE_BARRIER barrier{};
				//今回のバリアはTransition
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				//Noneにしておく
				barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				//バリアを張る対象のリソース。現在のバックバッファに対して行う
				barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
				//遷移前（現在）ResourceState
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
				//遷移後のResourceState
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
				//TransitionBarrierを張る
				commandList->ResourceBarrier(1, &barrier);

				//描画先のRTVを設定する
				commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);
				//指定した色で画面全体をクリアする
				float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };//
				commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

				//描画先のRTVとDSVｗｐ設定する
				D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);

				//指定した深度で画面全体をクリアする
				commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

				commandList->RSSetViewports(1, &viewport);//Viewportを設定
				commandList->RSSetScissorRects(1, &scissorRect);//ScissorRectを設定
				//RootSignatureを設定。PSOに設定しているけど別途設定が必要
				commandList->SetGraphicsRootSignature(rootSignature.Get());
				commandList->SetPipelineState(graphicsPipelineState.Get());//PSOを設定
				commandList->IASetVertexBuffers(0, 1, &vertexBufferView);//VBVを設定
				//形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い。
				commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				//マテリアルCBufferの場所を設定
				commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
				//wvp用のCBufferの場所を設定
				commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
				//SRVのDescriptorTableの先頭を設定。2はrootRarmeter[2]である
				//描画のテクスチャを読む所
				commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
				//ライト
				commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
				//描画(DrawCall/ドローコール)。３頂点で一つのインスタンス。インスタンスについては今後
				commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

				//-----------
				//球
				//-----------
				transformSphere.rotate.y += 0.02f;

				Matrix4x4 worldMatrixSphere = MakeAffineMatrix(transformSphere.scale, transformSphere.rotate, transformSphere.translate);
				// カメラやプロジェクション行列は共通のもの(viewMatrix, projectionMatrix)を使い回す
				Matrix4x4 worldViewProjectionMatrixSphere = Multiply(worldMatrixSphere, Multiply(viewMatrix, projectionMatrix));
				wvpDataSphere->WVP = worldViewProjectionMatrixSphere;
				wvpDataSphere->World = worldMatrixSphere;

				commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSphereMesh);

				commandList->IASetIndexBuffer(&indexBufferViewSphere);
				//追加？
				commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
				commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

				commandList->SetGraphicsRootConstantBufferView(1, wvpResourceSphere->GetGPUVirtualAddress());
				commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);
				/*commandList->DrawInstanced(kVertexCountSphere, 1, 0, 0);*/
				commandList->DrawIndexedInstanced(kIndexCountSphere, 1, 0, 0, 0);

				//-----------
				//四角形
				//-----------

				commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
				//Spriteの描画。
				commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);//VBVを設定

				commandList->IASetIndexBuffer(&indexBufferViewSprite);//IBVを設定

				//なんか入れろと
				commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());

				//TransformationMatrixCBufferの場所を設定
				commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
				//ライト
				commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

				//描画(DrawCall/ドローコール)６個のインデックスを使用し１つのインスタンスを描画。その他は当面０で良い
				commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

				Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
				uvTransformMatrix = Multiply(uvTransformMatrix, MakeRotateZMatrix(uvTransformSprite.rotate.z));
				uvTransformMatrix = Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
				materialDataSprite->uvTransform = uvTransformMatrix;

#ifdef USE_IMGUI
				//実際のcommandListのImGuiの描画コマンドを積む
				ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
#endif // USE_IMGUI

				//画面に描く処理は全て終わり、画面に移すので、状態を遷移
				//今回はRenderTargetからPresentにする
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
				//TransitionBarrierを張る
				commandList->ResourceBarrier(1, &barrier);

				//コマンドリストの内容確定させる。すべてのコマンドを積んでからCloseすること
				hr = commandList->Close();
				assert(SUCCEEDED(hr));

				//GPUにコマンドリストの実行を行わせる
				ID3D12CommandList* commandLists[] = { commandList.Get() };
				commandQueue->ExecuteCommandLists(1, commandLists);
				//GPUとOSに画面の交換を行うように通知する
				swapChain->Present(1, 0);

				//Fenceの値を更新
				fenceValue++;
				//GPUがここまでたどりついたときに、Fenceの値を指定した値に代入するようにSignalを送る
				commandQueue->Signal(fence.Get(), fenceValue);

				//Fenceの値が指定したSignal値にたどり着いているか確認する
				//GetCompletedValueFenceの初期値はFence作成時に渡した初期値
				if (fence->GetCompletedValue() < fenceValue)
				{
					//指定したSignalにたどり着いでいないので、たどり着くまで待つようにイベントを設定する
					fence->SetEventOnCompletion(fenceValue, fenceEvent);
					//イベントを待つ
					WaitForSingleObject(fenceEvent, INFINITE);
				}

				//次のフレーム用のコマンドリストを準備
				hr = commandAllocator->Reset();
				assert(SUCCEEDED(hr));
				hr = commandList->Reset(commandAllocator.Get(), nullptr);
				assert(SUCCEEDED(hr));
#pragma endregion
			}
		}
#ifdef USE_IMGUI
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
#endif // USE_IMGUI

		//出力ウィンドウの文字出力
		OutputDebugStringA("Hello,DirectX!\n");

		CloseHandle(fenceEvent);
		
		CloseWindow(hwnd);
	}
	CoUninitialize();
	return 0;
}