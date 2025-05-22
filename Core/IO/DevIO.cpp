#if DEVSLATE

#include "DevIO.h"

namespace TombForge
{
    std::string OpenFileDialog(const COMDLG_FILTERSPEC* fileTypes, UINT numFileTypes, bool pickFolders)
    {
        char result[512]{};

        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
            COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr))
        {
            IFileOpenDialog* pFileOpen;

            // Create the FileOpenDialog object.
            hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

            if (SUCCEEDED(hr))
            {
                pFileOpen->SetFileTypes(numFileTypes, fileTypes);
                
                if (pickFolders)
                {
                    FILEOPENDIALOGOPTIONS options{};
                    if (SUCCEEDED(pFileOpen->GetOptions(&options)))
                    {
                        pFileOpen->SetOptions(FOS_PICKFOLDERS | options);
                    }
                }

                // Show the Open dialog box.
                hr = pFileOpen->Show(NULL);

                // Get the file name from the dialog box.
                if (SUCCEEDED(hr))
                {
                    IShellItem* pItem;
                    hr = pFileOpen->GetResult(&pItem);
                    if (SUCCEEDED(hr))
                    {
                        PWSTR pszFilePath;
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                        // Display the file name to the user.
                        if (SUCCEEDED(hr))
                        {
                            size_t sizeOut{};
                            wcstombs_s(&sizeOut, result, 512, pszFilePath, wcslen(pszFilePath));
                            CoTaskMemFree(pszFilePath);
                        }
                        pItem->Release();
                    }
                }
                pFileOpen->Release();
            }
            CoUninitialize();
        }

        return result;
    }

    bool OpenMultiFileDialog(const COMDLG_FILTERSPEC* fileTypes, UINT numFileTypes, std::vector<std::string>& outPaths)
    {
        char result[512]{};

        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
            COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr))
        {
            IFileOpenDialog* pFileOpen;

            // Create the FileOpenDialog object.
            hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

            if (SUCCEEDED(hr))
            {
                pFileOpen->SetFileTypes(numFileTypes, fileTypes);
                FILEOPENDIALOGOPTIONS options{};
                if (SUCCEEDED(pFileOpen->GetOptions(&options)))
                {
                    pFileOpen->SetOptions(FOS_ALLOWMULTISELECT | options);
                }

                // Show the Open dialog box.
                hr = pFileOpen->Show(NULL);

                // Get the file name from the dialog box.
                if (SUCCEEDED(hr))
                {
                    IShellItemArray* pItems;
                    pFileOpen->GetResults(&pItems);
                    if (SUCCEEDED(hr))
                    {
                        DWORD numItems{};
                        hr = pItems->GetCount(&numItems);
                        if (SUCCEEDED(hr))
                        {
                            for (DWORD i = 0; i < numItems; i++)
                            {
                                IShellItem* pItem;
                                hr = pItems->GetItemAt(i, &pItem);
                                if (SUCCEEDED(hr))
                                {
                                    PWSTR pszFilePath;
                                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                                    if (SUCCEEDED(hr))
                                    {
                                        memset(result, 0, sizeof(char) * 512);
                                        size_t sizeOut{};
                                        wcstombs_s(&sizeOut, result, 512, pszFilePath, wcslen(pszFilePath));
                                        CoTaskMemFree(pszFilePath);

                                        outPaths.emplace_back(result);
                                    }
                                    pItem->Release();
                                }
                            }
                            pItems->Release();
                        }
                        
                    }
                }
                pFileOpen->Release();
            }
            CoUninitialize();
        }

        return result;
    }

    std::string SaveFileDialog(const COMDLG_FILTERSPEC* fileTypes, UINT numFileTypes)
    {
        char result[512]{};

        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
            COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr))
        {
            IFileSaveDialog* pFileSave;

            // Create the FileOpenDialog object.
            hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
                IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));

            if (SUCCEEDED(hr))
            {
                pFileSave->SetFileTypes(numFileTypes, fileTypes);
                FILEOPENDIALOGOPTIONS options{};
                if (SUCCEEDED(pFileSave->GetOptions(&options)))
                {
                    pFileSave->SetOptions(FOS_PICKFOLDERS | options);
                }

                // Show the Open dialog box.
                hr = pFileSave->Show(NULL);

                // Get the file name from the dialog box.
                if (SUCCEEDED(hr))
                {
                    IShellItem* pItem;
                    hr = pFileSave->GetResult(&pItem);
                    if (SUCCEEDED(hr))
                    {
                        PWSTR pszFilePath;
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                        // Display the file name to the user.
                        if (SUCCEEDED(hr))
                        {
                            size_t sizeOut{};
                            wcstombs_s(&sizeOut, result, 512, pszFilePath, wcslen(pszFilePath));
                            CoTaskMemFree(pszFilePath);
                        }
                        pItem->Release();
                    }
                }
                pFileSave->Release();
            }
            CoUninitialize();
        }

        return result;
    }
}

#endif
