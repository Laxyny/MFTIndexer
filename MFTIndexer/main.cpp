#include <windows.h>
#include <iostream>
#include "MFTIndexer.h"

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 3) {
        std::wcout << L"Usage: MFTIndexer.exe <volume (e.g. C)> <output.json>" << std::endl;
        return 1;
    }

    std::wstring volumeLetter = argv[1];
    if (volumeLetter.back() == L':')
        volumeLetter.pop_back();
    std::wstring volumePath = L"\\\\.\\" + volumeLetter + L":";

    const wchar_t* outputPath = argv[2];

    if (!ExportMFTToJson(volumePath.c_str(), outputPath)) {
        std::wcerr << L"Failed to export MFT. Ensure you run as Administrator and that the volume exists." << std::endl;
        return 1;
    }

    std::wcout << L"Export completed." << std::endl;
    return 0;
}
