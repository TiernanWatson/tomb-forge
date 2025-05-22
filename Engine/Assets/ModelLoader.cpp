#include "ModelLoader.h"

#include <fstream>

namespace TombForge
{
    std::shared_ptr<Model> ModelLoader::Read(const std::string& name)
    {
		std::ifstream inFile(name, std::ios::binary);

		if (inFile.is_open())
		{
			std::shared_ptr<Model> resource = std::make_shared<Model>();
			resource->name = name;

			size_t numMeshes{};
			inFile.read((char*)&numMeshes, sizeof(size_t));

			resource->meshes.resize(numMeshes);

			for (size_t i = 0; i < numMeshes; i++)
			{
				decltype(resource->meshes)::value_type& mesh = resource->meshes[i];

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
					size_t stringSize{};
					inFile.read((char*)&stringSize, sizeof(size_t));

					std::string materialName{};
					materialName.resize(stringSize + 1);

					inFile.read(materialName.data(), stringSize);

					if (m_materialLoader)
					{
						mesh.material = m_materialLoader->Load(materialName);
					}
				}
			}

			bool hasSkeleton{};
			inFile.read((char*)&hasSkeleton, sizeof(bool));

			if (hasSkeleton)
			{
				size_t stringSize{};
				inFile.read((char*)&stringSize, sizeof(size_t));

				resource->skeleton = std::make_shared<Skeleton>();
				resource->skeleton->name.resize(stringSize);

				inFile.read(resource->skeleton->name.data(), sizeof(char) * stringSize);

				resource->skeleton->Load();
			}

			return resource;
		}

		return nullptr;
    }
}
