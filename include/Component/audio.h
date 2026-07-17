#pragma once

#include <xaudio2.h>
#include "System/Component.h"


class Audio : public Component
{
private:
	static IXAudio2*				m_Xaudio;
	static IXAudio2MasteringVoice*	m_MasteringVoice;

	IXAudio2SourceVoice*	m_SourceVoice{};
	BYTE*					m_SoundData{};

	int						m_Length{};
	int						m_PlayLength{};

    void Load(const char* FileName);
    void Uninit();

public:
	static void InitMaster();
	static void UninitMaster();

    Audio() = delete;
    explicit Audio(GameObject* owner, const char* FileName)
        : Component(owner)
    {
        Load(FileName);
    }
    virtual ~Audio() { Uninit(); }

	void Play(bool Loop = false);


};

