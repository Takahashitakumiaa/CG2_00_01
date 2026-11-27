#pragma once
#include <xaudio2.h>
#include <wrl.h>
#include <cstdint>

#pragma comment(lib,"xaudio2.lib")

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

class Audio
{
public:
	void Initialize();

	void Finalize();

	//音声データ
	SoundData SoundLoadWave(const char* filename);

	//音声データ解放
	void SoundUnload(SoundData* soundData);

	//音声再生
	void SoundPlayWave(const SoundData& soundData);

private:

	Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;

	IXAudio2MasteringVoice* masteVoice_ = nullptr;
};