#include "Engine/Assets/ModelImporter.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <stb_image.h>

#include "Core/Debug.h"
#include "Core/IO/FileIO.h"
#include "Engine/Assets/TextureImport.h"
#include "Engine/Rendering/Material.h"
#include "Engine/Rendering/Texture.h"

namespace TombForge
{
    namespace
    {
        // To hold bone info temporarily during processing
        struct BoneInfo
        {
            glm::mat4 offset{};
            glm::mat4 transform{}; // Relative to parent
            glm::mat4 globalTransform{}; // Absolute in model space
        };
    }

    struct ImporterImpl
    {
        // Keeps the assimp library out of the header
        Assimp::Importer importer{};
    };

    std::string GetDirectoryFromFilePath(const std::string& filePath)
    {
        if (filePath.empty())
        {
            LOG_WARNING("Tried to get directory from an empty file path");
            return {};
        }

        const size_t lastSlash = filePath.find_last_of("/\\");

        if (lastSlash == std::string::npos)
        {
            LOG_WARNING("Tried to get directory from %s, but its not a file path", filePath);
            return filePath;
        }

        return filePath.substr(0, lastSlash);
    }

    std::string GetFilePathWithoutExtension(const std::string& filePath)
    {
        if (filePath.empty())
        {
            LOG_WARNING("Tried to get extensionless path from an empty file path");
            return {};
        }

        const size_t lastPeriod = filePath.find_last_of(".");

        if (lastPeriod == std::string::npos)
        {
            LOG_WARNING("Tried to remove extension from %s but already extensionless", filePath);
            return filePath;
        }

        return filePath.substr(0, lastPeriod);
    }

    std::string GetFileName(const std::string& filePath, bool includeExtension = false)
    {
        if (filePath.empty())
        {
            LOG_WARNING("Tried to get extensionless path from an empty file path");
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

    // Assimp Functions

    static inline glm::mat4 ConvertAssimpMatrixToGlm(const aiMatrix4x4& from)
    {
        // Assimp is row-major and glm is column-major
        glm::mat4 to;
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }

    static void FindAllBones(
        const aiScene* scene,
        const aiNode* node,
        const ImportSettings& settings,
        std::unordered_map<std::string, BoneInfo>& output,
        glm::mat4 parentTransform = glm::mat4(1.0f))
    {
        glm::mat4 nodeTransform = ConvertAssimpMatrixToGlm(node->mTransformation);
        nodeTransform[3][0] *= settings.scale;
        nodeTransform[3][1] *= settings.scale;
        nodeTransform[3][2] *= settings.scale;

        parentTransform = parentTransform * nodeTransform;

        // todo: More robust method of finding root bone (they're not skinned so Assimp doesn't pick it up as a bone).
        // Will probably need to allow user to specify it. This also doesn't handle transforms above the root bone.
        if (node->mName == aiString("ROOT") 
            || node->mName == aiString("Bone_0")
            || node->mName == aiString("Root"))
        {
            output.emplace(
                std::make_pair( node->mName.C_Str(), BoneInfo{ glm::inverse(parentTransform), parentTransform }));
        }

        for (unsigned int m = 0; m < node->mNumMeshes; m++)
        {
            const aiMesh* mesh = scene->mMeshes[node->mMeshes[m]];

            for (unsigned int b = 0; b < mesh->mNumBones; b++)
            {
                const aiBone* bone = mesh->mBones[b];

                if (output.contains(bone->mName.C_Str()))
                {
                    continue;
                }

                glm::mat4 offset = ConvertAssimpMatrixToGlm(bone->mOffsetMatrix);
                offset[3][0] *= settings.scale; // This is the translation part of matrix
                offset[3][1] *= settings.scale;
                offset[3][2] *= settings.scale;

                // Do not use the transform above because it messes up the skeleton
                glm::mat4 boneTransform = ConvertAssimpMatrixToGlm(bone->mNode->mTransformation);
                boneTransform[3][0] *= settings.scale; 
                boneTransform[3][1] *= settings.scale;
                boneTransform[3][2] *= settings.scale;

                output.emplace(std::make_pair( bone->mNode->mName.C_Str(),
                    BoneInfo{ offset, boneTransform } ));
            }
        }

        for (unsigned int c = 0; c < node->mNumChildren; c++)
        {
            FindAllBones(scene, node->mChildren[c], settings, output, parentTransform);
        }
    }

    static const aiNode* FindAssimpBoneRootNode(const aiNode* sceneRoot, const std::unordered_map<std::string, BoneInfo>& bones)
    {
        std::string nodeName = sceneRoot->mName.C_Str();
        if (bones.find(nodeName) != bones.end())
        {
            return sceneRoot;
        }

        for (unsigned int i = 0, count = sceneRoot->mNumChildren; i < count; i++)
        {
            const aiNode* result = FindAssimpBoneRootNode(sceneRoot->mChildren[i], bones);
            if (result)
            {
                return result;
            }
        }

        return nullptr;
    }

    static void ProcessSkeleton(
        const aiNode* node,
        std::vector<Bone>& bones,
        const std::unordered_map<std::string, BoneInfo>& boneSet,
        const ImportSettings& settings,
        uint8_t parent = 0)
    {
        const std::string boneName = node->mName.C_Str();

        // Only time NOT finding would happen if there was a non-skinned node between
        if (boneSet.find(boneName) != boneSet.end())
        {
            const BoneInfo& info = boneSet.at(boneName);

            // If there is any kind of global transform on the root bone, we need to bake it into the offset
            bones.emplace_back(boneName, info.offset, info.transform, parent);
            parent = static_cast<uint8_t>(bones.size() - 1);
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            const aiNode* childNode = node->mChildren[i];
            ProcessSkeleton(childNode, bones, boneSet, settings, parent);
        }
    }

    static bool ProcessEmbeddedTexture(Texture& outTexture, const aiTexture* texture)
    {
        if (!texture)
        {
            LOG_ERROR("Tried to process a null embedded texture");
            return false;
        }

        outTexture.width = texture->mWidth;
        outTexture.height = texture->mHeight;
        outTexture.format = TextureFormat::RGBA; // Assimp uses 4 channels for everything

        if (outTexture.height == 0) // Compressed texture
        {
            int width{};
            int height{};
            int channels{};

            stbi_uc* imageData = stbi_load_from_memory(
                reinterpret_cast<const stbi_uc*>(texture->pcData),
                static_cast<int>(texture->mWidth),
                &width,
                &height,
                &channels,
                STBI_rgb_alpha);

            if (imageData)
            {
                const size_t dataSize = width * height * 4;
                outTexture.data.resize(dataSize);
                memcpy(outTexture.data.data(),imageData, dataSize);
                outTexture.width = static_cast<uint32_t>(width);
                outTexture.height = static_cast<uint32_t>(height);
                stbi_image_free(imageData);
            }
            else
            {
                stbi_image_free(imageData);
                LOG_WARNING("STBI failed to load embedded compressed texture: %s", stbi_failure_reason());
                return false;
            }
        }
        else // Uncompressed texture
        {
            const size_t dataSize = outTexture.width * outTexture.height * 4;
            outTexture.data.resize(dataSize);
            memcpy(outTexture.data.data(), texture->pcData, dataSize);
        }
        return true;
    }

    static std::shared_ptr<Texture> ProcessTexture(const aiScene* scene, aiString texturePath, const std::string basePath)
    {
        std::shared_ptr<Texture> texture = std::make_shared<Texture>();

        if (texturePath.data[0] == '*') // Embedded starts with *
        {
            if (!ProcessEmbeddedTexture(*texture, scene->GetEmbeddedTexture(texturePath.C_Str())))
            {
                return nullptr;
            }
        }
        else
        {
            std::string fullTexturePath = basePath;
            if (FileIO::IsAbsolutePath(texturePath.C_Str()))
            {
                fullTexturePath = texturePath.C_Str();
            }
            else
            {
                fullTexturePath = basePath;
                fullTexturePath.append(texturePath.C_Str());
            }

            const std::string textureFileName = GetFileName(texturePath.C_Str());
            const std::string textureWithExt = GetFileName(texturePath.C_Str(), true);

            if (!FileIO::FileExists(fullTexturePath))
            {
                std::string possiblePath{};
                if (FileIO::SearchForFile(textureWithExt, basePath, &possiblePath))
                {
                    fullTexturePath = possiblePath;
                }
                else
                {
                    LOG_WARNING("Couldn't find texture %s", fullTexturePath.c_str());
                    return nullptr;
                }
            }

            if (!ImportTexture(fullTexturePath, *texture))
            {
                LOG_WARNING("Could not import texture: %s", fullTexturePath.c_str());
                return nullptr;
            }
        }

        return texture;
    }

    static void FillMaterial(const aiScene* scene, const aiMesh* mesh, Material& outMaterial, const std::string& outBaseName, const std::string& basePath)
    {
        const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        outMaterial.name = outBaseName;
        outMaterial.name.append("_M_");
        outMaterial.name.append(std::to_string(mesh->mMaterialIndex));
        outMaterial.name.append("_");
        outMaterial.name.append(material->GetName().C_Str());
        outMaterial.name.append(".tfmat");

        const aiTextureType diffuseTypesSearch[]{ aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR, aiTextureType_AMBIENT };
        for (const aiTextureType type : diffuseTypesSearch)
        {
            for (unsigned int t = 0; t < material->GetTextureCount(type); t++)
            {
                aiString texturePath;
                material->GetTexture(type, t, &texturePath);
                if (texturePath.length < 1)
                {
                    continue;
                }

                if (auto texture = ProcessTexture(scene, texturePath, basePath); texture)
                {
                    texture->name = outBaseName;
                    texture->name.append("_T_");
                    texture->name.append(std::to_string(t));
                    texture->name.append("_");
                    texture->name.append(texturePath.data[0] == '*' ? "Embedded" : FileIO::GetFileName(texturePath.data));
                    texture->name.append(".tftex");
                    texture->sRGB = true;

                    outMaterial.diffuse = texture;
                    outMaterial.AddFlag(MATERIAL_FLAG_DIFFUSE);

                    break;
                }
            }
        }

        const aiTextureType normalTypesSearch[]{ aiTextureType_NORMALS, aiTextureType_HEIGHT, aiTextureType_DISPLACEMENT };
        for (const aiTextureType type : normalTypesSearch)
        {
            for (unsigned int t = 0; t < material->GetTextureCount(type); t++)
            {
                aiString texturePath;
                material->GetTexture(type, t, &texturePath);
                if (texturePath.length < 1)
                {
                    continue;
                }

                if (auto texture = ProcessTexture(scene, texturePath, basePath); texture)
                {
                    texture->name = outBaseName;
                    texture->name.append("_N_");
                    texture->name.append(std::to_string(t));
                    texture->name.append("_");
                    texture->name.append(texturePath.data[0] == '*' ? "Embedded" : FileIO::GetFileName(texturePath.data));
                    texture->name.append(".tftex");
                    texture->sRGB = false;

                    outMaterial.normal = texture;
                    outMaterial.AddFlag(MATERIAL_FLAG_NORMAL);

                    break;
                }
            }
        }
    }

    static void ProcessMesh(
        const aiScene* scene,
        const aiNode* node,
        const std::string& basePath,
        const std::string& outBaseName,
        std::vector<Mesh>& meshesOut,
        const std::string name,
        const Skeleton* skeleton, // can be null
        const ImportSettings& settings,
        glm::mat4 recursiveTransform)
    {
        const glm::mat4 nodeTransform = ConvertAssimpMatrixToGlm(node->mTransformation);
        recursiveTransform *= nodeTransform;

        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            unsigned int meshSceneIndex = node->mMeshes[i];

            const aiMesh* mesh = scene->mMeshes[meshSceneIndex];

            Mesh& outMesh = meshesOut.emplace_back();
            outMesh.name = mesh->mName.C_Str();
            outMesh.vertices.reserve(mesh->mNumVertices);
            outMesh.indices.reserve(static_cast<size_t>(mesh->mNumFaces) * 3);

            for (unsigned int v = 0; v < mesh->mNumVertices; v++)
            {
                auto& vertex = outMesh.vertices.emplace_back();

                vertex.position = { mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z };
                vertex.position *= settings.scale; // Do this separate from transform as we don't want to scale normals

                if (mesh->HasNormals())
                {
                    vertex.normal = { mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z };
                }

                if (mesh->HasTangentsAndBitangents())
                {
                    vertex.tangent = { mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z };
                    vertex.bitangent = { mesh->mBitangents[v].x, mesh->mBitangents[v].y, mesh->mBitangents[v].z };
                }

                if (mesh->HasVertexColors(0))
                {
                    vertex.color = { mesh->mColors[0][v].r, mesh->mColors[0][v].g, mesh->mColors[0][v].b, mesh->mColors[0][v].a };
                }

                if (mesh->HasTextureCoords(0))
                {
                    vertex.uv = { mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y };
                }
            }

            // More efficient to do this separately instead of looping bones for each vertex
            if (mesh->HasBones() && skeleton)
            {
                for (unsigned int b = 0; b < mesh->mNumBones; b++)
                {
                    const aiBone* bone = mesh->mBones[b];
                    const std::string boneName = bone->mName.C_Str();

                    for (size_t k = 0; k < skeleton->bones.size(); k++)
                    {
                        if (boneName != skeleton->bones[k].name)
                        {
                            continue;
                        }

                        for (unsigned int w = 0; w < bone->mNumWeights; w++)
                        {
                            const unsigned int vertexId = bone->mWeights[w].mVertexId;
                            const float weight = bone->mWeights[w].mWeight;

                            // Find the first free bone slot for this
                            for (int id = 0; id < 4; id++)
                            {
                                if (outMesh.vertices[vertexId].boneIndices[id] == k)
                                {
                                    // This is already added
                                    break;
                                }
                                else if (outMesh.vertices[vertexId].boneIndices[id] < 0)
                                {
                                    outMesh.vertices[vertexId].boneIndices[id] = static_cast<int>(k);
                                    outMesh.vertices[vertexId].boneWeights[id] = weight;
                                    break; // No need to fill out more
                                }
                            }
                        }

                        // Found matching bone so break early
                        break;
                    }
                }
            }

            for (unsigned int e = 0; e < mesh->mNumFaces; e++)
            {
                outMesh.indices.emplace_back(mesh->mFaces[e].mIndices[0]);
                outMesh.indices.emplace_back(mesh->mFaces[e].mIndices[1]);
                outMesh.indices.emplace_back(mesh->mFaces[e].mIndices[2]);
            }

            CalculateBoundingBox(outMesh);

            outMesh.material = std::make_shared<Material>();
            FillMaterial(scene, mesh, *outMesh.material, outBaseName, basePath);
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            ProcessMesh(scene, node->mChildren[i], basePath, outBaseName, meshesOut, name, skeleton, settings, recursiveTransform);
        }
    }

    static void ProcessAnimation(const aiScene* scene, const Skeleton& skeleton, const ImportSettings& settings, Animation& result)
    {
        result.keys.clear();
        result.keys.resize(skeleton.bones.size());

        for (unsigned int i = 0; i < scene->mNumAnimations; i++)
        {
            const aiAnimation* aiAnim = scene->mAnimations[i];

            result.length = static_cast<float>(aiAnim->mDuration);
            
            const float fileFramerate = static_cast<float>(aiAnim->mTicksPerSecond);
            if (fileFramerate != 0.0f)
            {
                result.framerate = fileFramerate;
            }

            for (unsigned int c = 0; c < aiAnim->mNumChannels; c++)
            {
                const aiNodeAnim* node = aiAnim->mChannels[c];
                const std::string boneName = node->mNodeName.C_Str();

                const uint8_t boneId = skeleton.FindBoneId(boneName);
                if (boneId == 255)
                {
                    LOG_WARNING("Bone %s not found in skeleton %s", boneName.c_str(), skeleton.name.c_str());
                    continue;
                }

                BoneKeys& keys = result.keys[boneId];
                keys.positions.reserve(node->mNumPositionKeys);
                keys.rotations.reserve(node->mNumRotationKeys);
                keys.scales.reserve(node->mNumScalingKeys);

                for (unsigned int p = 0; p < node->mNumPositionKeys; p++)
                {
                    PositionKey& key = keys.positions.emplace_back();
                    key.time = node->mPositionKeys[p].mTime;
                    key.value.x = node->mPositionKeys[p].mValue.x * settings.scale;
                    key.value.y = node->mPositionKeys[p].mValue.y * settings.scale;
                    key.value.z = node->mPositionKeys[p].mValue.z * settings.scale;
                }

                for (unsigned int s = 0; s < node->mNumScalingKeys; s++)
                {
                    // Don't scale this by the setting because we already scale the position
                    ScaleKey& key = keys.scales.emplace_back();
                    key.time = node->mScalingKeys[s].mTime;
                    key.value.x = node->mScalingKeys[s].mValue.x;
                    key.value.y = node->mScalingKeys[s].mValue.y;
                    key.value.z = node->mScalingKeys[s].mValue.z;
                }

                for (unsigned int r = 0; r < node->mNumRotationKeys; r++)
                {
                    RotationKey& key = keys.rotations.emplace_back();
                    key.time = node->mRotationKeys[r].mTime;
                    key.value.x = node->mRotationKeys[r].mValue.x;
                    key.value.y = node->mRotationKeys[r].mValue.y;
                    key.value.z = node->mRotationKeys[r].mValue.z;
                    key.value.w = node->mRotationKeys[r].mValue.w;
                }
            }
        }
    }

    // AssetImportSession Functions

    AssetImportSession::AssetImportSession()
        : m_path{}, m_importer{ new ImporterImpl() }
    {
    }

    AssetImportSession::AssetImportSession(const std::string& path)
        : m_path{ path }, m_importer{ new ImporterImpl() }
    {
        Start(path);
    }

    AssetImportSession::~AssetImportSession()
    {
        if (m_importer)
        {
            m_importer->importer.FreeScene();
            delete m_importer;
        }
    }

    bool AssetImportSession::Start(const std::string& path)
    {
        Finish();

        Assimp::Importer& importer = m_importer->importer;
        const aiScene* scene = importer.ReadFile(path, 
            aiProcess_Triangulate 
            | aiProcess_FlipUVs 
            | aiProcess_PopulateArmatureData
            | aiProcess_CalcTangentSpace);

        if (!scene)
        {
            LOG_ERROR("Failed to load %s", path);
            return false;
        }

        m_contains.hasModel = CheckForModel();
        m_contains.hasSkeleton = CheckForSkeleton();
        m_contains.hasAnimation = CheckForAnimation();

        m_path = path;

        return true;
    }

    ImportResult AssetImportSession::Import(const ImportSettings& settings)
    {
        if (!m_importer)
        {
            LOG_ERROR("Tried to import an asset but the importer was not initialized");
            return {};
        }

        Assimp::Importer& importer = m_importer->importer;
        const aiScene* scene = importer.GetScene();

        const std::string basePath = m_path.substr(0, m_path.find_last_of("\\/") + 1);

        ImportResult result{};

        if (m_contains.hasSkeleton && settings.importSkeleton)
        {
            // First find all the nodes that are actually bones with info
            std::unordered_map<std::string, BoneInfo> allBonesSet;
            FindAllBones(scene, scene->mRootNode, settings, allBonesSet);

            // Find skeleton root node
            const aiNode* rootNode = FindAssimpBoneRootNode(scene->mRootNode, allBonesSet);

            // Then fill out the skeleton in a sorted order
            result.skeleton = std::make_shared<Skeleton>();
            result.skeleton->name = settings.skeletonPath;

            ProcessSkeleton(rootNode, result.skeleton->bones, allBonesSet, settings);
        }

        if (m_contains.hasModel && settings.importModel)
        {
            const std::string baseName = GetFilePathWithoutExtension(settings.modelPath);

            result.model = std::make_shared<Model>();
            result.model->name = settings.modelPath;
            result.model->skeleton = result.skeleton;

            glm::mat4 transformMatrix{ 1.0f };
            ProcessMesh(scene, scene->mRootNode, basePath, baseName, result.model->meshes, settings.modelPath, result.skeleton.get(), settings, transformMatrix);
            CalculateBoundingBox(*result.model);
        }

        if (m_contains.hasAnimation && settings.importAnimation)
        {
            const Skeleton* skeleton = result.skeleton ? result.skeleton.get() : settings.existingSkeleton.get();

            if (skeleton)
            {
                result.animation = std::make_shared<Animation>();
                result.animation->name = settings.animationPath;

                ProcessAnimation(scene, *skeleton, settings, *result.animation);
            }
            else
            {
                LOG_ERROR("Couldn't import animation due to missing skeleton");
            }
        }

        return result;
    }

    void AssetImportSession::Finish()
    {
        if (m_importer)
        {
            m_importer->importer.FreeScene();
        }
    }

    bool AssetImportSession::CheckForModel() const
    {
        return m_importer->importer.GetScene()->HasMeshes();
    }

    bool AssetImportSession::CheckForSkeleton() const
    {
        if (CheckForModel())
        {
            for (unsigned int i = 0; i < m_importer->importer.GetScene()->mNumMeshes; i++)
            {
                const aiMesh* mesh = m_importer->importer.GetScene()->mMeshes[i];
                if (mesh->HasBones())
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool AssetImportSession::CheckForAnimation() const
    {
        return m_importer->importer.GetScene()->HasAnimations();
    }
}
