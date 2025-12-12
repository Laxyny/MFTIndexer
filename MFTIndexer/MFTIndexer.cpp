#include <windows.h>
#include <winioctl.h>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

#define BUFFER_SIZE (1024 * 1024)

// Définition locale stricte de la structure V0 (24 octets)
// On utilise un nom unique pour éviter les conflits "redefinition" avec winioctl.h
typedef struct {
    DWORDLONG StartFileReferenceNumber;
    USN       LowUsn;
    USN       HighUsn;
} MFT_ENUM_DATA_FIXED;

struct MFTEntry {
    ULONGLONG ParentFRN = 0;
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

static void LogError(const std::wstring& msg, DWORD err)
{
    std::cerr << "[Error] ";
    std::wcerr << msg << L" (error=" << err << L")" << std::endl;
}

// Fonction pour activer les privilèges Admin/Backup
static bool EnablePrivilege(LPCWSTR name)
{
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        return false;

    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!LookupPrivilegeValueW(NULL, name, &tp.Privileges[0].Luid))
    {
        CloseHandle(hToken);
        return false;
    }

    AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);
    CloseHandle(hToken);
    return (GetLastError() == ERROR_SUCCESS);
}

static bool GetUsnJournalData(HANDLE hVol, USN_JOURNAL_DATA& journal)
{
    DWORD bytes = 0;
    // Essai 1 : Lecture directe
    if (DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, nullptr, 0,
                        &journal, sizeof(journal), &bytes, nullptr))
    {
        return true;
    }
    
    // Essai 2 : Tentative de création (si inexistant)
    CREATE_USN_JOURNAL_DATA createData{};
    createData.MaximumSize = 0; 
    createData.AllocationDelta = 0; 

    if (!DeviceIoControl(hVol, FSCTL_CREATE_USN_JOURNAL,
                         &createData, sizeof(createData),
                         nullptr, 0, &bytes, nullptr))
    {
        // Essai 3 : Re-lecture après échec création (parfois le handle a juste besoin d'être rafraichi ou erreur ignorée)
        if (DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, nullptr, 0,
            &journal, sizeof(journal), &bytes, nullptr))
        {
            return true;
        }

        LogError(L"Could not query or create USN Journal. Ensure Admin rights.", GetLastError());
        return false;
    }
    
    return DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, nullptr, 0,
        &journal, sizeof(journal), &bytes, nullptr);
}

static std::wstring BuildParentPathIterative(
    ULONGLONG startFrn, 
    const std::unordered_map<ULONGLONG, MFTEntry>& entries,
    std::unordered_map<ULONGLONG, std::wstring>& cache
) {
    auto cacheIt = cache.find(startFrn);
    if (cacheIt != cache.end()) {
        return cacheIt->second;
    }

    std::vector<ULONGLONG> chain;
    ULONGLONG current = startFrn;
    
    // Remontée vers la racine
    while (true) {
        auto cIt = cache.find(current);
        if (cIt != cache.end()) {
            break;
        }
        
        auto eIt = entries.find(current);
        if (eIt == entries.end()) {
            break;
        }
        
        if (eIt->second.ParentFRN == current) {
             break;
        }

        chain.push_back(current);
        current = eIt->second.ParentFRN;
    }

    std::wstring pathBase;
    auto baseIt = cache.find(current);
    if (baseIt != cache.end()) {
        pathBase = baseIt->second;
    }

    // Reconstruction du chemin
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        ULONGLONG frn = *it;
        const auto& entry = entries.at(frn);
        pathBase += L"\\" + entry.Name;
        cache[frn] = pathBase;
    }

    return pathBase;
}

static bool EnumeratePaths(const wchar_t* volumePath, std::vector<std::wstring>& paths) {
    // Activer les privilèges pour éviter les accès refusés
    EnablePrivilege(SE_BACKUP_NAME);
    EnablePrivilege(SE_MANAGE_VOLUME_NAME);
    EnablePrivilege(SE_DEBUG_NAME);

    std::wstring driveLetter;
    if (wcsncmp(volumePath, L"\\\\.\\", 4) == 0 && wcslen(volumePath) >= 5) {
        driveLetter.assign(1, volumePath[4]);
    } else {
        driveLetter.assign(1, volumePath[0]);
    }

    HANDLE hVol = CreateFileW(
        volumePath,
        GENERIC_READ | GENERIC_WRITE, 
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0, 
        NULL
    );

    if (hVol == INVALID_HANDLE_VALUE) {
        hVol = CreateFileW(
            volumePath,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );
        
        if (hVol == INVALID_HANDLE_VALUE) {
             LogError(L"CreateFile failed (Check Admin)", GetLastError());
             return false;
        }
    }

    USN_JOURNAL_DATA journalData = { 0 };
    if (!GetUsnJournalData(hVol, journalData))
    {
        CloseHandle(hVol);
        return false;
    }

    BYTE* buffer = new BYTE[BUFFER_SIZE];
    if (!buffer) {
        CloseHandle(hVol);
        return false;
    }
    ZeroMemory(buffer, BUFFER_SIZE);
    DWORD bytesReturned = 0;
    bool gotData = false;

    // Utilisation de NOTRE structure fixée à 24 octets
    MFT_ENUM_DATA_FIXED mftEnumData = { 0 };
    mftEnumData.StartFileReferenceNumber = 0;
    mftEnumData.LowUsn = 0;
    mftEnumData.HighUsn = journalData.NextUsn; 

    std::unordered_map<ULONGLONG, MFTEntry> entries;
    entries.reserve(200000); 

    while (true)
    {
        if (!DeviceIoControl(
            hVol,
            FSCTL_ENUM_USN_DATA,
            &mftEnumData,
            sizeof(mftEnumData), // Taille = 24 octets, ce que veut NTFS
            buffer,
            BUFFER_SIZE,
            &bytesReturned,
            NULL))
        {
            DWORD err = GetLastError();
            if (err == ERROR_HANDLE_EOF)
                break;
                
            LogError(L"FSCTL_ENUM_USN_DATA failed", err);
            // Debug info pour comprendre si ça plante encore
            std::cerr << "  StartFRN: " << mftEnumData.StartFileReferenceNumber << "\n";
            std::cerr << "  LowUsn: " << mftEnumData.LowUsn << "\n";
            std::cerr << "  HighUsn: " << mftEnumData.HighUsn << "\n";
            std::cerr << "  Size: " << sizeof(mftEnumData) << "\n";
            
            delete[] buffer;
            CloseHandle(hVol);
            return false;
        }
        gotData = true;
        BYTE* ptr = buffer;
        
        // Le premier élément retourné est le FRN pour le prochain appel
        mftEnumData.StartFileReferenceNumber = *((DWORDLONG*)ptr);
        ptr = buffer + sizeof(DWORDLONG); 

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
    }

    CloseHandle(hVol);
    delete[] buffer;

    if (!gotData)
    {
        return false;
    }

    // Construction des chemins complets
    std::unordered_map<ULONGLONG, std::wstring> pathCache;
    pathCache.reserve(entries.size() / 10); 
    paths.reserve(entries.size());

    for (const auto& pair : entries)
    {
        const auto& entry = pair.second;
        std::wstring parentPath = BuildParentPathIterative(entry.ParentFRN, entries, pathCache);
        std::wstring fullPath = driveLetter + L":" + parentPath + L"\\" + entry.Name;
        paths.push_back(std::move(fullPath));
    }
    
    std::sort(paths.begin(), paths.end());

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
    for (size_t i = 0; i < paths.size(); ++i)
    {
        outFile << L"  \"" << EscapeJsonString(paths[i]) << L"\"";
        if (i < paths.size() - 1) {
            outFile << L",\n";
        } else {
            outFile << L"\n";
        }
    }
    outFile << L"]";

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
