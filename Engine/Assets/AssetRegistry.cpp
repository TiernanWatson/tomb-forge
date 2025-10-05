#include "Engine/Assets/AssetRegistry.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <sndfile.h>

#include "Core/IO/BinaryReader.h"
#include "Core/IO/BinaryWriter.h"
#include "Core/IO/FileIO.h"
#include "Engine/Animation/Animation.h"
#include "Engine/Animation/AnimationSet.h"
#include "Engine/Animation/Skeleton.h"
#include "Engine/Audio/Sound.h"
#include "Engine/Levels/CollisionMesh.h"
#include "Engine/Levels/Level.h"
#include "Engine/Player/LaraConfig.h"
#include "Engine/Rendering/Material.h"
#include "Engine/Rendering/Model.h"
#include "Engine/Rendering/Texture.h"

namespace TombForge
{
    namespace
    {
        constexpr char const* SkeletonFileExt{ ".tfskel" };
        constexpr char const* AnimFileExt{ ".tfanim" };
        constexpr char const* AnimSetFileExt{ ".tfanimset" };
        constexpr char const* ModelFileExt{ ".tfmod" };
        constexpr char const* TextureFileExt{ ".tftex" };
        constexpr char const* MaterialFileExt{ ".tfmat" };
        constexpr char const* LevelFileExt{ ".tflev" };
        constexpr char const* ProjectFileExt{ ".tfproj" };
        constexpr char const* CollisionFileExt{ ".tfcol" };

        void SaveTransition(nlohmann::json& json, const AnimSetTransition& transition)
        {
            json["fromAnimations"] = transition.fromAnimations;
            json["toAnimation"] = transition.toAnimation;
            json["targetFrame"] = transition.targetFrame;
            json["blendDuration"] = transition.blendDuration;
            json["minFramesElapsed"] = transition.minFramesElapsed;
            json["shouldBlend"] = transition.shouldBlend;
            json["loop"] = transition.loop;
            json["conditions"] = nlohmann::json::array();
            for (const auto& condition : transition.conditions)
            {
                nlohmann::json& condJson = json["conditions"].emplace_back();
                condJson["condition"] = static_cast<uint8_t>(condition.condition);
                condJson["threshold"] = condition.threshold;
            }
        }

        void LoadTransition(const nlohmann::json& json, AnimSetTransition& transition)
        {
            if (json.contains("fromAnimations"))
            {
                transition.fromAnimations = json["fromAnimations"].get<std::vector<uint32_t>>();
            }
            if (json.contains("toAnimation"))
            {
                transition.toAnimation = json["toAnimation"].get<uint32_t>();
            }
            if (json.contains("targetFrame"))
            {
                transition.targetFrame = json["targetFrame"].get<float>();
            }
            if (json.contains("blendDuration"))
            {
                transition.blendDuration = json["blendDuration"].get<float>();
            }
            if (json.contains("minFramesElapsed"))
            {
                transition.minFramesElapsed = json["minFramesElapsed"].get<float>();
            }
            if (json.contains("shouldBlend"))
            {
                transition.shouldBlend = json["shouldBlend"].get<bool>();
            }
            if (json.contains("loop"))
            {
                transition.loop = json["loop"].get<bool>();
            }
            if (json.contains("conditions"))
            {
                for (const auto& condJson : json["conditions"])
                {
                    auto& condition = transition.conditions.emplace_back();
                    if (condJson.contains("condition"))
                    {
                        condition.condition = condJson["condition"].get<AnimSetTransition::Condition::Type>();
                    }
                    if (condJson.contains("threshold"))
                    {
                        condition.threshold = condJson["threshold"].get<float>();
                    }
                }
            }
        }
    }

    void AssetRegistry::Init(const std::string& projectPath)
    {
        if (!FileIO::IsDirectory(projectPath))
        {
            LOG_ERROR("Project path is not a directory: %s", projectPath.c_str());
            return;
        }

        m_basePath = projectPath + FileIO::Separator;
        m_registryPath = m_basePath + "assets.json";

        if (!FileIO::FileExists(m_registryPath))
        {
            LOG("Asset registry does not exist, creating assets.json");
            Save();
            return;
        }

        std::ifstream inFile(m_registryPath);
        if (inFile.is_open())
        {
            nlohmann::json json = nlohmann::json::parse(inFile);
            for (const auto& item : json["assets"])
            {
                if (!item.contains("id") || !item.contains("assetPath") || !item.contains("type"))
                {
                    LOG_WARNING("Malformed asset registry entry, skipping. Saving the registry will remove this entry!");
                    continue;
                }

                AssetMeta meta{};
                meta.id = item["id"].get<AssetId>();
                if (item.contains("name"))
                {
                    meta.name = item["name"].get<std::string>();
                }
                else
                {
                    meta.name = FileIO::GetFileName(item["assetPath"].get<std::string>());
                }
                meta.assetPath = item["assetPath"].get<std::string>();
                meta.sourcePath = item.contains("sourcePath") ? item["sourcePath"].get<std::string>() : std::string{};
                meta.type = static_cast<AssetType>(item["type"].get<uint8_t>());
                m_assets.emplace(std::make_pair(meta.id, meta));

                if (meta.id >= m_nextId)
                {
                    m_nextId = meta.id + 1;
                }
            }
            inFile.close();
        }
        else
        {
            LOG_ERROR("Failed to open asset registry: %s", m_registryPath.c_str());
        }
    }

    void AssetRegistry::Save()
    {
        std::ofstream outFile(m_registryPath);
        if (outFile.is_open())
        {
            nlohmann::json json{};
            json["assets"] = nlohmann::json::array();
            for (auto& [id, meta] : m_assets)
            {
                if (!IsValidAssetId(id) || meta.isBuiltin)
                {
                    continue;
                }

                switch (meta.type)
                {
                case ASSET_TYPE_MODEL:
                {
                    if (!SaveAssetIfDirty<Model>(id, meta))
                    {
                        continue;
                    }
                    break;
                }
                case ASSET_TYPE_TEXTURE:
                {
                    if (!SaveAssetIfDirty<Texture>(id, meta))
                    {
                        continue;
                    }
                    break;
                }
                case ASSET_TYPE_MATERIAL:
                {
                    if (!SaveAssetIfDirty<Material>(id, meta))
                    {
                        continue;
                    }
                    break;
                }
                case ASSET_TYPE_ANIMATION:
                {
                    if (!SaveAssetIfDirty<Animation>(id, meta))
                    {
                        continue;
                    }
                    break;
                }
                case ASSET_TYPE_SKELETON:
                {
                    if (!SaveAssetIfDirty<Skeleton>(id, meta))
                    {
                        continue;
                    }
                    break;
                }
                case ASSET_TYPE_LEVEL:
                {
                    if (!SaveAssetIfDirty<Level>(id, meta))
                    {
                        continue;
                    }
                    break;
                }
                case ASSET_TYPE_COLLISION_MESH:
                {
                    if (!SaveAssetIfDirty<CollisionMesh>(id, meta))
                    {
                        continue;
                    }
                    break;
                }
                case ASSET_TYPE_SOUND:
                    break; // No data to save for sounds yet
                }

                nlohmann::json item{};
                item["id"] = meta.id;
                item["name"] = meta.name;
                item["sourcePath"] = meta.sourcePath;
                item["assetPath"] = meta.assetPath;
                item["type"] = static_cast<uint8_t>(meta.type);
                json["assets"].emplace_back(item);
            }

            outFile << json.dump(4);
            outFile.close();
        }
        else
        {
            LOG_ERROR("Failed to open asset registry for writing: %s", m_registryPath.c_str());
        }
    }

    bool AssetRegistry::LoadLaraConfig(LaraConfig& config)
    {
        const std::string path = m_basePath + "lara.json";
        if (!FileIO::FileExists(path))
        {
#ifndef EDITOR_ENABLED
            LOG_ERROR("Lara config file does not exist: %s", path.c_str());
#endif
            return false;
        }

        std::ifstream inFile(path);
        if (inFile.is_open())
        {
            inFile.seekg(0, std::ios::end);
            std::streampos fileSize = inFile.tellg();
            inFile.seekg(0, std::ios::beg);
            if (fileSize < 1)
            {
                return false;
            }
            nlohmann::json json = nlohmann::json::parse(inFile);
            if (json.contains("modelId"))
            {
                config.modelId = json["modelId"].get<AssetId>();
            }
            if (json.contains("animationSets"))
            {
                config.animSetsForStates.clear();
                for (const auto& entry : json["animationSets"])
                {
                    config.animSetsForStates.emplace(entry["state"].get<LaraState>(), entry["set"].get<AssetId>());
                }
            }
            if (json.contains("transitionMaps"))
            {
                config.animSetEntries.clear();
                for (const auto& entry : json["transitionMaps"])
                {
                    auto& key = config.animSetEntries.emplace_back();
                    key.fromAnimSetId = entry["from"].get<AssetId>();
                    key.toAnimSetId = entry["to"].get<AssetId>();
                    LoadTransition(entry, key.transition);
                }
            }
            inFile.close();
            return true;
        }
        else
        {
            LOG_ERROR("Failed to open Lara config file: %s", path.c_str());
            return false;
        }
    }

    bool AssetRegistry::SaveLaraConfig(const LaraConfig& config) const
    {
        const std::string path = m_basePath + "lara.json";

        std::ofstream outFile(path);
        if (outFile.is_open())
        {
            nlohmann::json json{};
            json["modelId"] = config.modelId;
            json["animationSets"] = nlohmann::json::array();
            for (const auto& [state, setId] : config.animSetsForStates)
            {
                nlohmann::json item{};
                item["state"] = static_cast<uint32_t>(state);
                item["set"] = setId;
                json["animationSets"].emplace_back(item);
            }
            json["transitionMaps"] = nlohmann::json::array();
            for (const auto& entry : config.animSetEntries)
            {
                nlohmann::json item{};
                item["from"] = entry.fromAnimSetId;
                item["to"] = entry.toAnimSetId;
                SaveTransition(item, entry.transition);
                json["transitionMaps"].emplace_back(item);
            }
            outFile << json.dump(4);
            outFile.flush();
            outFile.close();
            return true;
        }
        else
        {
            LOG_ERROR("Failed to open Lara config file for writing: %s", path.c_str());
            return false;
        }
    }

    AssetId AssetRegistry::GetAssetId(const std::string& path) const
    {
        const std::string finalPath = FileIO::IsAbsolutePath(path) ? FileIO::GetRelativePath(path, m_basePath) : path;

        for (const auto& [id, meta] : m_assets)
        {
            if (meta.assetPath == finalPath)
            {
                return id;
            }
        }

        return InvalidAssetId;
    }

    std::string AssetRegistry::GetAssetName(const AssetId id) const
    {
        auto it = m_assets.find(id);
        if (it == m_assets.end())
        {
            return "Not Registered";
        }
        return it->second.assetPath;
    }

    std::string AssetRegistry::GetExtension(AssetType type) const
    {
        switch (type)
        {
        case ASSET_TYPE_SKELETON:      return SkeletonFileExt;
        case ASSET_TYPE_ANIMATION:     return AnimFileExt;
        case ASSET_TYPE_ANIMATION_SET: return AnimSetFileExt;
        case ASSET_TYPE_MODEL:         return ModelFileExt;
        case ASSET_TYPE_TEXTURE:       return TextureFileExt;
        case ASSET_TYPE_MATERIAL:      return MaterialFileExt;
        case ASSET_TYPE_LEVEL:         return LevelFileExt;
        case ASSET_TYPE_COLLISION_MESH:return CollisionFileExt;
        case ASSET_TYPE_SOUND:         return ".wav";
        case ASSET_TYPE_CONFIG:        return ProjectFileExt;
        default:                       return ".tf";
        }
    }

    std::string AssetRegistry::GetAbsolutePath(const std::string& path) const
    {
        if (FileIO::IsAbsolutePath(path))
        {
            return path;
        }

        return m_basePath + path;
    }

    template<>
    std::shared_ptr<Sound> AssetRegistry::LoadAsset(const AssetMeta& meta)
    {
        const std::string absPath = GetAbsolutePath(meta.assetPath);

        SF_INFO sfInfo{};
        SNDFILE* sndFile = sf_open(absPath.c_str(), SFM_READ, &sfInfo);
        if (!sndFile)
        {
            LOG_ERROR("Failed to open audio file: %s", meta.assetPath.c_str());
            return nullptr;
        }

        std::shared_ptr<Sound> sound = std::make_shared<Sound>();
        sound->data.resize(sfInfo.frames * sfInfo.channels);
        sound->sampleRate = sfInfo.samplerate;
        sound->channels = sfInfo.channels == 1 ? SOUND_CHANNEL_MONO : SOUND_CHANNEL_STEREO;
        sound->numFrames = sfInfo.frames;
        sound->format = SOUND_FORMAT_WAV;
        
        sf_count_t numFramesRead = sf_readf_short(sndFile, sound->data.data(), sfInfo.frames);
        sf_close(sndFile);

        if (numFramesRead != sfInfo.frames || numFramesRead < 1)
        {
            LOG_ERROR("Failed to read all frames from audio file: %s", meta.assetPath.c_str());
            return nullptr;
        }

        return sound;
    }

    template<>
    void AssetRegistry::WriteAsset(const Sound& asset, const AssetMeta& meta) const
    {
        // Sounds have no data other than a file path for now
    }

    template<>
    std::shared_ptr<Model> AssetRegistry::LoadAsset(const AssetMeta& meta)
    {
        const std::string absPath = GetAbsolutePath(meta.assetPath);
        std::ifstream inFile(absPath, std::ios::binary);

        if (inFile.is_open())
        {
            std::shared_ptr<Model> resource = std::make_shared<Model>();

            size_t numMeshes{};
            inFile.read((char*)&numMeshes, sizeof(size_t));
            resource->meshes.resize(numMeshes);

            for (size_t i = 0; i < numMeshes; i++)
            {
                Mesh& mesh = resource->meshes[i];

                size_t nameSize{};
                inFile.read((char*)&nameSize, sizeof(size_t));
                mesh.name.resize(nameSize);
                inFile.read(mesh.name.data(), sizeof(char) * nameSize);

                size_t numVertices{};
                inFile.read((char*)&numVertices, sizeof(size_t));
                mesh.vertices.resize(numVertices);
                inFile.read((char*)mesh.vertices.data(), numVertices * sizeof(Vertex));

                size_t numIndices{};
                inFile.read((char*)&numIndices, sizeof(size_t));
                mesh.indices.resize(numIndices);
                inFile.read((char*)mesh.indices.data(), numIndices * sizeof(uint32_t));

                inFile.read((char*)&mesh.bounds, sizeof(AABB));

                bool hasMaterial{};
                inFile.read((char*)&hasMaterial, sizeof(bool));
                if (hasMaterial)
                {
                    AssetId materialName{};
                    inFile.read((char*)&materialName, sizeof(size_t));
                    mesh.material = Load<Material>(materialName);
                }
            }

            bool hasSkeleton{};
            inFile.read((char*)&hasSkeleton, sizeof(bool));
            if (hasSkeleton)
            {
                AssetId skelId{};
                inFile.read((char*)&skelId, sizeof(size_t));
                resource->skeleton = Load<Skeleton>(skelId);
            }

            return resource;
        }

        return nullptr;
    }

    template<>
    void AssetRegistry::WriteAsset(const Model& asset, const AssetMeta& meta) const
    {
        const std::string& filePath = GetAbsolutePath(meta.assetPath);

        std::ofstream outFile(filePath, std::ios::binary);

        if (outFile.is_open())
        {
            const size_t numMeshes = asset.meshes.size();
            outFile.write((const char*)&numMeshes, sizeof(size_t));

            for (size_t i = 0; i < numMeshes; i++)
            {
                const Mesh& mesh = asset.meshes[i];

                const size_t nameSize = mesh.name.size();
                outFile.write((const char*)&nameSize, sizeof(size_t));
                outFile.write(mesh.name.c_str(), sizeof(char) * nameSize);

                const size_t numVertices = mesh.vertices.size();
                outFile.write((const char*)&numVertices, sizeof(size_t));
                outFile.write((const char*)mesh.vertices.data(), numVertices * sizeof(Vertex));

                const size_t numIndices = mesh.indices.size();
                outFile.write((const char*)&numIndices, sizeof(size_t));
                outFile.write((const char*)mesh.indices.data(), numIndices * sizeof(uint32_t));

                outFile.write((const char*)&mesh.bounds, sizeof(AABB));

                const bool hasMaterial = mesh.material != nullptr;
                outFile.write((const char*)&hasMaterial, sizeof(bool));
                if (hasMaterial)
                {
                    outFile.write((const char*)&mesh.material->id, sizeof(size_t));
                }
            }

            const bool hasSkeleton = asset.skeleton != nullptr;
            outFile.write((const char*)&hasSkeleton, sizeof(bool));
            if (hasSkeleton)
            {
                outFile.write((const char*)&asset.skeleton->id, sizeof(size_t));
            }

            return;
        }

        LOG_ERROR("Failed to open asset file for writing: %s", filePath.c_str());
    }

    template<>
    std::shared_ptr<Material> AssetRegistry::LoadAsset(const AssetMeta& meta)
    {
        const std::string name = GetAbsolutePath(meta.assetPath);

        std::ifstream inFile(name);

        if (!inFile.is_open())
        {
            LOG_ERROR("Failed to open %s", name.c_str());
            return nullptr;
        }

        nlohmann::json json = nlohmann::json::parse(inFile);

        std::shared_ptr<Material> resource = std::make_shared<Material>();

        if (json.contains("diffuse"))
        {
            const AssetId diffusePath = json["diffuse"].get<AssetId>();
            resource->albedoTexture = Load<Texture>(diffusePath);
        }

        if (json.contains("normal"))
        {
            const AssetId normalPath = json["normal"].get<AssetId>();
            resource->normalTexture = Load<Texture>(normalPath);
        }

        if (json.contains("roughness"))
        {
            const AssetId roughnessPath = json["roughness"].get<AssetId>();
            resource->roughnessTexture = Load<Texture>(roughnessPath);
        }

        if (json.contains("metalness"))
        {
            const AssetId metalnessPath = json["metalness"].get<AssetId>();
            resource->metalnessTexture = Load<Texture>(metalnessPath);
        }

        if (json.contains("albedoColor"))
        {
            auto albedo = json["albedoColor"];
            resource->albedoColor = glm::vec4{ albedo[0], albedo[1], albedo[2], albedo[3] };
        }

        if (json.contains("isTransparent"))
        {
            bool isTransparent = json["isTransparent"];
            if (isTransparent)
            {
                resource->AddFlag(MATERIAL_FLAG_TRANSPARENT);
            }
        }

        return resource;
    }

    template<>
    void AssetRegistry::WriteAsset(const Material& asset, const AssetMeta& meta) const
    {
        const std::string& filePath = GetAbsolutePath(meta.assetPath);

        std::ofstream outFile(filePath);

        if (outFile.is_open())
        {
            nlohmann::json json;
            if (asset.albedoTexture)
            {
                json["diffuse"] = asset.albedoTexture->id;
            }
            if (asset.normalTexture)
            {
                json["normal"] = asset.normalTexture->id;
            }
            if (asset.roughnessTexture)
            {
                json["roughness"] = asset.roughnessTexture->id;
            }
            if (asset.metalnessTexture)
            {
                json["metalness"] = asset.metalnessTexture->id;
            }
            json["albedoColor"] = { asset.albedoColor.r, asset.albedoColor.g, asset.albedoColor.b, asset.albedoColor.a };
            json["isTransparent"] = asset.TestFlag(MATERIAL_FLAG_TRANSPARENT);

            outFile << json.dump(4);

            outFile.flush();
            outFile.close();

            return;
        }

        LOG_ERROR("Could not save material to %s", filePath);
    }

    template<>
    std::shared_ptr<Texture> AssetRegistry::LoadAsset(const AssetMeta& meta)
    {
        const std::string name = GetAbsolutePath(meta.assetPath);
        std::ifstream inFile(name, std::ios::binary);

        if (inFile.is_open())
        {
            std::shared_ptr<Texture> resource = std::make_shared<Texture>();

            inFile.read((char*)&resource->format, sizeof(TextureFormat));
            inFile.read((char*)&resource->width, sizeof(uint32_t));
            inFile.read((char*)&resource->height, sizeof(uint32_t));

            const size_t dataSize = static_cast<size_t>(resource->format)
                * resource->width
                * resource->height;
            resource->data.resize(dataSize);
            inFile.read((char*)resource->data.data(), dataSize);

            inFile.read((char*)&resource->filter, sizeof(TextureFilter));
            inFile.read((char*)&resource->type, sizeof(TextureDataType));
            inFile.read((char*)&resource->sRGB, sizeof(bool));

            inFile.close();
            return resource;
        }

        return nullptr;
    }

    template<>
    void AssetRegistry::WriteAsset(const Texture& asset, const AssetMeta& meta) const
    {
        if (!asset.IsValidData())
        {
            LOG_ERROR("Tried to save texture %s, but invalid data", asset.name.c_str());
            return;
        }

        const std::string filePath = GetAbsolutePath(meta.assetPath);
        std::ofstream outFile(filePath, std::ios::binary);
        if (outFile.is_open())
        {
            outFile.write((const char*)&asset.format, sizeof(TextureFormat));
            outFile.write((const char*)&asset.width, sizeof(uint32_t));
            outFile.write((const char*)&asset.height, sizeof(uint32_t));
            outFile.write((const char*)asset.data.data(), sizeof(ColorByte) * asset.data.size());
            outFile.write((const char*)&asset.filter, sizeof(TextureFilter));
            outFile.write((const char*)&asset.type, sizeof(TextureDataType));
            outFile.write((const char*)&asset.sRGB, sizeof(bool));

            outFile.flush();
            outFile.close();
            return;
        }

        LOG_ERROR("Could not save texture to %s", filePath.c_str());
    }

    template<>
    std::shared_ptr<Skeleton> AssetRegistry::LoadAsset(const AssetMeta& meta)
    {
        const std::string& filePath = GetAbsolutePath(meta.assetPath);

        std::ifstream inFile(filePath, std::ios::binary);

        if (inFile.is_open())
        {
            size_t numOfBones{};
            inFile.read((char*)&numOfBones, sizeof(size_t));

            std::shared_ptr<Skeleton> resource = std::make_shared<Skeleton>();
            resource->bones.resize(numOfBones);

            for (size_t b = 0; b < numOfBones; b++)
            {
                size_t stringLength{};
                inFile.read((char*)&stringLength, sizeof(size_t));

                resource->bones[b].name.resize(stringLength);
                inFile.read(resource->bones[b].name.data(), sizeof(char) * stringLength);

                inFile.read((char*)&resource->bones[b].offset, sizeof(glm::mat4));
                inFile.read((char*)&resource->bones[b].transform, sizeof(glm::mat4));
                inFile.read((char*)&resource->bones[b].parent, sizeof(uint8_t));
            }

            inFile.close();

            return resource;
        }

        LOG_ERROR("Failed to open skeleton file: %s", filePath.c_str());
        return nullptr;
    }

    template<>
    void AssetRegistry::WriteAsset(const Skeleton& asset, const AssetMeta& meta) const
    {
        const std::string& filePath = GetAbsolutePath(meta.assetPath);

        std::ofstream outFile(filePath, std::ios::binary);

        if (outFile.is_open())
        {
            const size_t numOfBones = asset.bones.size();
            outFile.write((const char*)&numOfBones, sizeof(size_t));

            for (size_t b = 0; b < asset.bones.size(); b++)
            {
                const size_t stringLength = asset.bones[b].name.length();
                outFile.write((const char*)&stringLength, sizeof(size_t));
                outFile.write(asset.bones[b].name.c_str(), sizeof(char) * stringLength);

                outFile.write((const char*)&asset.bones[b].offset, sizeof(glm::mat4));
                outFile.write((const char*)&asset.bones[b].transform, sizeof(glm::mat4));
                outFile.write((const char*)&asset.bones[b].parent, sizeof(uint8_t));
            }

            outFile.flush();
            outFile.close();

            return;
        }

        LOG_ERROR("Failed to open skeleton file for writing: %s", filePath.c_str());
    }

    template<>
    std::shared_ptr<Animation> AssetRegistry::LoadAsset(const AssetMeta& meta)
    {
        const std::string& filePath = GetAbsolutePath(meta.assetPath);

        std::ifstream inFile(filePath, std::ios::binary);

        if (inFile.is_open())
        {
            std::shared_ptr<Animation> resource = std::make_shared<Animation>();

            size_t numOfKeys{};
            inFile.read((char*)&numOfKeys, sizeof(size_t));

            resource->keys.resize(numOfKeys);

            for (size_t b = 0; b < numOfKeys; b++)
            {
                size_t numPositions{};
                inFile.read((char*)&numPositions, sizeof(size_t));

                resource->keys[b].positions.resize(numPositions);
                inFile.read((char*)resource->keys[b].positions.data(), sizeof(PositionKey) * numPositions);

                size_t numRotations{};
                inFile.read((char*)&numRotations, sizeof(size_t));

                resource->keys[b].rotations.resize(numRotations);
                inFile.read((char*)resource->keys[b].rotations.data(), sizeof(RotationKey) * numRotations);

                size_t numScales{};
                inFile.read((char*)&numScales, sizeof(size_t));

                resource->keys[b].scales.resize(numScales);
                inFile.read((char*)resource->keys[b].scales.data(), sizeof(ScaleKey) * numScales);
            }

            size_t numEvents{};
            inFile.read((char*)&numEvents, sizeof(size_t));

            if (numEvents > 0)
            {
                resource->events.resize(numEvents);
                inFile.read((char*)resource->events.data(), sizeof(EventKey) * numEvents);
            }

            inFile.read((char*)&resource->hasRootMotion, sizeof(bool));
            inFile.read((char*)&resource->length, sizeof(float));
            inFile.read((char*)&resource->framerate, sizeof(float));

            inFile.close();

            return resource;
        }

        LOG_ERROR("Failed to open animation file: %s", filePath.c_str());
        return nullptr;
    }

    template<>
    void AssetRegistry::WriteAsset(const Animation& asset, const AssetMeta& meta) const
    {
        const std::string& filePath = GetAbsolutePath(meta.assetPath);

        std::ofstream outFile(filePath, std::ios::binary);

        if (outFile.is_open())
        {
            const size_t numOfKeys = asset.keys.size();
            outFile.write((const char*)&numOfKeys, sizeof(size_t));

            for (size_t b = 0; b < asset.keys.size(); b++)
            {
                const size_t numPositions = asset.keys[b].positions.size();
                outFile.write((const char*)&numPositions, sizeof(size_t));
                outFile.write((const char*)asset.keys[b].positions.data(), sizeof(PositionKey) * numPositions);

                const size_t numRotations = asset.keys[b].rotations.size();
                outFile.write((const char*)&numRotations, sizeof(size_t));
                outFile.write((const char*)asset.keys[b].rotations.data(), sizeof(RotationKey) * numRotations);

                const size_t numScales = asset.keys[b].scales.size();
                outFile.write((const char*)&numScales, sizeof(size_t));
                outFile.write((const char*)asset.keys[b].scales.data(), sizeof(ScaleKey) * numScales);
            }

            const size_t numEvents = asset.events.size();
            outFile.write((const char*)&numEvents, sizeof(size_t));
            if (numEvents > 0)
            {
                outFile.write((const char*)asset.events.data(), sizeof(EventKey) * numEvents);
            }

            outFile.write((const char*)&asset.hasRootMotion, sizeof(bool));
            outFile.write((const char*)&asset.length, sizeof(float));
            outFile.write((const char*)&asset.framerate, sizeof(float));

            outFile.flush();
            outFile.close();

            return;
        }

        LOG_ERROR("Failed to open animation file for writing: %s", filePath.c_str());
    }

    template<>
    std::shared_ptr<AnimationSet> AssetRegistry::LoadAsset(const AssetMeta& meta)
    {
        const std::string& filePath = GetAbsolutePath(meta.assetPath);

        std::ifstream inFile(filePath, std::ios::binary);
        if (inFile.is_open())
        {
            nlohmann::json json = nlohmann::json::parse(inFile);
            std::shared_ptr<AnimationSet> resource = std::make_shared<AnimationSet>();

            if (json.contains("animations") && json["animations"].is_array())
            {
                for (const auto& item : json["animations"])
                {
                    const AssetId animId = item.get<AssetId>();
                    auto anim = Load<Animation>(animId);
                    if (anim)
                    {
                        resource->animations.emplace_back(anim);
                    }
                }
            }

            if (json.contains("transitions") && json["transitions"].is_array())
            {
                for (const auto& transition : json["transitions"])
                {
                    LoadTransition(transition, resource->transitions.emplace_back());
                }
            }

            if (json.contains("defaultAnimation"))
            {
                const AssetId defaultAnimId = json["defaultAnimation"].get<AssetId>();
                for (size_t i = 0; i < resource->animations.size(); i++)
                {
                    if (resource->animations[i]->id == defaultAnimId)
                    {
                        resource->defaultAnimation = static_cast<int>(i);
                        break;
                    }
                }
            }

            if (json.contains("defaultBlendTime"))
            {
                resource->defaultBlendTime = json["defaultBlendTime"].get<float>();
            }

            if (json.contains("defaultTargetFrame"))
            {
                resource->defaultTargetFrame = json["defaultTargetFrame"].get<float>();
            }

            if (json.contains("defaultShouldLoop"))
            {
                resource->defaultShouldLoop = json["defaultShouldLoop"].get<bool>();
            }

            if (json.contains("defaultShouldBlend"))
            {
                resource->defaultShouldBlend = json["defaultShouldBlend"].get<bool>();
            }

            inFile.close();
            return resource;
        }

        LOG_ERROR("Failed to open animation file: %s", filePath.c_str());
        return nullptr;
    }

    template<>
    void AssetRegistry::WriteAsset(const AnimationSet& asset, const AssetMeta& meta) const
    {
        const std::string& filePath = GetAbsolutePath(meta.assetPath);

        std::ofstream outFile(filePath, std::ios::binary);
        if (outFile.is_open())
        {
            nlohmann::json json;
            if (asset.animations.size() > 0)
            {
                for (size_t i = 0; i < asset.animations.size(); i++)
                {
                    auto& anim = asset.animations[i];
                    json["animations"][i] = anim ? anim->id : InvalidAssetId;
                }
            }

            if (asset.transitions.size() > 0)
            {
                for (size_t t = 0; t < asset.transitions.size(); t++)
                {
                    SaveTransition(json["transitions"][t], asset.transitions[t]);
                }
            }

            if (asset.defaultAnimation >= 0 && asset.defaultAnimation < static_cast<int>(asset.animations.size()))
            {
                json["defaultAnimation"] = asset.animations[asset.defaultAnimation]->id;
            }

            outFile << json.dump(4);
            outFile.flush();
            outFile.close();
            return;
        }

        LOG_ERROR("Failed to open animation file for writing: %s", filePath.c_str());
    }

    template<>
    std::shared_ptr<Level> AssetRegistry::LoadAsset(const AssetMeta& meta)
    {
        const std::string& name = GetAbsolutePath(meta.assetPath);

        std::ifstream inFile(name);

        if (inFile.is_open())
        {
            std::shared_ptr<Level> result = std::make_shared<Level>();

            nlohmann::json json = nlohmann::json::parse(inFile);

            for (const auto& item : json["models"])
            {
                const AssetId modelId = item.get<AssetId>();
                auto model = Load<Model>(modelId);
                if (model)
                {
                    result->models.push_back(model);
                }
            }

            for (const auto& item : json["meshes"])
            {
                auto& instance = result->meshes.emplace_back();
                instance.name = item["name"].get<std::string>();
                instance.model = item["model"].get<uint32_t>();
                instance.mesh = item["mesh"].get<uint32_t>();
                const auto& transform = item["transform"];
                instance.transform.position = glm::vec3{ transform[0], transform[1], transform[2] };
                instance.transform.rotation = glm::quat{ transform[3], transform[4], transform[5], transform[6] };
                instance.transform.scale = glm::vec3{ transform[7], transform[8], transform[9] };
                if (item.contains("overrideMaterial"))
                {
                    const AssetId materialId = item["overrideMaterial"].get<AssetId>();
                    instance.overrideMaterial = Load<Material>(materialId);
                }
                const auto& bounds = item["bounds"];
                const auto& min = bounds["min"];
                instance.bounds.min = glm::vec3{ min[0], min[1], min[2] };
                const auto& max = bounds["max"];
                instance.bounds.max = glm::vec3{ max[0], max[1], max[2] };
                const auto& lights = item["lights"];
                instance.lights = {
                    lights[0],
                    lights[1],
                    lights[2],
                    lights[3],
                    lights[4],
                    lights[5],
                    lights[6],
                    lights[7]
                };
                instance.lightCount = item["lightCount"].get<uint8_t>();
                instance.modelMatrix = instance.transform.AsMatrix();
            }

            if (json.contains("collisionMeshes"))
            {
                result->collisionMeshes.reserve(json["collisionMeshes"].size());
                for (const auto& item : json["collisionMeshes"])
                {
                    const AssetId meshId = item.get<AssetId>();
                    auto mesh = Load<CollisionMesh>(meshId);
                    if (mesh)
                    {
                        result->collisionMeshes.emplace_back(mesh);
                    }
                }
            }

            if (json.contains("collisionMeshInstances"))
            {
                for (const auto& item : json["collisionMeshInstances"])
                {
                    auto& instance = result->meshColliders.emplace_back();
                    const auto& transform = item["transform"];
                    instance.transform.position = glm::vec3{ transform[0], transform[1], transform[2] };
                    instance.transform.rotation = glm::quat{ transform[3], transform[4], transform[5], transform[6] };
                    instance.transform.scale = glm::vec3{ transform[7], transform[8], transform[9] };
                    instance.mesh = item["mesh"].get<uint32_t>();
                }
            }

            if (json.contains("boxColliders"))
            {
                result->boxColliders.reserve(json["boxColliders"].size());
                for (const auto& item : json["boxColliders"])
                {
                    auto& instance = result->boxColliders.emplace_back();
                    const auto& transform = item["transform"];
                    instance.transform.position = glm::vec3{ transform[0], transform[1], transform[2] };
                    instance.transform.rotation = glm::quat{ transform[3], transform[4], transform[5], transform[6] };
                    instance.transform.scale = glm::vec3{ transform[7], transform[8], transform[9] };
                    const auto& halfExtents = item["halfExtents"];
                    instance.halfExtents = glm::vec3{ halfExtents[0], halfExtents[1], halfExtents[2] };
                }
            }

            if (json.contains("pointLights"))
            {
                result->pointLights.reserve(json["pointLights"].size());
                for (const auto& item : json["pointLights"])
                {
                    PointLight& light = result->pointLights.emplace_back();
                    const auto& position = item["position"];
                    light.position = glm::vec3{ position[0], position[1], position[2] };
                    const auto& color = item["color"];
                    light.color = glm::vec3{ color[0], color[1], color[2] };
                    light.innerRadius = item["innerRadius"].get<float>();
                    light.outerRadius = item["outerRadius"].get<float>();
                    light.intensity = item["intensity"].get<float>();
                }
            }

            if (json.contains("directionalLight"))
            {
                const auto& dir = json["directionalLight"]["direction"];
                result->directionalLight.dir = glm::vec3{ dir[0], dir[1], dir[2] };
                const auto& color = json["directionalLight"]["color"];
                result->directionalLight.color = glm::vec3{ color[0], color[1], color[2] };
            }

            if (json.contains("ambientColor"))
            {
                const auto& color = json["ambientColor"];
                result->ambientColor = glm::vec3{ color[0], color[1], color[2] };
            }

            if (json.contains("ambientSound"))
            {
                const AssetId soundId = json["ambientSound"].get<AssetId>();
                if (IsValidAssetId(soundId))
                {
                    result->ambientSound = Load<Sound>(soundId);
                }
            }

            return result;
        }

        return nullptr;
    }

    template<>
    void AssetRegistry::WriteAsset(const Level& asset, const AssetMeta& meta) const
    {
        const std::string name = GetAbsolutePath(meta.assetPath);

        std::ofstream outFile(name);

        if (!outFile.is_open())
        {
            LOG_ERROR("Failed to open level file for writing: %s", name.c_str());
            return;
        }

        nlohmann::json json;

        for (size_t i = 0; i < asset.models.size(); i++)
        {
            auto& obj = asset.models[i];
            json["models"][i] = obj->id;
        }

        for (size_t i = 0; i < asset.meshes.size(); i++)
        {
            auto& obj = asset.meshes[i];
            json["meshes"][i]["name"] = obj.name;
            json["meshes"][i]["model"] = obj.model;
            json["meshes"][i]["mesh"] = obj.mesh;
            json["meshes"][i]["transform"] = 
            {
                obj.transform.position.x, obj.transform.position.y, obj.transform.position.z,
                obj.transform.rotation.w, obj.transform.rotation.x, obj.transform.rotation.y, obj.transform.rotation.z,
                obj.transform.scale.x, obj.transform.scale.y, obj.transform.scale.z
            };
            if (obj.overrideMaterial)
            {
                json["meshes"][i]["overrideMaterial"] = obj.overrideMaterial->id;
            }
            json["meshes"][i]["bounds"]["min"] = { obj.bounds.min.x, obj.bounds.min.y, obj.bounds.min.z };
            json["meshes"][i]["bounds"]["max"] = { obj.bounds.max.x, obj.bounds.max.y, obj.bounds.max.z };
            json["meshes"][i]["lights"] = {
                obj.lights[0],
                obj.lights[1],
                obj.lights[2],
                obj.lights[3],
                obj.lights[4],
                obj.lights[5],
                obj.lights[6],
                obj.lights[7]
            };  
            json["meshes"][i]["lightCount"] = obj.lightCount;
        }

        json["collisionMeshes"] = nlohmann::json::array();
        for (size_t c = 0; c < asset.collisionMeshes.size(); c++)
        {
            json["collisionMeshes"][c] = asset.collisionMeshes[c]->id;
        }

        json["collisionMeshInstances"] = nlohmann::json::array();
        for (size_t c = 0; c < asset.meshColliders.size(); c++)
        {
            auto& obj = asset.meshColliders[c];
            const auto& transform = obj.transform;
            json["collisionMeshInstances"][c]["transform"] = {
                transform.position.x, transform.position.y, transform.position.z,
                transform.rotation.w, transform.rotation.x, transform.rotation.y, transform.rotation.z,
                transform.scale.x, transform.scale.y, transform.scale.z
            };
            json["collisionMeshInstances"][c]["mesh"] = obj.mesh;
        }

        json["boxColliders"] = nlohmann::json::array();
        for (size_t b = 0; b < asset.boxColliders.size(); b++)
        {
            auto& obj = asset.boxColliders[b];
            const auto& transform = obj.transform;
            json["boxColliders"][b]["transform"] = {
                transform.position.x, transform.position.y, transform.position.z,
                transform.rotation.w, transform.rotation.x, transform.rotation.y, transform.rotation.z,
                transform.scale.x, transform.scale.y, transform.scale.z
            };
            json["boxColliders"][b]["halfExtents"] = { obj.halfExtents.x, obj.halfExtents.y, obj.halfExtents.z };
        }

        for (size_t i = 0; i < asset.pointLights.size(); i++)
        {
            auto& obj = asset.pointLights[i];
            json["pointLights"][i]["position"] = { obj.position.x, obj.position.y, obj.position.z };
            json["pointLights"][i]["color"] = { obj.color.r, obj.color.g, obj.color.b };
            json["pointLights"][i]["innerRadius"] = obj.innerRadius;
            json["pointLights"][i]["outerRadius"] = obj.outerRadius;
            json["pointLights"][i]["intensity"] = obj.intensity;
        }

        json["directionalLight"]["direction"] = {
            asset.directionalLight.dir.x,
            asset.directionalLight.dir.y,
            asset.directionalLight.dir.z
        };

        json["directionalLight"]["color"] = {
            asset.directionalLight.color.r,
            asset.directionalLight.color.g,
            asset.directionalLight.color.b
        };

        json["ambientColor"] = {
            asset.ambientColor.r,
            asset.ambientColor.g,
            asset.ambientColor.b
        };

        json["ambientSound"] = asset.ambientSound ? asset.ambientSound->id : InvalidAssetId;

        outFile << json.dump(4);
        outFile.flush();
        outFile.close();
    }

    template<>
    std::shared_ptr<CollisionMesh> AssetRegistry::LoadAsset(const AssetMeta& meta)
    {
        const std::string name = GetAbsolutePath(meta.assetPath);
        std::ifstream inFile(name);
        if (inFile.is_open())
        {
            BinaryReader reader(inFile);
            std::shared_ptr<CollisionMesh> resource = std::make_shared<CollisionMesh>();
            uint32_t numVertices = reader.ReadUInt32();
            resource->vertices.reserve(numVertices);
            for (uint32_t i = 0; i < numVertices; i++)
            {
                auto& v = resource->vertices.emplace_back();
                v.x = reader.ReadFloat();
                v.y = reader.ReadFloat();
                v.z = reader.ReadFloat();
            }
            uint32_t numIndices = reader.ReadUInt32();
            resource->indices.reserve(numIndices);
            for (uint32_t i = 0; i < numIndices; i++)
            {
                resource->indices.emplace_back(reader.ReadUInt32());
            }
            inFile.close();
            return resource;
        }
        else
        {
            LOG_ERROR("Failed to open collision mesh file: %s", name.c_str());
            return nullptr;
        }
    }

    template<>
    void AssetRegistry::WriteAsset(const CollisionMesh& asset, const AssetMeta& meta) const
    {
        const std::string name = GetAbsolutePath(meta.assetPath);
        std::ofstream outFile(name);
        if (!outFile.is_open())
        {
            LOG_ERROR("Failed to open collision mesh file for writing: %s", name.c_str());
            return;
        }

        BinaryWriter writer(outFile);
        writer.WriteUInt32(static_cast<uint32_t>(asset.vertices.size()));
        for (const auto& vertex : asset.vertices)
        {
            writer.WriteFloat(vertex.x);
            writer.WriteFloat(vertex.y);
            writer.WriteFloat(vertex.z);
        }
        writer.WriteUInt32(static_cast<uint32_t>(asset.indices.size()));
        for (const auto& index : asset.indices)
        {
            writer.WriteUInt32(index);
        }

        outFile.flush();
        outFile.close();
    }

    template<typename T>
    bool AssetRegistry::SaveAssetIfDirty(const AssetId id, AssetMeta& meta)
    {
        auto& map = AssetTraits<T>::GetMap(*this);
        auto it = map.find(id);
        if (it != map.end() && it->second)
        {
            const bool isDirty = it->second->isDirty;
            if (isDirty)
            {
                const std::string& name = it->second->name;
                const std::string oldPath = GetAbsolutePath(meta.assetPath);

                if (name.empty() || FileIO::IsAbsolutePath(name))
                {
                    LOG_ERROR("Tried to save dirty asset with empty or absolute path: %s", name.c_str());
                    return false;
                }

                const bool renamed = meta.name != name;
                meta.name = name;
                meta.assetPath = FileIO::GetBasePath(meta.assetPath) + FileIO::Separator + name + GetExtension(meta.type);

                WriteAsset<T>(*it->second, meta);
                it->second->isDirty = false;

                if (renamed && FileIO::FileExists(GetAbsolutePath(meta.assetPath)))
                {
                    if (FileIO::FileExists(oldPath))
                    {
                        FileIO::DeleteFile(oldPath);
                    }
                }
            }

            return true;
        }
        return false;
    }
}
