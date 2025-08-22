#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "../../Core/Debug.h"
#include "../../Core/IO/FileIO.h"

namespace TombForge
{
    template<typename AssetType>
    class AssetLoader
    {
    public:
        std::shared_ptr<AssetType> Load(const std::string& name);

        void AddExisting(const std::string& name, std::shared_ptr<AssetType> asset);

        void ClearUnreferenced();

        inline void SetDirectory(const std::string& directory)
        {
            m_directory = directory;
        }

    protected:
        virtual std::shared_ptr<AssetType> Read(const std::string& name) = 0;

    private:
        std::unordered_map<std::string, std::shared_ptr<AssetType>> m_cache{};
        std::string m_directory{}; // Directory where assets are stored, used for relative paths
    };

    template<typename AssetType>
    std::shared_ptr<AssetType> AssetLoader<AssetType>::Load(const std::string& name)
    {
        if (m_cache.contains(name))
        {
            return m_cache[name];
        }

        std::string fullPath = name;
        if (!FileIO::IsAbsolutePath(name))
        {
            if (m_directory.empty())
            {
                LOG_ERROR("Asset %s is not absolute and no directory is set", name.c_str());
                return nullptr;
            }
            fullPath = m_directory + "\\" + name;
        }

        auto asset = Read(fullPath);

        if (asset)
        {
            m_cache.emplace(name, asset);
            return asset;
        }

        LOG_ERROR("Failed to load asset %s from path %s", name.c_str(), fullPath.c_str());
        return nullptr;
    }

    template<typename AssetType>
    inline void AssetLoader<AssetType>::AddExisting(const std::string& name, std::shared_ptr<AssetType> asset)
    {
        m_cache.emplace(name, asset);
    }

    template<typename AssetType>
    inline void AssetLoader<AssetType>::ClearUnreferenced()
    {
        for (auto it = m_cache.begin(); it != m_cache.end();)
        {
            // 1 means its only referenced in the cache
            if (it->second.use_count() == 1)
            {
                it = m_cache.erase(it);
            }
            else
            {
                it++;
            }
        }
    }
}

