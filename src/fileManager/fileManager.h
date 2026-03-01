#pragma once

#include <string>

class FileManager
{
public:
	FileManager();
	~FileManager();

	static std::string read(const std::string& filePath);
};