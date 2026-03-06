#pragma region Includes
#include "Console.h"

#include <cstddef>
#include <mutex>
#include <utility>
#pragma endregion

#pragma region File Scope
namespace
{
	constexpr size_t kMaxConsoleEntries = 2000u;

	std::mutex g_consoleMutex;
	std::vector<ConsoleEntry> g_consoleEntries;
	ConsoleLevel g_consoleLevel = ConsoleLevel::Debug;
	uint64_t g_consoleSequence = 0u;
}
#pragma endregion

#pragma region Function Definitions
void Console::Print(const std::string &message)
{
	Print(message, ConsoleLevel::Debug);
}

void Console::Print(const std::string &message, ConsoleLevel level)
{
	std::scoped_lock<std::mutex> lock(g_consoleMutex);

	ConsoleEntry entry;
	entry.level = level;
	entry.message = message;
	entry.sequence = ++g_consoleSequence;
	g_consoleEntries.push_back(std::move(entry));

	if (g_consoleEntries.size() > kMaxConsoleEntries)
	{
		const size_t overflow = g_consoleEntries.size() - kMaxConsoleEntries;
		const auto eraseCount = static_cast<std::vector<ConsoleEntry>::difference_type>(overflow);
		g_consoleEntries.erase(g_consoleEntries.begin(), g_consoleEntries.begin() + eraseCount);
	}
}

void Console::Print(const char *message)
{
	Print(message, ConsoleLevel::Debug);
}

void Console::Print(const char *message, ConsoleLevel level)
{
	Print(message ? std::string(message) : std::string(), level);
}

void Console::SetLevel(ConsoleLevel level)
{
	std::scoped_lock<std::mutex> lock(g_consoleMutex);
	g_consoleLevel = level;
}

ConsoleLevel Console::GetLevel()
{
	std::scoped_lock<std::mutex> lock(g_consoleMutex);
	return g_consoleLevel;
}

std::vector<ConsoleEntry> Console::GetEntries()
{
	std::scoped_lock<std::mutex> lock(g_consoleMutex);
	return g_consoleEntries;
}

void Console::Clear()
{
	std::scoped_lock<std::mutex> lock(g_consoleMutex);
	g_consoleEntries.clear();
}
#pragma endregion
