
#include "Component/audio.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <cctype>
#include <limits>
#include <new>
#include <vector>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

namespace
{
    std::filesystem::path CreatePathFromUtf8(const char* fileName)
    {
        const char8_t* const utf8FileName = reinterpret_cast<const char8_t*>(fileName);
        return std::filesystem::path(std::u8string(utf8FileName));
    }

    bool IsMp3File(const char* fileName)
    {
        if (!fileName)
        {
            return false;
        }

        std::string extension = CreatePathFromUtf8(fileName).extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](char character)
        {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        });
        return extension == ".mp3";
    }

    bool GetPcmWaveFormat(IMFMediaType* mediaType, WAVEFORMATEX& waveFormat)
    {
        if (!mediaType)
        {
            return false;
        }

        UINT32 channels = 0;
        UINT32 samplesPerSecond = 0;
        UINT32 bitsPerSample = 0;
        if (FAILED(mediaType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels))
            || FAILED(mediaType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &samplesPerSecond))
            || FAILED(mediaType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bitsPerSample)))
        {
            return false;
        }

        const UINT64 blockAlign = static_cast<UINT64>(channels) * bitsPerSample / 8;
        const UINT64 averageBytesPerSecond = static_cast<UINT64>(samplesPerSecond) * blockAlign;
        if (channels == 0
            || samplesPerSecond == 0
            || bitsPerSample == 0
            || bitsPerSample % 8 != 0
            || blockAlign == 0
            || blockAlign > std::numeric_limits<WORD>::max()
            || averageBytesPerSecond > std::numeric_limits<DWORD>::max())
        {
            return false;
        }

        waveFormat = {};
        waveFormat.wFormatTag = WAVE_FORMAT_PCM;
        waveFormat.nChannels = static_cast<WORD>(channels);
        waveFormat.nSamplesPerSec = samplesPerSecond;
        waveFormat.nAvgBytesPerSec = static_cast<DWORD>(averageBytesPerSecond);
        waveFormat.nBlockAlign = static_cast<WORD>(blockAlign);
        waveFormat.wBitsPerSample = static_cast<WORD>(bitsPerSample);
        return true;
    }
}

IXAudio2*				Audio::m_Xaudio = NULL;
IXAudio2MasteringVoice*	Audio::m_MasteringVoice = NULL;
bool					Audio::m_mediaFoundationInitialized = false;


void Audio::InitMaster()
{
	// COM初期化
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	m_mediaFoundationInitialized = SUCCEEDED(MFStartup(MF_VERSION));

	// XAudio生成
	XAudio2Create(&m_Xaudio, 0);

	// マスタリングボイス生成
	m_Xaudio->CreateMasteringVoice(&m_MasteringVoice);
}


void Audio::UninitMaster()
{
	if (m_MasteringVoice)
	{
		m_MasteringVoice->DestroyVoice();
		m_MasteringVoice = nullptr;
	}
	if (m_Xaudio)
	{
		m_Xaudio->Release();
		m_Xaudio = nullptr;
	}
	if (m_mediaFoundationInitialized)
	{
		MFShutdown();
		m_mediaFoundationInitialized = false;
	}
	CoUninitialize();
}









void Audio::Load(const char *FileName)
{
	if (IsMp3File(FileName))
	{
		LoadMp3(FileName);
		return;
	}

	// サウンドデータ読込
	WAVEFORMATEX wfx = { 0 };

	{
		HMMIO hmmio = NULL;
		MMIOINFO mmioinfo = { 0 };
		MMCKINFO riffchunkinfo = { 0 };
		MMCKINFO datachunkinfo = { 0 };
		MMCKINFO mmckinfo = { 0 };
		UINT32 buflen;
		LONG readlen;


		hmmio = mmioOpen((LPSTR)FileName, &mmioinfo, MMIO_READ);
		assert(hmmio);

		riffchunkinfo.fccType = mmioFOURCC('W', 'A', 'V', 'E');
		mmioDescend(hmmio, &riffchunkinfo, NULL, MMIO_FINDRIFF);

		mmckinfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
		mmioDescend(hmmio, &mmckinfo, &riffchunkinfo, MMIO_FINDCHUNK);

		if (mmckinfo.cksize >= sizeof(WAVEFORMATEX))
		{
			mmioRead(hmmio, (HPSTR)&wfx, sizeof(wfx));
		}
		else
		{
			PCMWAVEFORMAT pcmwf = { 0 };
			mmioRead(hmmio, (HPSTR)&pcmwf, sizeof(pcmwf));
			memset(&wfx, 0x00, sizeof(wfx));
			memcpy(&wfx, &pcmwf, sizeof(pcmwf));
			wfx.cbSize = 0;
		}
		mmioAscend(hmmio, &mmckinfo, 0);

		datachunkinfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
		mmioDescend(hmmio, &datachunkinfo, &riffchunkinfo, MMIO_FINDCHUNK);



		buflen = datachunkinfo.cksize;
		m_SoundData = new unsigned char[buflen];
		readlen = mmioRead(hmmio, (HPSTR)m_SoundData, buflen);


		m_Length = readlen;
		m_PlayLength = readlen / wfx.nBlockAlign;


		mmioClose(hmmio, 0);
	}


	// サウンドソース生成
	m_Xaudio->CreateSourceVoice(&m_SourceVoice, &wfx);
	assert(m_SourceVoice);
}

bool Audio::LoadMp3(const char* FileName)
{
    if (!FileName || !m_mediaFoundationInitialized || !m_Xaudio)
    {
        return false;
    }

    const std::wstring filePath = CreatePathFromUtf8(FileName).wstring();
    Microsoft::WRL::ComPtr<IMFSourceReader> sourceReader;
    if (FAILED(MFCreateSourceReaderFromURL(filePath.c_str(), nullptr, &sourceReader)))
    {
        return false;
    }

    if (FAILED(sourceReader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE))
        || FAILED(sourceReader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE)))
    {
        return false;
    }

    Microsoft::WRL::ComPtr<IMFMediaType> outputType;
    if (FAILED(MFCreateMediaType(&outputType))
        || FAILED(outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio))
        || FAILED(outputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM))
        || FAILED(sourceReader->SetCurrentMediaType(
            MF_SOURCE_READER_FIRST_AUDIO_STREAM,
            nullptr,
            outputType.Get())))
    {
        return false;
    }

    Microsoft::WRL::ComPtr<IMFMediaType> currentType;
    if (FAILED(sourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &currentType)))
    {
        return false;
    }

    WAVEFORMATEX waveFormat{};
    if (!GetPcmWaveFormat(currentType.Get(), waveFormat))
    {
        return false;
    }

    std::vector<BYTE> decodedData;
    while (true)
    {
        DWORD actualStream = 0;
        DWORD streamFlags = 0;
        LONGLONG timestamp = 0;
        Microsoft::WRL::ComPtr<IMFSample> sample;
        if (FAILED(sourceReader->ReadSample(
            MF_SOURCE_READER_FIRST_AUDIO_STREAM,
            0,
            &actualStream,
            &streamFlags,
            &timestamp,
            &sample)))
        {
            return false;
        }

        if ((streamFlags & MF_SOURCE_READERF_ENDOFSTREAM) != 0)
        {
            break;
        }
        if (!sample)
        {
            continue;
        }

        Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
        if (FAILED(sample->ConvertToContiguousBuffer(&buffer)))
        {
            return false;
        }

        BYTE* bufferData = nullptr;
        DWORD maxLength = 0;
        DWORD currentLength = 0;
        if (FAILED(buffer->Lock(&bufferData, &maxLength, &currentLength)))
        {
            return false;
        }

        if (currentLength > 0)
        {
            decodedData.insert(decodedData.end(), bufferData, bufferData + currentLength);
        }
        if (FAILED(buffer->Unlock()))
        {
            return false;
        }
    }

    const size_t playLength = waveFormat.nBlockAlign > 0
        ? decodedData.size() / waveFormat.nBlockAlign
        : 0;
    if (decodedData.empty()
        || decodedData.size() > static_cast<size_t>(std::numeric_limits<int>::max())
        || playLength > static_cast<size_t>(std::numeric_limits<int>::max()))
    {
        return false;
    }

    BYTE* soundData = new (std::nothrow) BYTE[decodedData.size()];
    if (!soundData)
    {
        return false;
    }
    memcpy(soundData, decodedData.data(), decodedData.size());

    IXAudio2SourceVoice* sourceVoice = nullptr;
    if (FAILED(m_Xaudio->CreateSourceVoice(&sourceVoice, &waveFormat)) || !sourceVoice)
    {
        delete[] soundData;
        return false;
    }

    m_SoundData = soundData;
    m_Length = static_cast<int>(decodedData.size());
    m_PlayLength = static_cast<int>(playLength);
    m_SourceVoice = sourceVoice;
    return true;
}


void Audio::Uninit()
{
    if (m_SourceVoice)
    {
        m_SourceVoice->Stop();
        m_SourceVoice->DestroyVoice();
        m_SourceVoice = nullptr;
    }

    delete[] m_SoundData;
    m_SoundData = nullptr;
}





void Audio::Play(bool Loop)
{
    if (m_owner && !m_owner->IsStarted())
    {
        m_playOnStart = true;
        m_loop = Loop;
    }
    if (!m_SourceVoice)
    {
        return;
    }

	m_SourceVoice->Stop();
	m_SourceVoice->FlushSourceBuffers();


	// バッファ設定
	XAUDIO2_BUFFER bufinfo;

	memset(&bufinfo, 0x00, sizeof(bufinfo));
	bufinfo.AudioBytes = m_Length;
	bufinfo.pAudioData = m_SoundData;
	bufinfo.PlayBegin = 0;
	bufinfo.PlayLength = m_PlayLength;

	// ループ設定
	if (Loop)
	{
		bufinfo.LoopBegin = 0;
		bufinfo.LoopLength = m_PlayLength;
		bufinfo.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	m_SourceVoice->SubmitSourceBuffer(&bufinfo, NULL);

/*
	float outputMatrix[4] = { 0.0f , 0.0f, 1.0f , 0.0f };
	m_SourceVoice->SetOutputMatrix(m_MasteringVoice, 2, 2, outputMatrix);
	//m_SourceVoice->SetVolume(0.1f);
*/


	// 再生
	m_SourceVoice->Start();

}

void Audio::OnLoaded()
{
    if (!m_SourceVoice && !m_filePath.empty())
    {
        Load(m_filePath.c_str());
    }
}

void Audio::OnEditorUpdate()
{
    Uninit();
    OnLoaded();
}

void Audio::OnStart()
{
    if (m_playOnStart)
    {
        Play(m_loop);
    }
}

void Audio::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD_PATH(m_filePath, "Assets/Local/Sounds", ".wav", ".mp3");
    GM_FIELD(m_playOnStart);
    GM_FIELD(m_loop);
}



