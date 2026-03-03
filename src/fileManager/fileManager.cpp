#pragma region Includes
#include "fileManager.h"

#include <fstream>
#include <sstream>
#include <iostream>
#pragma endregion

#pragma region Function Definitions
FileManager::FileManager()
{
}

FileManager::~FileManager()
{
}

std::string FileManager::read(const std::string &filePath)
{
	std::ifstream file;
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	std::stringstream fileStream;
	try
	{
		file.open(filePath);
		fileStream << file.rdbuf();
		file.close();
	}
	catch (std::ifstream::failure &e)
	{
		std::cerr << "Error reading file: " << filePath << ": " << e.what() << std::endl;
	}

    return fileStream.str();
}
#pragma endregion
