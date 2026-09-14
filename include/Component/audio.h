#pragma once

#include <xaudio2.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "System/Core/component.h"
#include "System/Core/ComponentRegistry.h"


class Audio : public Component
{
    REGISTER_COMPONENT(Audio, "Audio");

private:
	struct AudioData
	{
		WAVEFORMATEX			waveFormat{};
		std::vector<BYTE>	soundData;
		int					playLength{};
	};

	static IXAudio2*				m_Xaudio;
	static IXAudio2MasteringVoice*	m_MasteringVoice;
	static bool						m_mediaFoundationInitialized;
	static std::unordered_map<std::string, std::unique_ptr<AudioData>> m_audioDataCache;

	IXAudio2SourceVoice*	m_SourceVoice{};
	const AudioData*		m_audioData{};
    std::string             m_filePath;
    bool                    m_playOnStart = false;
    bool                    m_loop = false;
    float                   m_initialVolume = 1.0f;
    float                   m_volume = 1.0f;

    void Load(const char* FileName);
    bool LoadWav(const char* FileName, AudioData& audioData);
    bool LoadMp3(const char* FileName, AudioData& audioData);
    void Uninit();
    void ApplyVolume();

public:
	static void InitMaster();
	static void UninitMaster();

    Audio() = delete;
    explicit Audio(GameObject* owner)
        : Component(owner)
    {
    }
    explicit Audio(GameObject* owner, const char* FileName)
        : Component(owner),
        m_filePath(FileName ? FileName : "")
    {
        if (!m_filePath.empty())
        {
            Load(m_filePath.c_str());
        }
    }
    ~Audio() override { Uninit(); }

    void Play(bool Loop = false);
    void SetVolume(float volume);
    [[nodiscard]] float GetVolume() const { return m_volume; }
    void OnLoaded() override;
    void OnEditorUpdate() override;
    void OnStart() override;

protected:
    void RegisterFields(FieldRegistry& fields) override;
};

