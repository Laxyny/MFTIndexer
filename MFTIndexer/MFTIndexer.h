#pragma once
#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport)
bool ExportMFTToJson(const wchar_t* volumePath, const wchar_t* outputPath);

#ifdef __cplusplus
}
#endif
