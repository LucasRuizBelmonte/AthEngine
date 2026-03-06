/**
 * @file AudioEngine.h
 * @brief Declarations for AudioEngine.
 */

#pragma once

#pragma region Includes
#include <string>
#include <unordered_map>
#pragma endregion

#pragma region Declarations
namespace FMOD
{
	class System;
	class Sound;
	class Channel;
}

class AudioEngine
{
public:
#pragma region Public Interface
	/**
	 * @brief Constructs a new AudioEngine instance.
	 */
	AudioEngine() = default;
	/**
	 * @brief Destroys this AudioEngine instance.
	 */
	~AudioEngine();

	/**
	 * @brief Constructs a new AudioEngine instance.
	 */
	AudioEngine(const AudioEngine &) = delete;
	/**
	 * @brief Overloads operator=.
	 */
	AudioEngine &operator=(const AudioEngine &) = delete;

	/**
	 * @brief Executes Init.
	 */
	bool Init(int maxChannels = 512);
	/**
	 * @brief Executes Update.
	 */
	void Update();
	/**
	 * @brief Executes Shutdown.
	 */
	void Shutdown();

	/**
	 * @brief Executes Load Sound.
	 */
	bool LoadSound(const std::string &id, const std::string &path, bool loop = false, bool is3D = false);
	/**
	 * @brief Executes Play.
	 */
	FMOD::Channel *Play(const std::string &id, float volume = 1.0f);

#pragma endregion
private:
#pragma region Private Implementation
	FMOD::System *m_system = nullptr;
	std::unordered_map<std::string, FMOD::Sound *> m_sounds;
#pragma endregion
};
#pragma endregion
