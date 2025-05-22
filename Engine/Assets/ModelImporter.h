#pragma once

#include <string>
#include <memory>

#include "../../Core/Graphics/Model.h"
#include "../Animation/Skeleton.h"
#include "../Animation/Animation.h"

namespace TombForge
{
	struct ImporterImpl; // Defined in .cpp to hide away third party library from header

	struct AssetDescription
	{
		bool hasModel : 1 {};
		bool hasSkeleton : 1 {};
		bool hasAnimation : 1 {};
	};

	struct ImportSettings
	{
		std::string modelPath{};
		std::string skeletonPath{};
		std::string animationPath{};

		std::shared_ptr<Skeleton> existingSkeleton{}; // For importing anims

		float scale{ 1.0f }; // Scale the vertices by this
		float animationScale{ 1.0f }; // Scale animation time by this

		bool importModel : 1 {};
		bool importSkeleton : 1 {};
		bool importAnimation : 1 {};
		bool zIsUp : 1 {}; // If true, change vertices so that z values go in y
		bool isLeftHanded : 1 {}; // Swaps the handedness from left to right
	};

	struct ImportResult
	{
		std::shared_ptr<Model> model{};
		std::shared_ptr<Skeleton> skeleton{};
		std::shared_ptr<Animation> animation{};
	};

	class AssetImportSession
	{
	public:
		AssetImportSession();
		AssetImportSession(const std::string& path);
		AssetImportSession(const AssetImportSession&) = delete;
		AssetImportSession(AssetImportSession&&) = delete;
		~AssetImportSession();

		bool Start(const std::string& path);

		ImportResult Import(const ImportSettings& settings);

		void Finish();

		inline const AssetDescription& GetDetails() const
		{
			return m_contains;
		}

		inline const std::string& GetPath() const
		{
			return m_path;
		}

	private:
		bool CheckForModel() const;

		bool CheckForSkeleton() const;

		bool CheckForAnimation() const;

		ImporterImpl* m_importer{};
		std::string m_path{};

		AssetDescription m_contains{};
	};
}

