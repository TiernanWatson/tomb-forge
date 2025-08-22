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

    /// <summary>
    /// Returns a file name from a path. If a directory is passed, then it will just return the directory name.
    /// </summary>
    /// <param name="filePath">File path to get name from</param>
    /// <param name="includeExtension">Whether or not to include the file extension</param>
    /// <returns>File name</returns>
    std::string GetFileName(const std::string& filePath, bool includeExtension = false);

    std::string GetRelativePath(const std::string& filePath, const std::string& basePath);

    /// <summary>
    /// Gets a directory from a file path, e.g. "C:/path/to/file.txt" -> "C:/path/to". Returns same string if already a directory.
    /// The path will stay relative or absolute depending on what is passed.
    /// </summary>
    /// <param name="filePath">File path with directory</param>
    /// <returns>Directory</returns>
    std::string GetDirectory(const std::string& filePath);

    void CreateDirectory(const std::string& directory);

    bool IsDirectory(const std::string& path);
}

