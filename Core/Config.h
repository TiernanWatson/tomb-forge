#pragma once

#include <cstdint>
#include <string>

namespace TombForge
{
	struct Config
	{
		uint32_t resolutionX{ 1280 };
		uint32_t resolutionY{ 720 };

		uint32_t maxPhysicsBodies{ 1024 };
		uint32_t numPhysicsBodyMutexes{ 0 }; // 0 means use default
		uint32_t maxPhysicsBodyPairs{ 1024 };
		uint32_t maxPhysicsContactConstraints{ 1024 };

		std::string windowTitle{ "TombForge Editor" };
		std::string defaultLara{ "D:\\TestOutput\\LaraRoot\\LaraRoot.tombs" };
		std::string defaultLevel{ "" };

		bool isFullscreen{};

		static Config& Get();
	};
}

