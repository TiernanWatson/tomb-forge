#pragma once

#if DEVSLATE

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <shobjidl.h>
#include <string>
#include <vector>

namespace TombForge
{
    std::string OpenFileDialog(const COMDLG_FILTERSPEC* fileTypes, UINT numFileTypes, bool pickFolders = false);

    bool OpenMultiFileDialog(const COMDLG_FILTERSPEC* fileTypes, UINT numFileTypes, std::vector<std::string>& outPaths);

    std::string SaveFileDialog(const COMDLG_FILTERSPEC* fileTypes, UINT numFileTypes);
}

#endif
