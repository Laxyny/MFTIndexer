#include <windows.h>
#include <winioctl.h>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <iomanip>

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

static void LogError(const std::wstring& msg, DWORD err)
{
    std::wofstream log(L"debug_log.txt", std::ios::app);
    log << msg << L" (error=" << err << L")" << std::endl;
}

static bool EnsureUsnJournal(HANDLE hVol)
{
    DWORD bytes = 0;
    USN_JOURNAL_DATA journal{};
    if (DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, nullptr, 0,
                        &journal, sizeof(journal), &bytes, nullptr))
    {
        return true;
    }

    CREATE_USN_JOURNAL_DATA createData{};
    if (!DeviceIoControl(hVol, FSCTL_CREATE_USN_JOURNAL,
                         &createData, sizeof(createData),
                         nullptr, 0, &bytes, nullptr))
    {
        LogError(L"CREATE_USN_JOURNAL failed", GetLastError());
        return false;
    }
    return true;
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
        LogError(L"CreateFile failed", GetLastError());
        return false;
    }

    if (!EnsureUsnJournal(hVol))
    {
        CloseHandle(hVol);
        return false;
    }

    BYTE* buffer = new BYTE[BUFFER_SIZE];
    ZeroMemory(buffer, BUFFER_SIZE);
    DWORD bytesReturned = 0;
    bool gotData = false;

    MFT_ENUM_DATA mftEnumData = { 0 };
    mftEnumData.StartFileReferenceNumber = 0;
    mftEnumData.LowUsn = 0;
    mftEnumData.HighUsn = MAXLONGLONG;

    std::map<ULONGLONG, MFTEntry> entries;

    while (true)
    {
        if (!DeviceIoControl(
            hVol,
            FSCTL_ENUM_USN_DATA,
            &mftEnumData,
            sizeof(mftEnumData),
            buffer,
            BUFFER_SIZE,
            &bytesReturned,
            NULL))
        {
            DWORD err = GetLastError();
            if (err == ERROR_HANDLE_EOF)
                break;
            LogError(L"FSCTL_ENUM_USN_DATA failed", err);
            delete[] buffer;
            CloseHandle(hVol);
            return false;
        }
        gotData = true;
        BYTE* ptr = buffer;
        ptr += sizeof(DWORD); // version
        ptr += sizeof(USN);   // usn

        std::wofstream logStep(L"debug_log.txt", std::ios::app);
        logStep << L"DeviceIoControl OK  bytesReturned = " << bytesReturned << L"\n";

        while ((ptr - buffer) < bytesReturned) {
            USN_RECORD* record = (USN_RECORD*)ptr;

            std::wstring name(record->FileName, record->FileNameLength / sizeof(WCHAR));
            std::wofstream debugLog(L"debug_log.txt", std::ios::app);
            if (!name.empty() && name[0] != L'$') {
                entries[record->FileReferenceNumber] = {
                    record->ParentFileReferenceNumber,
                    name
                };
                debugLog << L"Name: " << name << L"\n";
            }
            else {
                debugLog << L"IGNORED: " << name << L"\n";
            }

            ptr += record->RecordLength;
        }

        mftEnumData.StartFileReferenceNumber = *(USN*)buffer;
    }

    CloseHandle(hVol);

    if (!gotData)
    {
        delete[] buffer;
        return false;
    }

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
    std::wofstream finalLog(L"debug_log.txt", std::ios::app);
    finalLog << L"Total fichiers indexés : " << paths.size() << L"\n";

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