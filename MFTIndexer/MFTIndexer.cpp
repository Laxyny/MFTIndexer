#include <windows.h>
#include <winioctl.h>
#include <tchar.h>
#include <fstream>
#include <vector>
#include <string>

#define BUFFER_SIZE (1024 * 1024)

std::wstring ConvertToJsonArray(const std::vector<std::wstring>& paths) {
    std::wstring json = L"[";
    for (size_t i = 0; i < paths.size(); ++i) {
        json += L"\"" + paths[i] + L"\"";
        if (i != paths.size() - 1) json += L",";
    }
    json += L"]";
    return json;
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

    std::vector<std::wstring> resultPaths;

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
            if (!name.empty() && name[0] != L'$')
                resultPaths.push_back(name);
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

    outFile << ConvertToJsonArray(resultPaths);
    outFile.close();

    delete[] buffer;
    return true;
}
