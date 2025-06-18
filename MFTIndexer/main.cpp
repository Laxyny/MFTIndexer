#include "MFTIndexer.h"
#include <iostream>
#include <string>
#include <cwchar>

int wmain(int argc, wchar_t* argv[])
{
    const wchar_t* volume = L"\\\\.\\C:";
    const wchar_t* output = L"mft.json";

    for (int i = 1; i < argc; ++i) {
        if (wcscmp(argv[i], L"--volume") == 0 && i + 1 < argc) {
            volume = argv[++i];
        } else if (wcscmp(argv[i], L"--export") == 0 && i + 1 < argc) {
            output = argv[++i];
        }
    }

    if (!ExportMFTToJson(volume, output)) {
        std::wcerr << L"Failed to export MFT" << std::endl;
        return 1;
    }

    std::wcout << L"Exported to " << output << std::endl;
    return 0;
}
