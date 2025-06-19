#include <windows.h>
#include <winioctl.h>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <sstream>

#define BUFFER_SIZE (1024 * 1024)

struct MFTEntry {
    ULONGLONG ParentFRN;
    std::wstring Name;
};

std::wstring EscapeJsonString(const std::wstring& input) {
    std::wstringstream ss;
    for (auto c : input) {
        switch (c) {
        case L'"': ss << L"\\\""; break;
        case L'\\': ss << L"\\\\"; break;
        default: ss << c; break;
        }
    }
    return ss.str();
}

static std::wstring BuildFullPath(ULONGLONG frn, const std::map<ULONGLONG, MFTEntry>& entries) {
    std::wstring path;
    auto it = entries.find(frn);
    while (it != entries.end()) {
        if (it->second.Name.empty()) break;
        path = L"\\" + it->second.Name + path;
        it = entries.find(it->second.ParentFRN);
    }
    return path;
}

static bool EnumeratePaths(const wchar_t* volumePath, std::vector<std::wstring>& paths) {
    std::wstring driveLetter;
    if (wcsncmp(volumePath, L"\\\\.\\", 4) == 0 && wcslen(volumePath) >= 5) {
        driveLetter.assign(1, volumePath[4]);
    } else {
        driveLetter.assign(1, volumePath[0]);
    }

    HANDLE hVol = CreateFileW(
        volumePath,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hVol == INVALID_HANDLE_VALUE) {
        return false;
    }

    BYTE* buffer = new BYTE[BUFFER_SIZE];
    ZeroMemory(buffer, BUFFER_SIZE);
    DWORD bytesReturned = 0;

    MFT_ENUM_DATA mftEnumData = { 0 };
    mftEnumData.StartFileReferenceNumber = 0;
    mftEnumData.LowUsn = 0;
    mftEnumData.HighUsn = MAXLONGLONG;

    std::map<ULONGLONG, MFTEntry> entries;

    while (DeviceIoControl(
        hVol,
        FSCTL_ENUM_USN_DATA,
        &mftEnumData,
        sizeof(mftEnumData),
        buffer,
        BUFFER_SIZE,
        &bytesReturned,
        NULL))
    {
        BYTE* ptr = buffer;
        ptr += sizeof(DWORD); // version
        ptr += sizeof(USN);   // usn

        while ((ptr - buffer) < bytesReturned) {
            USN_RECORD* record = (USN_RECORD*)ptr;

            std::wstring name(record->FileName, record->FileNameLength / sizeof(WCHAR));
            if (!name.empty() && name[0] != L'$') {
                entries[record->FileReferenceNumber] = {
                    record->ParentFileReferenceNumber,
                    name
                };
            }

            ptr += record->RecordLength;
        }

        mftEnumData.StartFileReferenceNumber = *(USN*)buffer;
    }

    CloseHandle(hVol);

    for (const auto& pair : entries)
    {
        auto frn = pair.first;
        std::wstring fullPath = BuildFullPath(frn, entries);
        if (fullPath.empty())
            continue;
        std::wstring finalPath = driveLetter + L":" + fullPath;
        paths.push_back(std::move(finalPath));
    }

    delete[] buffer;
    return true;
}

extern "C" __declspec(dllexport)
bool ExportMFTToJson(const wchar_t* volumePath, const wchar_t* outputPath) {
    std::vector<std::wstring> paths;
    if (!EnumeratePaths(volumePath, paths))
        return false;

    std::wofstream outFile(outputPath);
    if (!outFile.is_open()) {
        return false;
    }

    outFile << L"[\n";
    bool first = true;
    for (const auto& fullPath : paths)
    {
        if (!first) outFile << L",\n";
        first = false;

        outFile << L"  \"" << EscapeJsonString(fullPath) << L"\"";
    }
    outFile << L"\n]";

    outFile.close();
    return true;
}

extern "C" __declspec(dllexport)
bool ExportMFTToMemory(const wchar_t* volumePath, void(__stdcall* callback)(const wchar_t*)) {
    std::vector<std::wstring> paths;
    if (!EnumeratePaths(volumePath, paths))
        return false;
    if (!callback) return true;
    for (const auto& p : paths)
    {
        callback(p.c_str());
    }
    return true;
}