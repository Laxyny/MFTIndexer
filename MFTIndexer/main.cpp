#include "MFTIndexer.h"
#include <iostream>
#include <string>
#include <cwchar>
#include <algorithm>

void PrintUsage() {
    std::wcout << L"Usage: MFTIndexer [-d DRIVE] [-o OUTPUT] [--silent]\n"
               << L"  -d, --drive   Drive letter (e.g. C)\n"
               << L"  -o, --output  Output JSON file\n"
               << L"  -s, --silent  Suppress output\n";
}

int wmain(int argc, wchar_t* argv[])
{
    std::wstring drive = L"C";
    std::wstring output = L"output.json";
    bool silent = false;

    for (int i = 1; i < argc; ++i) {
        std::wstring arg = argv[i];
        if (arg == L"-d" || arg == L"--drive") {
            if (i + 1 < argc) {
                drive = argv[++i];
            } else {
                std::wcerr << L"Missing value for --drive\n";
                return 1;
            }
        } else if (arg == L"-o" || arg == L"--output") {
            if (i + 1 < argc) {
                output = argv[++i];
            } else {
                std::wcerr << L"Missing value for --output\n";
                return 1;
            }
        } else if (arg == L"-s" || arg == L"--silent") {
            silent = true;
        } else if (arg == L"-h" || arg == L"--help") {
            PrintUsage();
            return 0;
        }
    }

    // Normalize drive
    // Remove : \ /
    drive.erase(std::remove(drive.begin(), drive.end(), L':'), drive.end());
    drive.erase(std::remove(drive.begin(), drive.end(), L'\\'), drive.end());
    drive.erase(std::remove(drive.begin(), drive.end(), L'/'), drive.end());

    if (drive.empty()) {
        if (!silent) std::wcerr << L"Invalid drive letter\n";
        return 1;
    }

    std::wstring volumePath = L"\\\\.\\" + drive + L":";

    if (!silent) {
        std::wcout << L"Indexing " << volumePath << L" to " << output << L"..." << std::endl;
    }

    if (!ExportMFTToJson(volumePath.c_str(), output.c_str())) {
        if (!silent) std::wcerr << L"Failed to export MFT. Ensure you are running as Admin." << std::endl;
        return 1;
    }

    if (!silent) {
        std::wcout << L"Exported successfully." << std::endl;
    }
    return 0;
}
