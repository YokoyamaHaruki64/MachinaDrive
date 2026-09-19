
#include "Component/audio.h"
#include "System/Core/StringEncoding.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <algorithm>
#include <cctype>
#include <limits>
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
std::unordered_map<std::string, std::unique_ptr<Audio::AudioData>> Audio::m_audioDataCache;


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
	m_audioDataCache.clear();
	if (m_mediaFoundationInitialized)
	{
		MFShutdown();
		m_mediaFoundationInitialized = false;
	}
	CoUninitialize();
}









void Audio::Load(const char* FileName)
{
    if (!FileName || !m_Xaudio)
    {
        return;
    }

    const std::string filePath(FileName);
    const auto cachedData = m_audioDataCache.find(filePath);
    if (cachedData != m_audioDataCache.end())
    {
        m_audioData = cachedData->second.get();
    }
    else
    {
        auto audioData = std::make_unique<AudioData>();
        const bool loaded = IsMp3File(FileName)
            ? LoadMp3(FileName, *audioData)
            : LoadWav(FileName, *audioData);
        if (!loaded)
        {
            return;
        }

        const auto insertedData = m_audioDataCache.emplace(filePath, std::move(audioData));
        m_audioData = insertedData.first->second.get();
    }

    if (FAILED(m_Xaudio->CreateSourceVoice(&m_SourceVoice, &m_audioData->waveFormat)))
    {
        m_audioData = nullptr;
        return;
    }
    ApplyVolume();
}


bool Audio::LoadWav(const char* FileName, AudioData& audioData)
{
    std::wstring wideFileName = StringEncoding::Utf8ToWide(FileName ? FileName : "");
    if (wideFileName.empty())
    {
        return false;
    }

    WAVEFORMATEX waveFormat{};
    MMIOINFO mmioInfo{};
    MMCKINFO riffChunkInfo{};
    MMCKINFO dataChunkInfo{};
    MMCKINFO formatChunkInfo{};
    HMMIO hmmio = mmioOpenW(wideFileName.data(), &mmioInfo, MMIO_READ);
    if (!hmmio)
    {
        return false;
    }

    riffChunkInfo.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    formatChunkInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
    dataChunkInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioDescend(hmmio, &riffChunkInfo, nullptr, MMIO_FINDRIFF) != MMSYSERR_NOERROR
        || mmioDescend(hmmio, &formatChunkInfo, &riffChunkInfo, MMIO_FINDCHUNK) != MMSYSERR_NOERROR)
    {
        mmioClose(hmmio, 0);
        return false;
    }

    if (formatChunkInfo.cksize >= sizeof(WAVEFORMATEX))
    {
        mmioRead(hmmio, reinterpret_cast<HPSTR>(&waveFormat), sizeof(waveFormat));
    }
    else
    {
        PCMWAVEFORMAT pcmWaveFormat{};
        mmioRead(hmmio, reinterpret_cast<HPSTR>(&pcmWaveFormat), sizeof(pcmWaveFormat));
        memcpy(&waveFormat, &pcmWaveFormat, sizeof(pcmWaveFormat));
        waveFormat.cbSize = 0;
    }
    mmioAscend(hmmio, &formatChunkInfo, 0);

    if (waveFormat.nBlockAlign == 0
        || mmioDescend(hmmio, &dataChunkInfo, &riffChunkInfo, MMIO_FINDCHUNK) != MMSYSERR_NOERROR)
    {
        mmioClose(hmmio, 0);
        return false;
    }

    audioData.soundData.resize(dataChunkInfo.cksize);
    const LONG readLength = mmioRead(
        hmmio,
        reinterpret_cast<HPSTR>(audioData.soundData.data()),
        dataChunkInfo.cksize);
    mmioClose(hmmio, 0);
    if (readLength <= 0)
    {
        audioData.soundData.clear();
        return false;
    }

    audioData.soundData.resize(static_cast<size_t>(readLength));
    audioData.waveFormat = waveFormat;
    audioData.playLength = readLength / waveFormat.nBlockAlign;
    return true;
}


bool Audio::LoadMp3(const char* FileName, AudioData& audioData)
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

    audioData.waveFormat = waveFormat;
    audioData.soundData = std::move(decodedData);
    audioData.playLength = static_cast<int>(playLength);
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

    m_audioData = nullptr;
}


void Audio::ApplyVolume()
{
    if (m_SourceVoice)
    {
        m_SourceVoice->SetVolume(m_volume * m_gain);
    }
}





void Audio::Play(bool Loop)
{
    if (m_owner && !m_owner->IsStarted())
    {
        m_playOnStart = true;
        m_loop = Loop;
    }
    if (!m_SourceVoice || !m_audioData)
    {
        return;
    }

	m_SourceVoice->Stop();
	m_SourceVoice->FlushSourceBuffers();


	// バッファ設定
	XAUDIO2_BUFFER bufinfo;

	memset(&bufinfo, 0x00, sizeof(bufinfo));
	bufinfo.AudioBytes = static_cast<UINT32>(m_audioData->soundData.size());
	bufinfo.pAudioData = m_audioData->soundData.data();
	bufinfo.PlayBegin = 0;
	bufinfo.PlayLength = m_audioData->playLength;

	// ループ設定
	if (Loop)
	{
		bufinfo.LoopBegin = 0;
		bufinfo.LoopLength = m_audioData->playLength;
		bufinfo.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	m_SourceVoice->SubmitSourceBuffer(&bufinfo, NULL);
	ApplyVolume();

/*
	float outputMatrix[4] = { 0.0f , 0.0f, 1.0f , 0.0f };
	m_SourceVoice->SetOutputMatrix(m_MasteringVoice, 2, 2, outputMatrix);
	//m_SourceVoice->SetVolume(0.1f);
*/


	// 再生
	m_SourceVoice->Start();

}


void Audio::Stop()
{
    // 再生待ちのバッファも破棄し、次のPlayで先頭から確実に再生する。
    m_playOnStart = false;
    m_loop = false;

    if (!m_SourceVoice)
    {
        return;
    }

    m_SourceVoice->Stop();
    m_SourceVoice->FlushSourceBuffers();
}


void Audio::SetVolume(float volume)
{
    m_volume = std::clamp(volume, 0.0f, 1.0f);
    ApplyVolume();
}


void Audio::SetGain(float gain)
{
    m_gain = (std::max)(gain, 0.0f);
    ApplyVolume();
}


void Audio::OnLoaded()
{
    m_volume = std::clamp(m_initialVolume, 0.0f, 1.0f);
    m_gain = (std::max)(m_initialGain, 0.0f);

    if (!m_SourceVoice && !m_filePath.empty())
    {
        Load(m_filePath.c_str());
    }
    else
    {
        ApplyVolume();
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
    GM_FIELD(m_initialVolume);
    GM_FIELD(m_initialGain);
}



