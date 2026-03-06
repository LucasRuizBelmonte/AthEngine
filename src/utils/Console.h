/**
 * @file Console.h
 * @brief Declarations for in-editor console logging.
 */

#pragma once

#pragma region Includes
#include <cstdint>
#include <string>
#include <vector>
#pragma endregion

#pragma region Declarations
enum class ConsoleLevel
{
	Debug = 0,
	Info = 1,
	Warning = 2,
	Error = 3
};

inline constexpr ConsoleLevel Debug = ConsoleLevel::Debug;
inline constexpr ConsoleLevel Info = ConsoleLevel::Info;
inline constexpr ConsoleLevel Warning = ConsoleLevel::Warning;
inline constexpr ConsoleLevel Error = ConsoleLevel::Error;

struct ConsoleEntry
{
	ConsoleLevel level = ConsoleLevel::Debug;
	std::string message;
	uint64_t sequence = 0;
};

class Console
{
public:
	#pragma region Public Interface
	static void Print(const std::string &message);
	static void Print(const std::string &message, ConsoleLevel level);
	static void Print(const char *message);
	static void Print(const char *message, ConsoleLevel level);

	static void SetLevel(ConsoleLevel level);
	static ConsoleLevel GetLevel();

	static std::vector<ConsoleEntry> GetEntries();
	static void Clear();
	#pragma endregion
};
#pragma endregion
