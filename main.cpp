#include "MyMath.h"
#include <Windows.h>
//ファイルやディレクトリに関する操作を行うライブラリ
#include <filesystem>
//ファイルを書いたり読んだりするライブラリ
#include <fstream>
//時間を扱うライブラリ
#include <chrono>
#include <format>
#include <cassert>
#include <dbghelp.h>
#include <strsafe.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <xaudio2.h>
#define DIRECTINPUT_VERSION       0x0800//DirectInputのバージョン指定
#include <dinput.h>
#include "keyInput.h"
#include "DebugCamera.h"
#include "WindowsApp.h"
#include "DirectXBasics.h"
#include "D3DResourceLeakChecker.h"
#include <random>
#include <numbers>

#pragma comment(lib,"d3d12.lib") 
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"Dbghelp.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")
#pragma comment(lib,"xaudio2.lib")
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

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

//チャンクヘッダ
struct ChunkHeader
{
	char id[4];//チャンク毎のID
	int32_t size;//チャンクサイズ
};
//RIFFヘッダチャンク
struct RiffHeader
{
	ChunkHeader chunk;//"RIFF"
	char type[4];//"WAVE"
};
//FMTチャンク
struct FormatChunk
{
	ChunkHeader chunk;//"fmt"
	WAVEFORMATEX fmt;//波形フォーマット
};
//音声データ
struct SoundData
{
	//波形フォーマット
	WAVEFORMATEX wfex;
	//バッファの先頭サイズ
	BYTE* pBuffer;
	//バッファのサイズ
	unsigned int bufferSize;
};
//音声データ
SoundData SoundLoadWave(const char* filename)
{
	//ファイルオープン
	//ファイル入力ストリームのインスタンス
	std::ifstream file;
	//.wavファイルをバイナリモードで開く
	file.open(filename, std::ios_base::binary);
	//ファイルオープン失敗を検出する
	assert(file.is_open());
	//.wavデータ読み込み
	//RIFFヘッダーの読み込み
	RiffHeader riff;
	file.read((char*)&riff, sizeof(riff));
	//ファイルがRIFFかチェック
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0)
	{
		assert(0);
	}
	//タイプがWAVEかチェック
	if (strncmp(riff.type, "WAVE", 4) != 0)
	{
		assert(0);
	}
	//Formatチャンクの読み込み
	FormatChunk format = {};
	//チャンクヘッダーの確認
	file.read((char*)&format, sizeof(ChunkHeader));
	if (strncmp(format.chunk.id, "fmt ", 4) != 0)
	{
		assert(0);
	}

	//チャンク本体の読み込み
	assert(format.chunk.size <= sizeof(format.fmt));
	file.read((char*)&format.fmt, format.chunk.size);

	//Dataチャンクの読み込み
	ChunkHeader data;
	file.read((char*)&data, sizeof(data));
	//JUNKチャンクを検出した場合
	if (strncmp(data.id, "JUNK", 4) == 0)
	{
		//読み取り位置をJUNKチャンクの終わりまで進める
		file.seekg(data.size, std::ios_base::cur);
		//再読み込み
		file.read((char*)&data, sizeof(data));
	}

	if (strncmp(data.id, "data", 4) != 0)
	{
		assert(0);
	}

	//Dataチャンクのデータ部(波形データ)の読み込み
	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);

	//ファイルクローズ
	//WAVファイルを閉じる
	file.close();

	//読み込んだ音声データをreturn
	//returnする為の音声データ
	SoundData soundData = {};

	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;

	return soundData;
}

//音声データ解放
void SoundUnload(SoundData* soundData)
{
	//バッファのメモリ解放
	delete[] soundData->pBuffer;

	soundData->pBuffer = 0;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}

//音声再生
void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData)
{
	HRESULT hr;

	//波形フォーマットを元にSourceVoiceの生成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	hr = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(hr));

	//再生する波形データの決定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	//波形データの再生
	hr = pSourceVoice->SubmitSourceBuffer(&buf);
	hr = pSourceVoice->Start();
}

struct Particle
{
	Transform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float currentTime;
};

std::random_device seedGenerator;
std::mt19937 randomEngine(seedGenerator());

Particle MakeNewParticle(std::mt19937& randomEngine)
{
	//位置と速度を[-1,1]でランダムに初期化
	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
	std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
	std::uniform_real_distribution<float> distTime(1.0f, 3.0f);
	Particle particle;
	particle.transform.scale = { 1.0f,1.0f,1.0f };
	particle.transform.rotate = { 0.0f,0.0f,0.0f };
	particle.transform.translate = { distribution(randomEngine),distribution(randomEngine),distribution(randomEngine) };
	particle.velocity = { distribution(randomEngine),distribution(randomEngine),distribution(randomEngine) };
	particle.color = { distColor(randomEngine),distColor(randomEngine),distColor(randomEngine),1.0f };
	particle.lifeTime = distTime(randomEngine);
	particle.currentTime = 0;
	return particle;
}

struct ParticalForGPU
{
	Matrix4x4 WVP;
	Matrix4x4 World;
	Vector4 color;
};

//Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	//誰も補足しなかった場合に（Unhandled）、補足する関数
	SetUnhandledExceptionFilter(ExportDump);

	D3DResourceLeakChecker leakCheck;

	WindowsApp* windowsApp = new WindowsApp();
	windowsApp->Initialize();

	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;

	IXAudio2MasteringVoice* masteVoice;

	HRESULT hr;

	HRESULT hrAudio = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(hrAudio));

	hrAudio = xAudio2->CreateMasteringVoice(&masteVoice);
	assert(SUCCEEDED(hrAudio));

	//音声読み込み
	SoundData soundData1 = SoundLoadWave("resources/Alarm01.wav");
	//音声再生
	SoundPlayWave(xAudio2.Get(), soundData1);

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
	{

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

		////ポインタ
		//WindowsApp* windowsApp = nullptr;
		////Windowsの初期化
		//windowsApp = new WindowsApp();
		/*windowsApp->Initialize();*/

		//ポインタ
		DirectXBasics* dxBasics = nullptr;
		//DirectXの初期化
		dxBasics = new DirectXBasics();
		dxBasics->Initialize(windowsApp);

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

		// StructuredBuffer (t0) 用のレンジ設定 (VS用)
		D3D12_DESCRIPTOR_RANGE particleSrvRangeInstancing[1] = {};
		particleSrvRangeInstancing[0].BaseShaderRegister = 0; // t0
		particleSrvRangeInstancing[0].NumDescriptors = 1;
		particleSrvRangeInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		particleSrvRangeInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Texture (t0) 用のレンジ設定 (PS用)
		D3D12_DESCRIPTOR_RANGE particleSrvRangeTexture[1] = {};
		particleSrvRangeTexture[0].BaseShaderRegister = 0; // t0
		particleSrvRangeTexture[0].NumDescriptors = 1;
		particleSrvRangeTexture[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		particleSrvRangeTexture[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER particleRootParameters[3] = {};
		// [0] Material用 CBV (b0) -> PixelShader[cite: 24]
		particleRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		particleRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		particleRootParameters[0].Descriptor.ShaderRegister = 0;

		// [1] Instancing(StructuredBuffer)用 SRV (t0) -> VertexShader[cite: 24]
		particleRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		particleRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		particleRootParameters[1].DescriptorTable.pDescriptorRanges = particleSrvRangeInstancing;
		particleRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;

		// [2] Texture用 SRV (t0) -> PixelShader[cite: 24]
		particleRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		particleRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		particleRootParameters[2].DescriptorTable.pDescriptorRanges = particleSrvRangeTexture;
		particleRootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

		// パーティクル用 RootSignature
		D3D12_ROOT_SIGNATURE_DESC particleRootSignatureDesc{};
		particleRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		particleRootSignatureDesc.pParameters = particleRootParameters;
		particleRootSignatureDesc.NumParameters = _countof(particleRootParameters);
		particleRootSignatureDesc.pStaticSamplers = staticSamplers;
		particleRootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);

		Microsoft::WRL::ComPtr<ID3D10Blob> particleSignatureBlob = nullptr;
		Microsoft::WRL::ComPtr<ID3D10Blob> particleErrorBlob = nullptr;
		hr = D3D12SerializeRootSignature(&particleRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &particleSignatureBlob, &particleErrorBlob);
		assert(SUCCEEDED(hr));

		Microsoft::WRL::ComPtr<ID3D12RootSignature> particleRootSignature = nullptr;
		hr = dxBasics->GetDevice()->CreateRootSignature(0, particleSignatureBlob->GetBufferPointer(), particleSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&particleRootSignature));
		assert(SUCCEEDED(hr));

		//パーティクル用
		D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
		descriptorRangeForInstancing[0].BaseShaderRegister = 0;//0から始まる
		descriptorRangeForInstancing[0].NumDescriptors = 1;//数は1つ
		descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//SRVを使う
		descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		//パーティクル用
		D3D12_ROOT_PARAMETER parrticleRootParameters[1] = {};
		parrticleRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;//DescriptorTableを使う
		parrticleRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;//VertexShaderで使う
		parrticleRootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;//Tableの中身の配列を指定
		parrticleRootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);//Tableで利用する数

		//シリアライズしてバイナリにする
		Microsoft::WRL::ComPtr <ID3D10Blob> signatureBlob = nullptr;
		Microsoft::WRL::ComPtr <ID3D10Blob> errorBlob = nullptr;
		hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		if (FAILED(hr))
		{
			Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
			assert(false);
		}
		//バイナリを元に生成
		Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature = nullptr;
		hr = dxBasics->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
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
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

		//RasterizeStateの設定
		D3D12_RASTERIZER_DESC rasterizeDesc{};
		//裏面（時計周り）を表示しない
		rasterizeDesc.CullMode = D3D12_CULL_MODE_BACK;
		//三角形の中を塗りつぶす
		rasterizeDesc.FillMode = D3D12_FILL_MODE_SOLID;

		//Shaderをコンパイルする
		Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = dxBasics->CompileShader(L"Object3d.VS.hlsl", L"vs_6_0");
		assert(vertexShaderBlob != nullptr);

		Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob = dxBasics->CompileShader(L"Object3d.PS.hlsl", L"ps_6_0");
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
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		//比較関数はLessEqual。つまり、近ければ描画される
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		//DepthStencilの設定
		graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
		graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		//実際に生成
		Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicsPipelineState = nullptr;
		hr = dxBasics->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
		assert(SUCCEEDED(hr));

		//------パーティクル用のPSO------
		Microsoft::WRL::ComPtr<IDxcBlob> particleVSBlob = dxBasics->CompileShader(L"Particle.VS.hlsl", L"vs_6_0");
		assert(particleVSBlob != nullptr);

		Microsoft::WRL::ComPtr<IDxcBlob> particlePSBlob = dxBasics->CompileShader(L"Particle.PS.hlsl", L"ps_6_0");
		assert(particlePSBlob != nullptr);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC particlePipelineStateDesc = graphicsPipelineStateDesc; // 基本設定はコピー
		particlePipelineStateDesc.pRootSignature = particleRootSignature.Get();
		particlePipelineStateDesc.VS = { particleVSBlob->GetBufferPointer(), particleVSBlob->GetBufferSize() };
		particlePipelineStateDesc.PS = { particlePSBlob->GetBufferPointer(), particlePSBlob->GetBufferSize() };

		D3D12_BLEND_DESC particleBlendDesc{};
		particleBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		particleBlendDesc.RenderTarget[0].BlendEnable = TRUE;
		particleBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;  // 元の色のα値を考慮
		particleBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;       // 加算処理
		particleBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;       // 背景色をそのまま100%加算
		particleBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		particleBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		particleBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

		// パーティクル用PSOに設定を適用
		particlePipelineStateDesc.BlendState = particleBlendDesc;

		Microsoft::WRL::ComPtr<ID3D12PipelineState> particlePipelineState = nullptr;
		hr = dxBasics->GetDevice()->CreateGraphicsPipelineState(&particlePipelineStateDesc, IID_PPV_ARGS(&particlePipelineState));
		assert(SUCCEEDED(hr));

		//----------
		//三角形
		//----------

		//モデル読み込み
		ModelData modelData = LoadObjFile("resources", "plane.obj");
		//頂点リソースを作る
		Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource = dxBasics->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());

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

		//----------
		//球
		//----------

		const uint32_t kSubdivision = 16; // 分割数
		const uint32_t kVertexCountSphere = (kSubdivision + 1) * (kSubdivision + 1);// 重複を排除した頂点数 (289)

		Microsoft::WRL::ComPtr <ID3D12Resource> vertexResourceSphere = dxBasics->CreateBufferResource(sizeof(VertexData) * kVertexCountSphere);

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

		Microsoft::WRL::ComPtr <ID3D12Resource> vertexResourceSprite = dxBasics->CreateBufferResource(sizeof(VertexData) * 4);

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
		Microsoft::WRL::ComPtr <ID3D12Resource> transformationMatrixResourceSprite = dxBasics->CreateBufferResource(sizeof(TransformationMatrix));
		//データを書き込む
		TransformationMatrix* transformationMatrixDataSprite = nullptr;
		//書き込むためのアドレスを取得
		transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
		//単位行列を書き込んでいく
		transformationMatrixDataSprite->WVP = MakeIdentity4x4();
		transformationMatrixDataSprite->World = MakeIdentity4x4();

		//マテリアル用リソースを作る。今回はcolor１つ分のサイズを用意する
		Microsoft::WRL::ComPtr <ID3D12Resource> materialResourceObj = dxBasics->CreateBufferResource(sizeof(Material));
		//マテリアルにデータを読み込む
		Material* materialDataObj = nullptr;
		//書き込むためのアドレスを取得
		materialResourceObj->Map(0, nullptr, reinterpret_cast<void**>(&materialDataObj));
		//今回は赤を書き込んでみる
		materialDataObj->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		materialDataObj->enableLighing = true;
		materialDataObj->uvTransform = MakeIdentity4x4();

		//球体マテリアル用リソースを作る。今回はcolor１つ分のサイズを用意する
		Microsoft::WRL::ComPtr <ID3D12Resource> materialResourceSphere = dxBasics->CreateBufferResource(sizeof(Material));
		//マテリアルにデータを読み込む
		Material* materialDataSphere = nullptr;
		//書き込むためのアドレスを取得
		materialResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSphere));
		//今回は赤を書き込んでみる
		materialDataSphere->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		materialDataSphere->enableLighing = true;
		materialDataSphere->uvTransform = MakeIdentity4x4();

		//WVP用のリソースを作る。Matrix4x4　１つ分のサイズを用意する
		Microsoft::WRL::ComPtr <ID3D12Resource> wvpResource = dxBasics->CreateBufferResource(sizeof(TransformationMatrix));
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

		Transform cameraTransform{
			{1.0f,1.0f,1.0f},
			{std::numbers::pi_v<float> / 3.0f,std::numbers::pi_v<float>,0.0f},
			{0.0f,23.0f,10.0f}
		};

		Transform uvTransformSprite{
			{1.0f,1.0f,1.0f},
			{0.0f,0.0f,0.0f},
			{0.0f,0.0f,0.0f},
		};

		//Textureを読んで転送する
		DirectX::ScratchImage mipImagesSprite = DirectXBasics::LoadTexture("resources/uvChecker.png");
		const DirectX::TexMetadata& metadataSprite = mipImagesSprite.GetMetadata();
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResourceSprite = dxBasics->CreateTextureResource(metadataSprite);
		Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResourceSprite = dxBasics->UploadTextureData(textureResourceSprite, mipImagesSprite);

		//OBJ用のTextureを読んで転送する
		DirectX::ScratchImage mipImagesObj = DirectXBasics::LoadTexture(modelData.material.textureFilePath);
		const DirectX::TexMetadata& metadataObj = mipImagesObj.GetMetadata();
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResourceObj = dxBasics->CreateTextureResource(metadataObj);
		Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResourceObj = dxBasics->UploadTextureData(textureResourceObj, mipImagesObj);

		//球体用のテクスチャ
		DirectX::ScratchImage mipImagesSphere = DirectXBasics::LoadTexture("resources/uvChecker.png");
		const DirectX::TexMetadata& metadataSphere = mipImagesSphere.GetMetadata();
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResourceSphere = dxBasics->CreateTextureResource(metadataSphere);
		Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResourceSphere = dxBasics->UploadTextureData(textureResourceSphere, mipImagesSphere);

		//パーティクル
		DirectX::ScratchImage mipImagesParticle = DirectXBasics::LoadTexture("resources/circle.png");
		const DirectX::TexMetadata& metadataParticle = mipImagesParticle.GetMetadata();
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResourceParticle = dxBasics->CreateTextureResource(metadataParticle);
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResourceParticle = dxBasics->UploadTextureData(textureResourceParticle, mipImagesParticle);

		//metDataを基にSRVの設定スプライト
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesccSprite{};
		srvDesccSprite.Format = metadataSprite.format;
		srvDesccSprite.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesccSprite.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
		srvDesccSprite.Texture2D.MipLevels = UINT(metadataSprite.mipLevels);

		//metDataを基にSRVの設定OBJ
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesccObj{};
		srvDesccObj.Format = metadataObj.format;
		srvDesccObj.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesccObj.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
		srvDesccObj.Texture2D.MipLevels = UINT(metadataObj.mipLevels);

		//metDataを基にSRVの設定球体用
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesccSphere{};
		srvDesccSphere.Format = metadataSphere.format;
		srvDesccSphere.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesccSphere.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
		srvDesccSphere.Texture2D.MipLevels = UINT(metadataSphere.mipLevels);

		//パーティクル
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDescParticle{};
		srvDescParticle.Format = metadataParticle.format;
		srvDescParticle.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDescParticle.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDescParticle.Texture2D.MipLevels = UINT(metadataParticle.mipLevels);

		const uint32_t desriptorSizeSRV = dxBasics->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		const uint32_t desriptorSizeRTV = dxBasics->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		const uint32_t desriptorSizeDSV = dxBasics->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		// 1枚目（uvChecker）は index 1 に配置（index 0 は ImGui 用）スプライト
		D3D12_CPU_DESCRIPTOR_HANDLE spriteSrvHandleCPU = dxBasics->GetCPUDescriptorHandle(1);
		D3D12_GPU_DESCRIPTOR_HANDLE spriteSrvHandleGPU = dxBasics->GetGPUDescriptorHandle(1);
		dxBasics->GetDevice()->CreateShaderResourceView(textureResourceSprite.Get(), &srvDesccSprite, spriteSrvHandleCPU);

		// 2枚目（monsterBall）は index 2 に配置OBJ
		D3D12_CPU_DESCRIPTOR_HANDLE objSrvHandleCPU = dxBasics->GetCPUDescriptorHandle(2);
		D3D12_GPU_DESCRIPTOR_HANDLE objSrvHandleGPU = dxBasics->GetGPUDescriptorHandle(2);
		dxBasics->GetDevice()->CreateShaderResourceView(textureResourceObj.Get(), &srvDesccObj, objSrvHandleCPU);

		//3枚目（uvChecker）は index 3に配置（index 0 は ImGui 用）球体
		D3D12_CPU_DESCRIPTOR_HANDLE sphereSrvHandleCPU = dxBasics->GetCPUDescriptorHandle(3);
		D3D12_GPU_DESCRIPTOR_HANDLE sphereSrvHandleGPU = dxBasics->GetGPUDescriptorHandle(3);
		dxBasics->GetDevice()->CreateShaderResourceView(textureResourceSphere.Get(), &srvDesccSphere, sphereSrvHandleCPU);

		//5枚目（circle）は index 3に配置（index 0 は ImGui 用）パーティクル
		D3D12_CPU_DESCRIPTOR_HANDLE particleTextureSrvHandleCPU = dxBasics->GetCPUDescriptorHandle(5);
		D3D12_GPU_DESCRIPTOR_HANDLE particleTextureSrvHandleGPU = dxBasics->GetGPUDescriptorHandle(5);
		dxBasics->GetDevice()->CreateShaderResourceView(textureResourceParticle.Get(), &srvDescParticle, particleTextureSrvHandleCPU);

		//球
		Microsoft::WRL::ComPtr <ID3D12Resource> wvpResourceSphere = dxBasics->CreateBufferResource(sizeof(TransformationMatrix));
		TransformationMatrix* wvpDataSphere = nullptr;
		wvpResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&wvpDataSphere));

		bool useMonsterBall = true;
		bool useBillboard = true;
		bool isParticleActive = false;
		Vector3 particleRotate = { 0.0f, 0.0f, 0.0f };

		//Sprite用のマテリアルリソースを作る
		Microsoft::WRL::ComPtr <ID3D12Resource> materialResourceSprite = dxBasics->CreateBufferResource(sizeof(Material));

		//Mapしてデータを書き込む
		Material* materialDataSprite = nullptr;
		materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
		//色は白を設定
		materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		//SpriteはLighingしないのでfalseを設定する
		materialDataSprite->enableLighing = false;

		// OBJライト用のリソースを作成
		Microsoft::WRL::ComPtr <ID3D12Resource> directionalLightResourceObj = dxBasics->CreateBufferResource(sizeof(DirectionalLight));

		// データを書き込むためのポインタ
		DirectionalLight* directionalLightDataObj = nullptr;
		directionalLightResourceObj->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightDataObj));

		//ライトの初期値を設定する
		directionalLightDataObj->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);        // 白い光
		Vector3 lightDirObj = { 1.0f, -1.0f, 1.0f }; // 斜め下を向く光の例
		directionalLightDataObj->direction = Normalize(lightDirObj);  // 真上から真下へ照らす
		directionalLightDataObj->intensity = 1.0f;                               // 輝度1.0

		// 球体ライト用のリソースを作成
		Microsoft::WRL::ComPtr <ID3D12Resource> directionalLightResourceSphere = dxBasics->CreateBufferResource(sizeof(DirectionalLight));

		// データを書き込むためのポインタ
		DirectionalLight* directionalLightDataSphere = nullptr;
		directionalLightResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightDataSphere));

		//ライトの初期値を設定する
		directionalLightDataSphere->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);        // 白い光
		Vector3 lightDirSphere = { 1.0f, -1.0f, 1.0f }; // 斜め下を向く光の例
		directionalLightDataSphere->direction = Normalize(lightDirSphere);  // 真上から真下へ照らす
		directionalLightDataSphere->intensity = 1.0f;                               // 輝度1.0

		Microsoft::WRL::ComPtr <ID3D12Resource> indexResourceSprite = dxBasics->CreateBufferResource(sizeof(uint32_t) * 6);
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

		Microsoft::WRL::ComPtr <ID3D12Resource> indexResourceSphere = dxBasics->CreateBufferResource(sizeof(uint32_t) * kIndexCountSphere);
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

		materialDataObj->uvTransform = MakeIdentity4x4();
		materialDataSprite->uvTransform = MakeIdentity4x4();

		//----------
		//パーティクル
		//----------
		const uint32_t kNumMaxInstance = 10;//インスタンス数
		//Instancing用のTransfomationMatrixリソースを作る

		Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource = dxBasics->CreateBufferResource(sizeof(ParticalForGPU) * kNumMaxInstance);
		//書き込むためのアドレスを取得
		ParticalForGPU* instancingData = nullptr;
		instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&instancingData));
		//単位行列を書き込んでおく
		for (uint32_t index = 0; index < kNumMaxInstance; ++index)
		{
			instancingData[index].WVP = MakeIdentity4x4();
			instancingData[index].World = MakeIdentity4x4();
			instancingData[index].color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
		instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
		instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		instancingSrvDesc.Buffer.FirstElement = 0;
		instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		instancingSrvDesc.Buffer.NumElements = kNumMaxInstance;
		instancingSrvDesc.Buffer.StructureByteStride = sizeof(ParticalForGPU);

		//パーティクル用のインデックス割り当て
		D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU = dxBasics->GetCPUDescriptorHandle(4);
		D3D12_GPU_DESCRIPTOR_HANDLE intancingSrvHandleGPU = dxBasics->GetGPUDescriptorHandle(4);
		dxBasics->GetDevice()->CreateShaderResourceView(instancingResource.Get(), &instancingSrvDesc, instancingSrvHandleCPU);

		Particle particles[kNumMaxInstance];
		for (uint32_t index = 0; index < kNumMaxInstance; ++index)
		{
			particles[index] = MakeNewParticle(randomEngine);
		}

		const float kDeltaTime = 1.0f / 60.0f;

		//ポインタ
		KeyInput* keyInput = nullptr;
		//入力の初期化
		keyInput = new KeyInput();
		keyInput->Initialize(windowsApp);

		DebugCamera debugCamera;
		debugCamera.Initialize();

#pragma endregion
#pragma region IMGUI
#ifdef USE_IMGUI
		dxBasics->ImGuiInitialize();
#endif // USE_IMGUI
#pragma endregion

		//xAudioエンジンのインスタンスを生成
		hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
		//マスターボイスの生成
		hr = xAudio2->CreateMasteringVoice(&masteVoice);

		//ウィンドウのｘボタンが押されるまでループ
		while (true)
		{
			//Windowsのメッセージ処理
			if (windowsApp->ProcessMessage())
			{
				//ゲームループを抜ける
				break;
			}
			else
			{
				//ゲームの処理
				/*uint32_t* p = nullptr;
				*p = 100;*/

				keyInput->Update();

				if (keyInput->TriggerKey(DIK_RETURN))
				{
					debugCamera.SetActive(!debugCamera.IsActive());
				}

				debugCamera.Update(*keyInput);

#ifdef USE_IMGUI
				ImGui_ImplDX12_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();

				ImGui::Begin("Object Control");

				// --- パーティクルコントロール ---
				ImGui::Checkbox("Billboard ON/OFF", &useBillboard);
				ImGui::Checkbox("Move ON/OFF", &isParticleActive);

				ImGui::SliderAngle("Rotate X", &particleRotate.x);
				ImGui::SliderAngle("Rotate Y", &particleRotate.y);
				ImGui::SliderAngle("Rotate Z", &particleRotate.z);
				ImGui::Separator();
				// --- OBJモデルコントロール ---
				ImGui::SliderFloat3("OBJ Position", &transform.translate.x, -5.0f, 5.0f);
				ImGui::SliderAngle("OBJ Rotate X", &transform.rotate.x);
				ImGui::SliderAngle("OBJ Rotate Y", &transform.rotate.y);
				ImGui::SliderAngle("OBJ Rotate Z", &transform.rotate.z);
				ImGui::ColorEdit4("OBJ Color", &materialDataObj->color.x);

				static float lightDirObj[3] = { directionalLightDataObj->direction.x, directionalLightDataObj->direction.y, directionalLightDataObj->direction.z };
				if (ImGui::SliderFloat3("OBJ Light Direction", lightDirObj, -1.0f, 1.0f)) {
					directionalLightDataObj->direction = Normalize(Vector3{ lightDirObj[0], lightDirObj[1], lightDirObj[2] });
				}
				ImGui::SliderFloat("OBJ Light Intensity", &directionalLightDataObj->intensity, 0.0f, 5.0f);
				ImGui::Separator();
				// --- 球体コントロール ---
				ImGui::SliderFloat3("Sphere Position", &transformSphere.translate.x, -5.0f, 5.0f);
				ImGui::SliderAngle("Sphere Rotate X", &transformSphere.rotate.x);
				ImGui::SliderAngle("Sphere Rotate Y", &transformSphere.rotate.y);
				ImGui::SliderAngle("Sphere Rotate Z", &transformSphere.rotate.z);
				ImGui::ColorEdit4("Sphere Color", &materialDataSphere->color.x);
				ImGui::Checkbox("useMonsterBall", &useMonsterBall);

				static float lightDirSphere[3] = { directionalLightDataSphere->direction.x, directionalLightDataSphere->direction.y, directionalLightDataSphere->direction.z };
				if (ImGui::SliderFloat3("Sphere Light Direction", lightDirSphere, -1.0f, 1.0f)) {
					directionalLightDataSphere->direction = Normalize(Vector3{ lightDirSphere[0], lightDirSphere[1], lightDirSphere[2] });
				}
				ImGui::SliderFloat("Sphere Light Intensity", &directionalLightDataSphere->intensity, 0.0f, 5.0f);
				ImGui::Separator();
				// --- スプライトコントロール ---
				ImGui::SliderFloat2("Sprite Position", &transformSprite.translate.x, 0.0f, 1280.0f);
				ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
				ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
				ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);

				ImGui::Separator();

				ImGui::End();

				//開発用UIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
				/*ImGui::ShowDemoWindow();*/

				//ImGuiの内部のコマンドを生成する
				ImGui::Render();
#endif // USE_IMGUI

				//描画用のDescriptorHeapの設定
				ID3D12DescriptorHeap* descriptorHeaps[] = {dxBasics->GetSrvDescriptorHeap()};
				dxBasics->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);

#pragma region 三角形の処理

				Matrix4x4 viewMatrix;
				Matrix4x4 projectionMatrix;
				Matrix4x4 cameraWorld;

				if (debugCamera.IsActive())
				{
					//
					viewMatrix = debugCamera.GetMatView();
					projectionMatrix = debugCamera.GetMatProjection();
					cameraWorld = Inverse(viewMatrix);
				}
				else
				{
					cameraWorld = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
					viewMatrix = Inverse(cameraWorld);
					projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(WindowsApp::kClientWidth) / float(WindowsApp::kClientHeight), 0.1f, 100.0f);

				}
				Matrix4x4 bacckToFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);
				//全軸ビルボード
				Matrix4x4 billboardMatrixFull = Multiply(bacckToFrontMatrix, cameraWorld);
				billboardMatrixFull.m[3][0] = 0.0f;//平行移動成分はいらない
				billboardMatrixFull.m[3][1] = 0.0f;
				billboardMatrixFull.m[3][2] = 0.0f;
				billboardMatrixFull.m[3][3] = 1.0f;

				// Y軸ビルボード
				float angleY = std::atan2(cameraWorld.m[2][0], cameraWorld.m[2][2]);
				Matrix4x4 billboardMatrixY = MakeRotateYMatrix(angleY);

				// X軸ビルボード
				float angleX = std::atan2(-cameraWorld.m[2][1], cameraWorld.m[2][2]);
				Matrix4x4 billboardMatrixX = MakeRotateXMatrix(angleX);

				// Z軸ビルボード（
				float angleZ = std::atan2(cameraWorld.m[0][1], cameraWorld.m[0][0]);
				Matrix4x4 billboardMatrixZ = MakeRotateZMatrix(angleZ);

				/*transform.rotate.y += 0.03f;*/
				Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

				Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
				wvpData->WVP = worldViewProjectionMatrix;
				wvpData->World = worldMatrix;

				//Sprite用のworldViewProjectionMatrixを作る
				Matrix4x4 worldMattrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
				Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
				Matrix4x4 projectionMatrixSprite = MakeOrthograhicMatrix(0.0f, 0.0f, float(WindowsApp::kClientWidth), float(WindowsApp::kClientHeight), 0.0f, 100.0f);
				Matrix4x4 worldViewProjectionMatrixSprite = Multiply(worldMattrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));
				transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;
				transformationMatrixDataSprite->World = MakeIdentity4x4();

				//パーティクル行列更新
				Matrix4x4 viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);

				uint32_t numInstance = 0;//描画すべきインスタンス数	
				for (uint32_t index = 0; index < kNumMaxInstance; ++index)
				{
					if (particles[index].lifeTime <= particles[index].currentTime)
					{//生存時間を過ぎていたら更新せず描画対象にしない
						continue;
					}

					if (isParticleActive)
					{
						particles[index].transform.translate.y += particles[index].velocity.y * kDeltaTime;
						particles[index].currentTime += kDeltaTime;//経過時間を足す
					}

					float alpha = 1.0f - (particles[index].currentTime / particles[index].lifeTime);

					Matrix4x4 matScale = MakeScaleMatrix(particles[index].transform.scale);

					// スライダーで指定したXYZ回転角 + 個別パーティクルの初期回転
					Vector3 totalRotate = {
						particleRotate.x + particles[index].transform.rotate.x,
						particleRotate.y + particles[index].transform.rotate.y,
						particleRotate.z + particles[index].transform.rotate.z
					};
					Matrix4x4 matRotateSelf = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, totalRotate, { 0.0f, 0.0f, 0.0f });

					Matrix4x4 matTranslate = MakeTranslateMatrix(particles[index].transform.translate);

					// ワールド行列の合成
					Matrix4x4 worldMatrixParticle;
					if (useBillboard)
					{
						// ビルボード ON： 正面に向かせた状態でXYZ回転を適用 (Scale * Rotate * Billboard * Translate)
						worldMatrixParticle = Multiply(matScale, Multiply(matRotateSelf, Multiply(billboardMatrixFull, matTranslate)));
					}
					else
					{
						// ビルボード OFF： 通常のアフィン変換
						worldMatrixParticle = MakeAffineMatrix(
							particles[index].transform.scale,
							totalRotate,
							particles[index].transform.translate
						);
					}

					Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrixParticle, viewProjectionMatrix);
					instancingData[index].WVP = worldViewProjectionMatrix;
					instancingData[index].World = worldMatrixParticle;
					instancingData[index].color = particles[index].color;
					instancingData[index].color.w = alpha;
					++numInstance;//生きているParticleの数を1つカウントする
				}

				//描画前処理
				dxBasics->PreDraw();

				//RootSignatureを設定。PSOに設定しているけど別途設定が必要
				dxBasics->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
				dxBasics->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());//PSOを設定
				dxBasics->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);//VBVを設定
				//形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い。
				dxBasics->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				//マテリアルCBufferの場所を設定
				dxBasics->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResourceObj->GetGPUVirtualAddress());
				//wvp用のCBufferの場所を設定
				dxBasics->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
				//SRVのDescriptorTableの先頭を設定。2はrootRarmeter[2]である
				//描画のテクスチャを読む所
				dxBasics->GetCommandList()->SetGraphicsRootDescriptorTable(2, objSrvHandleGPU);
				//ライト
				dxBasics->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResourceObj->GetGPUVirtualAddress());
				//描画(DrawCall/ドローコール)。３頂点で一つのインスタンス。インスタンスについては今後

				dxBasics->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

				//------パーティクル------

				dxBasics->GetCommandList()->SetGraphicsRootSignature(particleRootSignature.Get()); // パーティクル用 RootSignature
				dxBasics->GetCommandList()->SetPipelineState(particlePipelineState.Get());         // パーティクル用 PSO

				dxBasics->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
				dxBasics->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				//パーティクルマテリアルのCBV
				dxBasics->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResourceObj->GetGPUVirtualAddress());

				//intancing用のDataを読むためにStructreBufferのSRVを設定する
				dxBasics->GetCommandList()->SetGraphicsRootDescriptorTable(1, intancingSrvHandleGPU);

				//パーティクルテクスチャのSRV、DescriptorTable
				dxBasics->GetCommandList()->SetGraphicsRootDescriptorTable(2, particleTextureSrvHandleGPU);

				//描画6頂点の板ポリゴンを、kNumInstance(今回は10)だけInstance描画を行う
				dxBasics->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), kNumMaxInstance, 0, 0);

				//-----------
				//球
				//-----------
				/*transformSphere.rotate.y += 0.02f;*/

				dxBasics->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
				dxBasics->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());

				dxBasics->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewSphereMesh);

				Matrix4x4 worldMatrixSphere = MakeAffineMatrix(transformSphere.scale, transformSphere.rotate, transformSphere.translate);
				// カメラやプロジェクション行列は共通のもの(viewMatrix, projectionMatrix)を使い回す
				Matrix4x4 worldViewProjectionMatrixSphere = Multiply(worldMatrixSphere, Multiply(viewMatrix, projectionMatrix));
				wvpDataSphere->WVP = worldViewProjectionMatrixSphere;
				wvpDataSphere->World = worldMatrixSphere;

				dxBasics->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewSphereMesh);

				dxBasics->GetCommandList()->IASetIndexBuffer(&indexBufferViewSphere);
				//追加？
				dxBasics->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResourceSphere->GetGPUVirtualAddress());
				dxBasics->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResourceSphere->GetGPUVirtualAddress());

				dxBasics->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResourceSphere->GetGPUVirtualAddress());
				dxBasics->GetCommandList()->SetGraphicsRootDescriptorTable(2, useMonsterBall ? sphereSrvHandleGPU : spriteSrvHandleGPU);
				/*commandList->DrawInstanced(kVertexCountSphere, 1, 0, 0);*/
				dxBasics->GetCommandList()->DrawIndexedInstanced(kIndexCountSphere, 1, 0, 0, 0);

				//-----------
				//四角形
				//-----------

				dxBasics->GetCommandList()->SetGraphicsRootDescriptorTable(2, spriteSrvHandleGPU);
				//Spriteの描画。
				dxBasics->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);//VBVを設定

				dxBasics->GetCommandList()->IASetIndexBuffer(&indexBufferViewSprite);//IBVを設定

				//なんか入れろと
				dxBasics->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());

				//TransformationMatrixCBufferの場所を設定
				dxBasics->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
				//ライト
				dxBasics->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResourceObj->GetGPUVirtualAddress());

				//描画(DrawCall/ドローコール)６個のインデックスを使用し１つのインスタンスを描画。その他は当面０で良い
				dxBasics->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);

				Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
				uvTransformMatrix = Multiply(uvTransformMatrix, MakeRotateZMatrix(uvTransformSprite.rotate.z));
				uvTransformMatrix = Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
				materialDataSprite->uvTransform = uvTransformMatrix;

#ifdef USE_IMGUI
				//実際のcommandListのImGuiの描画コマンドを積む
				ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxBasics->GetCommandList());
#endif // USE_IMGUI

				//描画後処理
				dxBasics->PostDraw();

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

		if (masteVoice)
		{
			masteVoice->DestroyVoice();
			masteVoice = nullptr;
		}

		//音声データ解放
		SoundUnload(&soundData1);

		//xAudio2解放
		xAudio2.Reset();

		delete keyInput;

		windowsApp->Finalize();

		delete windowsApp;
		windowsApp = nullptr;

		delete dxBasics;
	}
	return 0;
}