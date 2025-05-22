#include "GmxImport.h"

#include "../../Core/Debug.h"
#include "../../Core/Graphics/Model.h"
#include "../../Core/Graphics/Material.h"
#include "../../Core/Graphics/Texture.h"
#include "../../Core/IO/FileIO.h"
#include "../../Core/Maths/Maths.h"
#include "TextureImport.h"

#include <LevelExport.h>

namespace TombForge
{
    struct RoomObjInfo
    {
        int roomNum{};
        int objNum{};
    };

    bool GetRoomObj(const std::string& meshName, RoomObjInfo& result)
    {
        size_t roomPos = meshName.find("_ROOM_");
        if (roomPos == std::string::npos)
        {
            return false;
        }

        size_t numPos = roomPos + 6;
        size_t endNumPos = meshName.find('_', numPos);

        result.roomNum = std::stoi(meshName.substr(numPos, endNumPos - numPos));

        size_t objPos = meshName.find("_OBJ_");
        if (objPos == std::string::npos)
        {
            return false;
        }

        size_t objNumPos = objPos + 5;
        size_t endObjNumPos = meshName.find('_', objNumPos);
        if (endObjNumPos == meshName.npos)
        {
            endObjNumPos = meshName.size();
        }

        result.objNum = std::stoi(meshName.substr(objNumPos, endObjNumPos - objNumPos));

        return true;
    }

    GmxResult ImportGmx(const std::string& filePath, const GmxImportSettings& settings)
    {
        MA_EXPORT outRmx{};
        MA_EXPORT outZone{};
        MA_EXPORT outCam{};
        MA_EXPORT outCln{};

        GmxResult result{};

        if (ExportError err = ExportLevel(filePath, outRmx, outZone, outCam, outCln); err == ExportError::None)
        {
            Transform adjustment{};
            adjustment.scale = { settings.scale, settings.scale, settings.scale };
            adjustment.rotation = glm::vec3{ settings.makeYUp ? glm::radians(-90.0f) : 0.0f, 0.0f, 0.0f };

            Transform adjustmentRotationOnly{};
            adjustmentRotationOnly.rotation = adjustment.rotation;

            for (size_t i = 0; i < outRmx.Light.size(); i++)
            {
                if (outRmx.Light[i].layer.find("duplicated") != std::string::npos)
                {
                    continue;
                }

                auto& light = outRmx.Light[i];
                if (light.type.find("Point") == std::string::npos)
                {
                    LOG("Importing light type: %s", light.type.c_str());
                }
                auto& newLight = result.lights.emplace_back();
                newLight.color = { light.R, light.G, light.B };
                newLight.intensity = 1.0f; // Not provided by exporter (it just uses radius)
                newLight.position = adjustment.AsMatrix() * glm::vec4{ light.tX, light.tY, light.tZ, 1.0f };
                newLight.innerRadius = light.Decay_Far_Start * settings.scale;
                newLight.outerRadius = light.Decay_Far_End * settings.scale;
            }

            //result.geometry = std::make_shared<Model>();

            std::vector<Transform> transforms{};
            transforms.reserve(outZone.Transform.size());

            std::vector<std::shared_ptr<Material>> materialsIn{};
            materialsIn.reserve(outZone.Material.size());

            // Fill out transforms
            for (size_t t = 0; t < outZone.Transform.size(); t++)
            {
                auto& trans = transforms.emplace_back();
                trans.position.x = outZone.Transform[t].tX;
                trans.position.y = outZone.Transform[t].tY;
                trans.position.z = outZone.Transform[t].tZ;

                trans.scale.x = outZone.Transform[t].sX;
                trans.scale.y = outZone.Transform[t].sY;
                trans.scale.z = outZone.Transform[t].sZ;

                trans.rotation = glm::quat({ glm::radians(outZone.Transform[t].rX), glm::radians(outZone.Transform[t].rY), glm::radians(outZone.Transform[t].rZ) });

                if (outZone.Transform[t].parent.size() > 0)
                {
                    bool found = false;
                    size_t pI{};
                    for (pI = 0; pI < outZone.Transform.size(); pI++)
                    {
                        if (outZone.Transform[pI].name == outZone.Transform[t].parent)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (found)
                    {
                        trans = transforms[pI] * trans;
                    }
                    else
                    {
                        LOG_WARNING("Did not find a parent transform for %s", outZone.Transform[t].name.c_str());
                    }
                }
            }

            // File type not supported, but the equivalent tga is available
            std::vector<std::string> excludeExt{ ".dds" };

            // Fill out materials
            for (size_t m = 0; m < outZone.Material.size(); m++)
            {
                auto& mat = materialsIn.emplace_back();
                mat = std::make_shared<Material>();
                mat->name = outZone.Material[m].name;

                std::string textureName{};
                if (outZone.Material[m].color.size() > 0)
                {
                    textureName = outZone.Material[m].color;
                }

                if (outZone.Material[m].transparency.size() > 0)
                {
                    textureName = outZone.Material[m].transparency;
                    mat->AddFlag(MATERIAL_FLAG_TRANSPARENT);
                }

                if (textureName.size() > 0)
                {
                    std::string texturePath{};
                    for (size_t t = 0; t < outZone.Texture.size(); t++)
                    {
                        if (textureName == outZone.Texture[t].name)
                        {
                            texturePath = outZone.Texture[t].filename;
                        }
                    }

                    bool exists = FileIO::FileExists(texturePath);
                    if (!exists)
                    {
                        texturePath.clear();

                        const std::string basePath = FileIO::GetBasePath(filePath);
                        const std::string fileName = FileIO::GetFileName(outZone.Material[m].color, false);

                        exists = FileIO::SearchForFile(fileName, basePath, &texturePath, &excludeExt);
                    }

                    mat->diffuse = std::make_shared<Texture>();
                    mat->diffuse->name = outZone.Material[m].name;
                    if (!exists || !ImportTexture(texturePath, *mat->diffuse))
                    {
                        LOG_WARNING("Could not import texture %s", mat->diffuse->name);
                        mat->diffuse = nullptr;
                    }
                    else
                    {
                        mat->AddFlag(MATERIAL_FLAG_DIFFUSE);
                    }
                }
            }

            std::unordered_map<int, std::shared_ptr<Model>> roomToModel{};
            std::unordered_map<int, std::unordered_set<int>> filledObjects{};

            // Fill out the geometry
            for (auto& mesh : outZone.Mesh)
            {
                RoomObjInfo info{};
                bool foundRoomInfo = GetRoomObj(mesh.name, info);

                if (mesh.layer.find("_bounding") != std::string::npos)
                {
                    std::shared_ptr<Model> boundModel{};
                    if (roomToModel.find(info.roomNum) != roomToModel.end())
                    {
                        boundModel = roomToModel[info.roomNum];
                    }
                    else
                    {
                        boundModel = std::make_shared<Model>();
                        boundModel->name = "ROOM_";
                        boundModel->name.append(std::to_string(info.roomNum));

                        roomToModel.emplace(std::make_pair(info.roomNum, boundModel));
                    }

                    glm::vec3 largest{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
                    glm::vec3 smallest{ FLT_MAX, FLT_MAX, FLT_MAX };

                    for (size_t i = 0; i < mesh.X.size(); i++)
                    {
                        largest.x = mesh.X[i] > largest.x ? mesh.X[i] : largest.x;
                        largest.y = mesh.Y[i] > largest.y ? mesh.Y[i] : largest.y;
                        largest.z = mesh.Z[i] > largest.z ? mesh.Z[i] : largest.z;

                        smallest.x = mesh.X[i] < smallest.x ? mesh.X[i] : smallest.x;
                        smallest.y = mesh.Y[i] < smallest.y ? mesh.Y[i] : smallest.y;
                        smallest.z = mesh.Z[i] < smallest.z ? mesh.Z[i] : smallest.z;
                    }

                    if (mesh.parent.size() > 0)
                    {
                        size_t tI{};
                        bool found{};
                        for (tI = 0; tI < outZone.Transform.size(); tI++)
                        {
                            if (outZone.Transform[tI].name == mesh.parent)
                            {
                                found = true;
                                break;
                            }
                        }
                        if (found)
                        {
                            const glm::mat4 trans = transforms[tI].AsMatrix();
                            boundModel->bounds.min = adjustment.AsMatrix() * trans * glm::vec4(smallest, 1.0f);
                            boundModel->bounds.max = adjustment.AsMatrix() * trans * glm::vec4(largest, 1.0f);
                        }
                    }

                    // Irrelevant bounding boxes for the rooms
                    continue;
                }

                if (mesh.V1.size() < 1 || mesh.Face.size() < 1)
                {
                    // Would mean this mesh has no geometry
                    continue;
                }

                std::shared_ptr<Model> outModel{};
                if (settings.optimize && foundRoomInfo)
                {
                    if (roomToModel.find(info.roomNum) != roomToModel.end())
                    {
                        outModel = roomToModel[info.roomNum];

                        // With multiple zones we don't want to fill out twice
                        if (filledObjects[info.roomNum].contains(info.objNum))
                        {
                            continue;
                        }
                        else
                        {
                            filledObjects[info.roomNum].emplace(info.objNum);
                        }
                    }
                    else
                    {
                        roomToModel.emplace(std::make_pair(info.roomNum, std::make_shared<Model>()));
                        roomToModel[info.roomNum]->name = "ROOM_";
                        roomToModel[info.roomNum]->name.append(std::to_string(info.roomNum));

                        outModel = roomToModel[info.roomNum];

                        filledObjects.emplace(std::make_pair(info.roomNum, std::unordered_set<int>{}));
                    }
                }
                else
                {
                    if (result.geometry.size() < 1)
                    {
                        result.geometry.emplace_back(std::make_shared<Model>());
                    }
                    outModel = result.geometry[0];
                }

                auto& meshIn = outModel->meshes.emplace_back();
                meshIn.name = mesh.name;
                meshIn.isDoubleSided = mesh.doublesided;

                for (int i = 0; i < mesh.X.size(); i++)
                {
                    glm::vec3 pos{ mesh.X[i], mesh.Y[i], mesh.Z[i] };
                    glm::vec3 nor{};
                    if (mesh.Xn.size() == mesh.X.size())
                    {
                        nor = { mesh.Xn[i], mesh.Yn[i], mesh.Zn[i] };
                    }
                    glm::vec4 col{};
                    if (mesh.R.size() == mesh.X.size())
                    {
                        col = { mesh.R[i], mesh.G[i], mesh.B[i], mesh.A[i] };
                    }
                    glm::vec2 uv{};
                    if (mesh.U1.size() == mesh.X.size())
                    {
                        uv = { mesh.U1[i], 1.0f - mesh.V1[i] };
                    }

                    meshIn.vertices.emplace_back(pos, nor, col, uv);
                }

                // Transform vertices
                if (mesh.parent.size() > 0)
                {
                    size_t transformIndex{};
                    bool foundTransform{};
                    for (transformIndex = 0; transformIndex < outZone.Transform.size(); transformIndex++)
                    {
                        if (outZone.Transform[transformIndex].name == mesh.parent)
                        {
                            foundTransform = true;
                            break;
                        }
                    }
                    if (foundTransform)
                    {
                        const glm::mat4 trans = transforms[transformIndex].AsMatrix();

                        for (auto& v : meshIn.vertices)
                        {
                            v.position = adjustment.AsMatrix() * trans * glm::vec4(v.position, 1.0f);
                            v.normal = adjustmentRotationOnly.AsMatrix() * glm::vec4(v.normal, 1.0f); // May need to do rotation of 'trans'
                        }
                    }
                }

                for (int i = 0; i < mesh.Face.size(); i++)
                {
                    meshIn.indices.emplace_back(mesh.Face[i].v1);
                    meshIn.indices.emplace_back(mesh.Face[i].v2);
                    meshIn.indices.emplace_back(mesh.Face[i].v3);
                }

                if (mesh.material_name.size() > 0)
                {
                    auto res = std::find_if(materialsIn.begin(), materialsIn.end(), [&](std::shared_ptr<::TombForge::Material>& m) { return m->name == mesh.material_name; });
                    if (res != materialsIn.end())
                    {
                        meshIn.material = *res;
                    }
                }

                CalculateBoundingBox(meshIn);
            }

            for (auto& pair : roomToModel)
            {
                result.geometry.emplace_back(pair.second);
            }

            for (auto& model : result.geometry)
            {
                CalculateBoundingBox(*model);
            }

            if (settings.importCollision)
            {
                transforms.clear();

                // Fill out transforms
                for (size_t t = 0; t < outCln.Transform.size(); t++)
                {
                    auto& trans = transforms.emplace_back();
                    trans.position.x = outCln.Transform[t].tX;
                    trans.position.y = outCln.Transform[t].tY;
                    trans.position.z = outCln.Transform[t].tZ;

                    trans.scale.x = outCln.Transform[t].sX;
                    trans.scale.y = outCln.Transform[t].sY;
                    trans.scale.z = outCln.Transform[t].sZ;

                    trans.rotation = glm::quat({ glm::radians(outCln.Transform[t].rX), glm::radians(outCln.Transform[t].rY), glm::radians(outCln.Transform[t].rZ) });

                    if (outCln.Transform[t].parent.size() > 0)
                    {
                        bool found = false;
                        size_t pI{};
                        for (pI = 0; pI < outCln.Transform.size(); pI++)
                        {
                            if (outCln.Transform[pI].name == outCln.Transform[t].parent)
                            {
                                found = true;
                                break;
                            }
                        }
                        if (found && pI < t)
                        {
                            trans = transforms[pI] * trans;
                        }
                        else
                        {
                            LOG_WARNING("Did not find a parent transform for %s", outCln.Transform[t].name.c_str());
                        }
                    }
                }

                for (size_t c = 0; c < outCln.Mesh.size(); c++)
                {
                    glm::mat4 trans(1.0f);
                    size_t tI{};
                    bool foundTransform{};
                    if (outCln.Mesh[c].parent.size() > 0)
                    {
                        
                        
                        for (tI = 0; tI < outCln.Transform.size(); tI++)
                        {
                            if (outCln.Transform[tI].name == outCln.Mesh[c].parent)
                            {
                                foundTransform = true;
                                break;
                            }
                        }
                        if (foundTransform)
                        {
                            trans = transforms[tI].AsMatrix();
                        }
                    }

                    const bool isBox = outCln.Mesh[c].name.find("box") != std::string::npos;

                    if (isBox)
                    {
                        float largestX{};
                        float largestY{};
                        float largestZ{};
                        for (size_t c2 = 1; c2 < outCln.Mesh[c].X.size(); c2++)
                        {
                            const float xDiff = Maths::Abs(outCln.Mesh[c].X[c2] - outCln.Mesh[c].X[0]);
                            const float yDiff = Maths::Abs(outCln.Mesh[c].Y[c2] - outCln.Mesh[c].Y[0]);
                            const float zDiff = Maths::Abs(outCln.Mesh[c].Z[c2] - outCln.Mesh[c].Z[0]);
                            if (xDiff > largestX)
                            {
                                largestX = xDiff;
                            }
                            if (yDiff > largestY)
                            {
                                largestY = yDiff;
                            }
                            if (zDiff > largestZ)
                            {
                                largestZ = zDiff;
                            }
                        }

                        glm::vec3 extents{ largestX, largestZ, largestY };
                        extents *= settings.scale / 2.0f;

                        Transform position = adjustment * (foundTransform ? transforms[tI] : Transform{});

                        BoxCollider& box = result.boxColliders.emplace_back();
                        box.halfExtents = extents;
                        box.transform = position;
                    }
                    else if (outCln.Mesh[c].name.find("CLN_TRIANGLES") != std::string::npos)
                    {
                        MeshCollider& mesh = result.meshColliders.emplace_back();
                        for (size_t c2 = 0; c2 < outCln.Mesh[c].X.size(); c2++)
                        {
                            
                            glm::vec4 colPos(outCln.Mesh[c].X[c2], outCln.Mesh[c].Y[c2], outCln.Mesh[c].Z[c2], 1.0f);
                            //glm::vec3 colPos(outCln.Mesh[c].X[c2], outCln.Mesh[c].Y[c2], outCln.Mesh[c].Z[c2]);
                            //colPos *= settings.scale;
                            mesh.vertices.emplace_back(adjustment.AsMatrix() * colPos);
                            //mesh.vertices.emplace_back(adjustment.AsMatrix() * trans * colPos);
                        }

                        for (auto& face : outCln.Mesh[c].Face)
                        {
                            if (face.TrisOrQuads != 3)
                            {
                                mesh.indices.emplace_back(face.v1);
                                mesh.indices.emplace_back(face.v2);
                                mesh.indices.emplace_back(face.v3);
                                mesh.indices.emplace_back(face.v3);
                                mesh.indices.emplace_back(face.v4);
                                mesh.indices.emplace_back(face.v1);
                                LOG_WARNING("Importing quads");
                            }
                            else
                            {
                                mesh.indices.emplace_back(face.v1);
                                mesh.indices.emplace_back(face.v2);
                                mesh.indices.emplace_back(face.v3);
                            }
                        }
                    }
                }
            }

            LOG("Imported %s", filePath.c_str());

            return result;
        }
        else
        {
            LOG_ERROR("Error importing %s", filePath.c_str());
        }

        return {};
    }
}
