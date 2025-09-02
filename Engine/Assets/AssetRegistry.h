#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <cstdint>

#include "../../Core/Debug.h"
#include "../../Core/IO/FileIO.h"
#include "AssetId.h"

namespace TombForge
{
    struct Model;
    struct Animation;
    struct Texture;
    struct Material;
    struct Skeleton;
    struct Level;

    enum AssetType : uint8_t
    {
        ASSET_TYPE_NONE = 0,
        ASSET_TYPE_MODEL = 1,
        ASSET_TYPE_TEXTURE = 2,
        ASSET_TYPE_MATERIAL = 3,
        ASSET_TYPE_ANIMATION = 4,
        ASSET_TYPE_SKELETON = 5,
        ASSET_TYPE_LEVEL = 6
    };

    struct AssetMeta
    {
        AssetId id{}; // Unique asset identifier
        std::string assetPath{}; // Engine format path
        std::string sourcePath{}; // Original source import path
        AssetType type{}; // Asset type e.g. model, texture, material, etc...
        bool isBuiltin{}; // Whether or not this asset is built into the engine (cannot be deleted)
    };

    class AssetRegistry
    {
    public:
        AssetRegistry() = default;
        AssetRegistry(const AssetRegistry&) = delete;
        AssetRegistry(AssetRegistry&&) = delete;
        ~AssetRegistry() = default;

        AssetRegistry& operator=(const AssetRegistry&) = delete;
        AssetRegistry& operator=(AssetRegistry&&) = delete;

        void Init(const std::string& projectPath);
        void Save() const;

        template<typename T>
        std::shared_ptr<T> Load(const AssetId id);

        template<typename T>
        std::shared_ptr<T> Load(const std::string& path); // This is slower, prefer ID

        template<typename T>
        void SaveAsset(const std::shared_ptr<T> asset) const;

        template<typename T>
        AssetId AddAsset(std::shared_ptr<T> asset, const std::string& path, const std::string& sourcePath);

        template<typename T>
        void AddAssetBuiltin(std::shared_ptr<T> asset);

        AssetId GetAssetId(const std::string& path) const;

    private:
        template<typename T>
        struct AssetTraits;

        template<>
        struct AssetTraits<Model>
        {
            static constexpr AssetType Type = ASSET_TYPE_MODEL;
            static auto& GetMap(AssetRegistry& reg) { return reg.m_loadedModels; }
        };

        template<>
        struct AssetTraits<Material>
        {
            static constexpr AssetType Type = ASSET_TYPE_MATERIAL;
            static auto& GetMap(AssetRegistry& reg) { return reg.m_loadedMaterials; }
        };

        template<>
        struct AssetTraits<Texture>
        {
            static constexpr AssetType Type = ASSET_TYPE_TEXTURE;
            static auto& GetMap(AssetRegistry& reg) { return reg.m_loadedTextures; }
        };

        template<>
        struct AssetTraits<Animation>
        {
            static constexpr AssetType Type = ASSET_TYPE_ANIMATION;
            static auto& GetMap(AssetRegistry& reg) { return reg.m_loadedAnimations; }
        };

        template<>
        struct AssetTraits<Skeleton>
        {
            static constexpr AssetType Type = ASSET_TYPE_SKELETON;
            static auto& GetMap(AssetRegistry& reg) { return reg.m_loadedSkeletons; }
        };

        template<>
        struct AssetTraits<Level>
        {
            static constexpr AssetType Type = ASSET_TYPE_LEVEL;
            static auto& GetMap(AssetRegistry& reg) { return reg.m_loadedLevels; }
        };

        template<typename T>
        std::shared_ptr<T> LoadAsset(const AssetMeta& meta);
        template<typename T>
        void WriteAsset(const T& asset, const AssetMeta& meta) const;

        template<>
        std::shared_ptr<Model> LoadAsset(const AssetMeta& meta);
        template<>
        void WriteAsset(const Model& asset, const AssetMeta& meta) const;

        template<>
        std::shared_ptr<Material> LoadAsset(const AssetMeta& meta);
        template<>
        void WriteAsset(const Material& asset, const AssetMeta& meta) const;

        template<>
        std::shared_ptr<Texture> LoadAsset(const AssetMeta& meta);
        template<>
        void WriteAsset(const Texture& asset, const AssetMeta& meta) const;

        template<>
        std::shared_ptr<Skeleton> LoadAsset(const AssetMeta& meta);
        template<>
        void WriteAsset(const Skeleton& asset, const AssetMeta& meta) const;

        template<>
        std::shared_ptr<Animation> LoadAsset(const AssetMeta& meta);
        template<>
        void WriteAsset(const Animation& asset, const AssetMeta& meta) const;

        template<>
        std::shared_ptr<Level> LoadAsset(const AssetMeta& meta);
        template<>
        void WriteAsset(const Level& asset, const AssetMeta& meta) const;

        static constexpr AssetId ReservedAssetIdStart = 1000; // IDs below this are reserved for built-in assets

        std::string GetAbsolutePath(const std::string& path) const;

        std::unordered_map<AssetId, AssetMeta> m_assets{};
        std::unordered_map<AssetId, std::shared_ptr<Model>> m_loadedModels{};
        std::unordered_map<AssetId, std::shared_ptr<Texture>> m_loadedTextures{};
        std::unordered_map<AssetId, std::shared_ptr<Material>> m_loadedMaterials{};
        std::unordered_map<AssetId, std::shared_ptr<Animation>> m_loadedAnimations{};
        std::unordered_map<AssetId, std::shared_ptr<Skeleton>> m_loadedSkeletons{};
        std::unordered_map<AssetId, std::shared_ptr<Level>> m_loadedLevels{};

        std::string m_registryPath{};
        std::string m_basePath{};

        friend class Engine;
    };

    template<typename T>
    inline std::shared_ptr<T> AssetRegistry::Load(const AssetId id)
    {
        auto metaIt = m_assets.find(id);
        if (metaIt == m_assets.end() || metaIt->second.type != AssetTraits<T>::Type)
        {
            return nullptr;
        }

        auto& map = AssetTraits<T>::GetMap(*this);
        auto it = map.find(id);
        if (it != map.end())
        {
            return it->second;
        }

        std::shared_ptr<T> asset = LoadAsset<T>(metaIt->second);
        if (asset)
        {
            asset->id = id;
            asset->name = metaIt->second.assetPath;
            map[id] = asset;
        }

        return asset;
    }

    template<typename T>
    inline std::shared_ptr<T> AssetRegistry::Load(const std::string& path)
    {
        for (const auto& [id, meta] : m_assets)
        {
            if (meta.assetPath == path && meta.type == AssetTraits<T>::Type)
            {
                return Load<T>(id);
            }
        }

        return nullptr;
    }

    template<typename T>
    inline void AssetRegistry::SaveAsset(const std::shared_ptr<T> asset) const
    {
        if (!asset)
        {
            LOG_ERROR("Tried to save null asset");
            return;
        }

        const auto metaIt = m_assets.find(asset->id);
        if (metaIt == m_assets.end() || metaIt->second.type != AssetTraits<T>::Type)
        {
            LOG_ERROR("Tried to save asset that is not registered in the asset registry");
            return;
        }

        WriteAsset<T>(*asset, metaIt->second);
    }

    template<typename T>
    inline AssetId AssetRegistry::AddAsset(std::shared_ptr<T> asset, const std::string& path, const std::string& sourcePath)
    {
        if (IsValidAssetId(asset->id))
        {
            LOG_ERROR("Tried to add asset that already has a valid id");
            return InvalidAssetId;
        }

        std::string actualPath = path;
        if (FileIO::IsAbsolutePath(path))
        {
            actualPath = FileIO::GetRelativePath(path, m_basePath);
        }

        AssetMeta meta{};
        meta.id = ReservedAssetIdStart + m_assets.size();
        meta.assetPath = actualPath;
        meta.sourcePath = sourcePath;
        meta.type = AssetTraits<T>::Type;

        asset->id = meta.id;
        asset->name = meta.assetPath;

        m_assets.emplace(std::make_pair(meta.id, meta));

        WriteAsset<T>(*asset, meta);

        AssetTraits<T>::GetMap(*this)[meta.id] = asset;
    }

    template<typename T>
    inline void AssetRegistry::AddAssetBuiltin(std::shared_ptr<T> asset)
    {
        if (!IsValidAssetId(asset->id))
        {
            LOG_ERROR("Tried to add builtin asset with invalid ID");
            return;
        }

        ASSERT(asset->id < ReservedAssetIdStart, "Tried to add builtin asset with non-builtin ID");

        AssetMeta meta{};
        meta.id = asset->id;
        meta.assetPath = asset->name;
        meta.sourcePath = "";
        meta.type = AssetTraits<T>::Type;
        meta.isBuiltin = true;

        m_assets[meta.id] = meta;
        AssetTraits<T>::GetMap(*this)[meta.id] = asset;
    }

    template<typename T>
    inline std::shared_ptr<T> LoadAsset(const AssetMeta& meta)
    {
        LOG_ERROR("LoadAsset not implemented for this type");
        return nullptr;
    }

    template<typename T>
    inline void WriteAsset(const T& asset, const AssetMeta& meta)
    {
        LOG_ERROR("WriteAsset not implemented for this type");
    }
}

