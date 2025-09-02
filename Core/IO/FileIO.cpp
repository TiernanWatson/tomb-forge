#include "FileIO.h"

#include <fstream>
#include <sstream>
#include <filesystem>

#include "../Debug.h"

namespace TombForge
{
    std::string FileIO::ReadEntireFile(const std::string& filePath)
    {
        std::stringstream stringStream;

        std::ifstream fileStream(filePath);
        if (fileStream.is_open())
        {
            stringStream << fileStream.rdbuf();
            fileStream.close();
        }

        return stringStream.str();
    }

    bool FileIO::FileExists(const std::string& filePath)
    {
        return std::filesystem::exists(filePath);
    }

    bool FileIO::IsAbsolutePath(const std::string& filePath)
    {
        for (char c : filePath)
        {
            if (c == ':')
            {
                return true;
            }
        }
        return false;
    }

    bool FileIO::SearchForFile(const std::string& fileName, const std::string& directory, std::string* outOptional, std::vector<std::string>* exclude)
    {
        if (fileName.empty())
        {
            return false;
        }

        namespace fs = std::filesystem;
        if (fs::is_directory(directory))
        {
            for (const auto& entry : fs::recursive_directory_iterator(directory))
            {
                const std::string testPath = entry.path().string();

                if (entry.is_regular_file() && testPath.find(fileName) != std::string::npos)
                {
                    if (exclude)
                    {
                        bool isOk = true;
                        for (auto& e : *exclude)
                        {
                            if (testPath.find(e) != std::string::npos)
                            {
                                isOk = false;
                                break;
                            }
                        }

                        if (!isOk)
                        {
                            continue;
                        }
                    }
                    *outOptional = testPath;
                    return true;
                }
            }
        }
        return false;
    }

    int FileIO::RelativeParentLevels(const std::string& filePath)
    {
        int count{};
        for (size_t i = 0; i < filePath.size() - 2; i += 3)
        {
            if (filePath[i] == '.' && filePath[i + 1] == '.' && (filePath[i + 2] == '/' || filePath[i + 2] == '\\'))
            {
                count++;
            }
            else
            {
                break;
            }
        }
        return count;
    }

    std::string FileIO::GetBasePath(const std::string& filePath)
    {
        if (filePath.empty())
        {
            return {};
        }

        const size_t lastSlash = filePath.find_last_of("/\\");

        if (lastSlash == std::string::npos)
        {
            return filePath;
        }

        return filePath.substr(0, lastSlash);
    }

    std::string FileIO::GetFileName(const std::string& filePath, bool includeExtension)
    {
        if (filePath.empty())
        {
            return {};
        }

        const size_t lastSlash = filePath.find_last_of("/\\");
        const size_t startIndex = lastSlash != std::string::npos ? (lastSlash + 1) : 0;

        const size_t lastPeriod = filePath.find_last_of('.');

        const size_t count =
            includeExtension || lastPeriod == std::string::npos
            ? (filePath.size() - startIndex)
            : filePath.find_last_of('.') - startIndex;

        return filePath.substr(startIndex, count);
    }

    std::string FileIO::GetRelativePath(const std::string& filePath, const std::string& basePath)
    {
        ASSERT(basePath.size() < filePath.size(), "%s is not larger than %s", filePath.c_str(), basePath.c_str());
        return filePath.substr(basePath.size());
    }

    std::string FileIO::GetDirectory(const std::string& filePath)
    {
        if (filePath.empty())
        {
            return {};
        }

        if (std::filesystem::is_directory(filePath))
        {
            return filePath; // Already a directory
        }

        return GetBasePath(filePath);
    }

    void FileIO::CreateDirectory(const std::string& directory)
    {
        if (directory.empty())
        {
            LOG_ERROR("Trying to create directory with empty string");
            return;
        }

        if (!std::filesystem::exists(directory))
        {
            std::filesystem::create_directories(directory);
        }
        else
        {
            LOG_WARNING("Trying to create directory %s that already exists", directory.c_str());
        }
    }

    void FileIO::DeleteFile(const std::string& filePath)
    {
        if (filePath.empty())
        {
            LOG_ERROR("Trying to delete file with empty string");
            return;
        }

        if (std::filesystem::exists(filePath))
        {
            std::filesystem::remove(filePath);
        }
        else
        {
            LOG_WARNING("Trying to delete file %s that doesn't exist", filePath.c_str());
        }
    }

    bool FileIO::IsContainedInDirectory(const std::string& filePath, const std::string& directory)
    {
        namespace fs = std::filesystem;

        if (filePath.empty() || directory.empty())
        {
            return false;
        }

        try
        {
            fs::path fileAbs = fs::weakly_canonical(fs::absolute(filePath));
            fs::path dirAbs = fs::weakly_canonical(fs::absolute(directory));

            // Early out if directory is not a directory
            if (!fs::is_directory(dirAbs))
            {
                return false;
            }

            // Check if fileAbs is within dirAbs
            auto fileIt = fileAbs.begin();
            auto dirIt = dirAbs.begin();

            for (; dirIt != dirAbs.end(); ++dirIt, ++fileIt)
            {
                if (fileIt == fileAbs.end() || *fileIt != *dirIt)
                {
                    return false;
                }
            }
            return true;
        }
        catch (const fs::filesystem_error& e)
        {
            LOG_ERROR("Filesystem error: %s", e.what());
            return false;
        }
    }

    bool FileIO::IsDirectory(const std::string& path)
    {
        return std::filesystem::is_directory(path);
    }

    bool FileIO::IsExtension(const std::string& file, const std::string& extension)
    {
        if (file.empty())
        {
            return false;
        }

        const size_t extIndex = file.find(extension);
        if (extIndex == std::string::npos)
        {
            // Technically ends with this extension if the file has no extension
            return extension.empty();
        }

        // File does have an extension, so if test extension is empty, then not a match
        if (extension.empty())
        {
            return false;
        }

        const size_t startPos = file.size() - extension.size();
        return startPos == extIndex; // String ends with extension is its the last bit of string
    }
}
