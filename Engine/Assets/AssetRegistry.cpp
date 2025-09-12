#include "Engine/Assets/AssetRegistry.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <sndfile.h>

#include "Core/IO/FileIO.h"
#include "Engine/Animation/Animation.h"
#include "Engine/Animation/Skeleton.h"
#include "Engine/Audio/Sound.h"
#include "Engine/Levels/Level.h"
#include "Engine/Rendering/Material.h"
#include "Engine/Rendering/Model.h"
#include "Engine/Rendering/Texture.h"

namespace TombForge
{
    void AssetRegistry::Init(const std::string& projectPath)
    {
        if (!FileIO::IsDirectory(projectPath))
        {
            LOG_ERROR("Project path is not a directory: %s", projectPath.c_str());
            return;
        }

        m_basePath = projectPath + "/";
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
                meta.sourcePath = item.contains("sourcePath") ? item["sourcePath"].get<std::string>() : "";
                meta.assetPath = item["assetPath"].get<std::string>();
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
                case ASSET_TYPE_SOUND:
                    break; // No data to save for sounds yet
                }

                nlohmann::json item{};
                item["id"] = meta.id;
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

                inFile.read((char*)mesh.vertices.data(), numVertices * sizeof(decltype(mesh.vertices)::value_type));

                size_t numIndices{};
                inFile.read((char*)&numIndices, sizeof(size_t));

                mesh.indices.resize(numIndices);

                inFile.read((char*)mesh.indices.data(), numIndices * sizeof(uint32_t));

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
                outFile.write((const char*)mesh.vertices.data(), numVertices * sizeof(decltype(mesh.vertices)::value_type));

                const size_t numIndices = mesh.indices.size();
                outFile.write((const char*)&numIndices, sizeof(size_t));
                outFile.write((const char*)mesh.indices.data(), numIndices * sizeof(uint32_t));

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
            resource->diffuse = Load<Texture>(diffusePath);
            resource->AddFlag(MATERIAL_FLAG_DIFFUSE);
        }

        if (json.contains("normal"))
        {
            const AssetId normalPath = json["normal"].get<AssetId>();
            resource->normal = Load<Texture>(normalPath);
            resource->AddFlag(MATERIAL_FLAG_NORMAL);
        }

        if (json.contains("baseColor"))
        {
            auto baseColor = json["baseColor"];
            resource->baseColor = glm::vec4{ baseColor[0], baseColor[1], baseColor[2], baseColor[3] };
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
            if (asset.diffuse)
            {
                json["diffuse"] = asset.diffuse->id;
            }
            if (asset.normal)
            {
                json["normal"] = asset.normal->id;
            }
            json["baseColor"] = { asset.baseColor.r, asset.baseColor.g, asset.baseColor.b, asset.baseColor.a };
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

            inFile.close();

            return resource;
        }

        return nullptr;
    }

    template<>
    void AssetRegistry::WriteAsset(const Texture& asset, const AssetMeta& meta) const
    {
        const std::string name = GetAbsolutePath(meta.assetPath);

        if (!asset.IsValidData())
        {
            LOG_ERROR("Tried to save texture %s, but invalid data");
            return;
        }

        const std::string& filePath = name;

        std::ofstream outFile(filePath, std::ios::binary);

        if (outFile.is_open())
        {
            outFile.write((const char*)&asset.format, sizeof(TextureFormat));
            outFile.write((const char*)&asset.width, sizeof(uint32_t));
            outFile.write((const char*)&asset.height, sizeof(uint32_t));
            outFile.write((const char*)asset.data.data(), sizeof(ColorByte) * asset.data.size());

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
                MeshInstance instance{};
                instance.name = item["name"].get<std::string>();
                instance.model = item["model"].get<AssetId>();
                instance.mesh = item["mesh"].get<AssetId>();
                const auto& transform = item["transform"];
                instance.transform.position = glm::vec3{ transform[0], transform[1], transform[2] };
                instance.transform.rotation = glm::quat{ transform[3], transform[4], transform[5], transform[6] };
                instance.transform.scale = glm::vec3{ transform[7], transform[8], transform[9] };
                result->meshes.push_back(instance);
            }

            for (const auto& item : json["pointLights"])
            {
                PointLight light{};
                const auto& position = item["position"];
                light.position = glm::vec3{ position[0], position[1], position[2] };
                const auto& color = item["color"];
                light.color = glm::vec3{ color[0], color[1], color[2] };
                light.innerRadius = item["innerRadius"].get<float>();
                light.outerRadius = item["outerRadius"].get<float>();
                result->pointLights.push_back(light);
            }

            if (json.contains("directionalLight"))
            {
                const auto& dir = json["directionalLight"]["direction"];
                result->directionalLight.dir = glm::vec3{ dir[0], dir[1], dir[2] };
                const auto& color = json["directionalLight"]["color"];
                result->directionalLight.color = glm::vec3{ color[0], color[1], color[2] };
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
        }

        for (size_t i = 0; i < asset.pointLights.size(); i++)
        {
            auto& obj = asset.pointLights[i];
            json["pointLights"][i]["position"] = { obj.position.x, obj.position.y, obj.position.z };
            json["pointLights"][i]["color"] = { obj.color.r, obj.color.g, obj.color.b };
            json["pointLights"][i]["innerRadius"] = obj.innerRadius;
            json["pointLights"][i]["outerRadius"] = obj.outerRadius;
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

        json["ambientSound"] = asset.ambientSound ? asset.ambientSound->id : InvalidAssetId;

        outFile << json.dump(4);
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

                const bool renamed = meta.assetPath != name;
                meta.assetPath = name;

                WriteAsset<T>(*it->second, meta);
                it->second->isDirty = false;

                if (renamed && FileIO::FileExists(GetAbsolutePath(name)))
                {
                    if (FileIO::FileExists(oldPath))
                    {
                        FileIO::DeleteFile(oldPath);
                    }
                }
            }

            return true;
        }
    }
}
