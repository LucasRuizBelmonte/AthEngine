#pragma region Includes
#include "AudioEngine.h"

#include <fmod.hpp>
#include <fmod_errors.h>
#pragma endregion

#pragma region Function Definitions
AudioEngine::~AudioEngine()
{
	Shutdown();
}

bool AudioEngine::Init(int maxChannels)
{
	if (FMOD::System_Create(&m_system) != FMOD_OK)
		return false;
	if (m_system->init(maxChannels, FMOD_INIT_NORMAL, nullptr) != FMOD_OK)
		return false;
	return true;
}

void AudioEngine::Update()
{
	if (m_system)
		m_system->update();
}

void AudioEngine::Shutdown()
{
	for (auto &kv : m_sounds)
	{
		if (kv.second)
			kv.second->release();
	}
	m_sounds.clear();

	if (m_system)
	{
		m_system->close();
		m_system->release();
		m_system = nullptr;
	}
}

bool AudioEngine::LoadSound(const std::string &id, const std::string &path, bool loop, bool is3D)
{
	if (!m_system)
		return false;

	FMOD_MODE mode = FMOD_DEFAULT;
	mode |= loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
	mode |= is3D ? FMOD_3D : FMOD_2D;

	FMOD::Sound *sound = nullptr;
	if (m_system->createSound(path.c_str(), mode, nullptr, &sound) != FMOD_OK)
		return false;

	m_sounds[id] = sound;
	return true;
}

FMOD::Channel *AudioEngine::Play(const std::string &id, float volume)
{
	if (!m_system)
		return nullptr;

	auto it = m_sounds.find(id);
	if (it == m_sounds.end())
		return nullptr;

	FMOD::Channel *channel = nullptr;
	if (m_system->playSound(it->second, nullptr, false, &channel) != FMOD_OK)
		return nullptr;

	if (channel)
		channel->setVolume(volume);
	return channel;
}
#pragma endregion
