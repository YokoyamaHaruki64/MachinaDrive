#pragma once

#include <xaudio2.h>
#include <string>
#include "System/Core/component.h"
#include "System/Core/ComponentRegistry.h"


class Audio : public Component
{
    REGISTER_COMPONENT(Audio, "Audio");

private:
	static IXAudio2*				m_Xaudio;
	static IXAudio2MasteringVoice*	m_MasteringVoice;
	static bool						m_mediaFoundationInitialized;

	IXAudio2SourceVoice*	m_SourceVoice{};
	BYTE*					m_SoundData{};

	int						m_Length{};
	int						m_PlayLength{};
    std::string             m_filePath;
    bool                    m_playOnStart = false;
    bool                    m_loop = false;

    void Load(const char* FileName);
    bool LoadMp3(const char* FileName);
    void Uninit();

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
    void OnLoaded() override;
    void OnEditorUpdate() override;
    void OnStart() override;

protected:
    void RegisterFields(FieldRegistry& fields) override;
};

