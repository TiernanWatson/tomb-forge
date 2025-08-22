#pragma once

#include <string>

namespace TombForge
{
	/// <summary>
	/// Used to store details about a project for use in the editor
	/// </summary>
	struct ProjectSettings
	{
		std::string name{ "Untitled" };
		std::string laraPath{};

		void LoadJson(const std::string& path);
		void SaveJson(const std::string& path) const;
	};
}
