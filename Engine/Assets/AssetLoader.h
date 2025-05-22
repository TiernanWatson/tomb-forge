#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "../../Core/Debug.h"

namespace TombForge
{
	template<typename AssetType>
	class AssetLoader
	{
	public:
		std::shared_ptr<AssetType> Load(const std::string& name);

		void AddExisting(const std::string& name, std::shared_ptr<AssetType> asset);

		void ClearUnreferenced();

	protected:
		virtual std::shared_ptr<AssetType> Read(const std::string& name) = 0;

	private:
		std::unordered_map<std::string, std::shared_ptr<AssetType>> m_cache{};
	};

	template<typename AssetType>
	std::shared_ptr<AssetType> AssetLoader<AssetType>::Load(const std::string& name)
	{
		if (m_cache.contains(name))
		{
			return m_cache[name];
		}

		auto asset = Read(name);

		if (asset)
		{
			m_cache.emplace(name, asset);
			return asset;
		}

		LOG_ERROR("Failed to load asset %s", name.c_str());
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

