#pragma once

#include <string>
#include <vector>

namespace TombForge::FileIO
{
	static constexpr uint8_t TFFileMagic{ 'T' };

	struct TFFileHeader
	{
		uint8_t magic{ TFFileMagic }; // Indicates this is indeed a TombForge file
		uint8_t type{}; // Type e.g. material, model, texture, etc...
		uint16_t engine{}; // Engine version
		uint16_t version{}; // Version of the file type specifically
	};

	std::string ReadEntireFile(const std::string& filePath);

	bool FileExists(const std::string& filePath);

	bool IsAbsolutePath(const std::string& filePath);

	bool SearchForFile(const std::string& fileName, const std::string& directory, std::string* outOptional, std::vector<std::string>* exclude = nullptr);

	int RelativeParentLevels(const std::string& filePath);

	std::string GetBasePath(const std::string& filePath);

	std::string GetFileName(const std::string& filePath, bool includeExtension = false);
}

