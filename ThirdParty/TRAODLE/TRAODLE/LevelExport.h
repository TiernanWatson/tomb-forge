#pragma once

#include "MA_Export.h"
#include <string>

enum class ExportError
{
	None = 0,
	InitFail,
	ClzDecompressFail,
	GmxExportFail,
	RmxExportFail,
	ZoneExportFail,
	CamExportFail,
	ClnExportFail
};

ExportError ExportLevel(std::string fileName, MA_EXPORT& outRmx, MA_EXPORT& outZone, MA_EXPORT& outCam, MA_EXPORT& outCln);
