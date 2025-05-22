#include "LevelExport.h"

#include "Classes.h"
#include "MA/MA_Classes.h"

#include "Misc_Functions.h"
#include "TRAOD/CAM/CAM_Functions.h"
#include "TRAOD/ZONE/ZONE_Functions.h"
#include "TRAOD/GMX/GMX_Functions.h"
#include "TRAOD/RMX/RMX_Functions.h"
#include "TRAOD/CLN/CLN_Functions.h"
#include "TRAOD/CLZ/CLZ_Functions.h"

extern AOD_IO_CLASS AOD_IO;

ExportError ExportLevel(string fileName, MA_EXPORT& outRmx, MA_EXPORT& outZone, MA_EXPORT& outCam, MA_EXPORT& outCln)
{
	// Using const_cast only to get this working using existing API - this is bad
	//char* argv[1]{ const_cast<char*>(fileName.c_str()) };
	if (!AOD_IO_Init(fileName))
	{
		return ExportError::InitFail;
	}

	unsigned int sz = AOD_IO.file_clzgmx.size();
	if (AOD_IO.file_clzgmx[sz - 3] == 'C' && AOD_IO.file_clzgmx[sz - 2] == 'L' && AOD_IO.file_clzgmx[sz - 1] == 'Z')
	{
		msg(msg::TGT::FILE_CONS, msg::TYP::LOG) << "CLZ input file detected. Decompressing";
		if (!Decompress_CLZ())
			return ExportError::ClzDecompressFail;
	}

	if (!Export_GMX())
	{
		return ExportError::GmxExportFail;
	}

	SetCurrentDirectory(AOD_IO.folder_level_lpwstr);
	if (AOD_IO.SearchFileInGMXList("", AoDFileType::CAM) || AOD_IO.SearchFileInGMXList("", AoDFileType::CBH))
		CreateDirectory(AOD_IO.folder_cameras_lpwstr, NULL);		// Crea la cartella \NOMELIVELLO\Cameras
	if (AOD_IO.SearchFileInGMXList("", AoDFileType::RMX))
		CreateDirectory(AOD_IO.folder_rooms_lpwstr, NULL);			// Crea la cartella \NOMELIVELLO\Rooms
	if (AOD_IO.SearchFileInGMXList("", AoDFileType::ZONE))
		CreateDirectory(AOD_IO.folder_zones_lpwstr, NULL);			// Crea la cartella \NOMELIVELLO\Zones
	if (AOD_IO.SearchFileInGMXList("", AoDFileType::CHR))
		CreateDirectory(AOD_IO.folder_characters_lpwstr, NULL);		// Crea la cartella \NOMELIVELLO\Characters
	if (AOD_IO.SearchFileInGMXList("", AoDFileType::TMT))
		CreateDirectory(AOD_IO.folder_blendshapes_lpwstr, NULL);	// Crea la cartella \NOMELIVELLO\Blendshapes
	if (AOD_IO.SearchFileInGMXList("", AoDFileType::CAL) || AOD_IO.SearchFileInGMXList("", AoDFileType::POS) ||
		AOD_IO.SearchFileInGMXList("", AoDFileType::XXX) || AOD_IO.SearchFileInGMXList("", AoDFileType::TMS))
		CreateDirectory(AOD_IO.folder_animations_lpwstr, NULL);		// Crea la cartella \NOMELIVELLO\Animations
	if (AOD_IO.SearchFileInGMXList("", AoDFileType::CLN))
		CreateDirectory(AOD_IO.folder_collisions_lpwstr, NULL);		// Crea la cartella \NOMELIVELLO\Collisions

	for (unsigned int i = 0; i < AOD_IO.gmxfiles.size(); i++)
	{
		switch (AOD_IO.gmxfiles[i].type)
		{
		case(AoDFileType::RMX):
			if (!Export_RMX(AOD_IO.gmxfiles[i].name, outRmx))
				msg(msg::TGT::FILE_CONS, msg::TYP::ERR) << "RMX exporting completed with error(s)";
			msg(msg::TGT::FILE_CONS, msg::TYP::LOG) << "";
			break;
		case(AoDFileType::ZONE):
			if (!Export_ZONE(AOD_IO.gmxfiles[i].name, outZone))
				msg(msg::TGT::FILE_CONS, msg::TYP::ERR) << "ZONE exporting completed with error(s)";
			msg(msg::TGT::FILE_CONS, msg::TYP::LOG) << "";
			break;
		case(AoDFileType::CAM):
			if (!Export_CAM(AOD_IO.gmxfiles[i].name, outCam))
				msg(msg::TGT::FILE_CONS, msg::TYP::ERR) << "CAM exporting completed with error(s)";
			msg(msg::TGT::FILE_CONS, msg::TYP::LOG) << "";
			break;
		case(AoDFileType::CLN):
			if (!Export_CLN(AOD_IO.gmxfiles[i].name, outCln))
				msg(msg::TGT::FILE_CONS, msg::TYP::ERR) << "CLN exporting completed with error(s)";
			msg(msg::TGT::FILE_CONS, msg::TYP::LOG) << "";
			break;
		}
	}

	return ExportError::None;
}
