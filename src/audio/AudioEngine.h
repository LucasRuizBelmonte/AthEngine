#pragma once
#include <string>
#include <unordered_map>

namespace FMOD
{
	class System;
	class Sound;
	class Channel;
}

class AudioEngine
{
public:
	AudioEngine() = default;
	~AudioEngine();

	AudioEngine(const AudioEngine &) = delete;
	AudioEngine &operator=(const AudioEngine &) = delete;

	bool Init(int maxChannels = 512);
	void Update();
	void Shutdown();

	bool LoadSound(const std::string &id, const std::string &path, bool loop = false, bool is3D = false);
	FMOD::Channel *Play(const std::string &id, float volume = 1.0f);

private:
	FMOD::System *m_system = nullptr;
	std::unordered_map<std::string, FMOD::Sound *> m_sounds;
};