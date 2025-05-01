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

std::wstring BuildFullPath(ULONGLONG frn, const std::map<ULONGLONG, MFTEntry>& entries) {
    std::wstring path;
    auto it = entries.find(frn);
    while (it != entries.end()) {
        if (it->second.Name.empty()) break;
        path = L"\\" + it->second.Name + path;
        it = entries.find(it->second.ParentFRN);
    }
    return path;
}

extern "C" __declspec(dllexport)
bool ExportMFTToJson(const wchar_t* volumePath, const wchar_t* outputPath) {
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

    std::wofstream outFile(outputPath);
    if (!outFile.is_open()) {
        delete[] buffer;
        return false;
    }

    outFile << L"[\n";
    bool first = true;
    for (const auto& pair : entries)
    {
        auto frn = pair.first;
        const auto& entry = pair.second;

        std::wstring fullPath = BuildFullPath(frn, entries);
        if (fullPath.empty()) continue;

        if (!first) outFile << L",\n";
        first = false;

        outFile << L"  \"" << EscapeJsonString(fullPath) << L"\"";
    }
    outFile << L"\n]";

    outFile.close();
    delete[] buffer;
    return true;
}