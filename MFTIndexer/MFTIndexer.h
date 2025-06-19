#ifndef MFTINDEXER_H
#define MFTINDEXER_H

#ifdef _WIN32
#  include <windows.h>
#  ifdef MFTINDEXER_EXPORTS
#    define MFTINDEXER_API __declspec(dllexport)
#  else
#    define MFTINDEXER_API __declspec(dllimport)
#  endif
#else
#  define MFTINDEXER_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

MFTINDEXER_API bool ExportMFTToJson(const wchar_t* volumePath, const wchar_t* outputPath);
typedef void(__stdcall *MFTPathCallback)(const wchar_t* path);
MFTINDEXER_API bool ExportMFTToMemory(const wchar_t* volumePath, MFTPathCallback callback);

#ifdef __cplusplus
}
#endif

#endif // MFTINDEXER_H
