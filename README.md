# MFTIndexer

> MFTIndexer is a native NTFS file system indexing engine built in C++.  
> It directly parses the Master File Table (MFT) using low-level Windows APIs, enabling high-performance file enumeration across local volumes.

![GitHub last commit](https://img.shields.io/github/last-commit/laxyny/MFTIndexer?style=for-the-badge)
![GitHub issues](https://img.shields.io/github/issues/laxyny/MFTIndexer?style=for-the-badge)
![GitHub pull requests](https://img.shields.io/github/issues-pr/laxyny/MFTIndexer?style=for-the-badge)
![GitHub license](https://img.shields.io/github/license/laxyny/MFTIndexer?style=for-the-badge)

---

## About

**MFTIndexer** provides ultra-fast file listing capabilities by reading directly from NTFS volumes via the USN journal and `FSCTL_ENUM_USN_DATA`.  
Unlike traditional indexing, it avoids recursive directory scanning, resulting in near-instant file enumeration with minimal resource consumption.

---

## Features

- Direct raw access to NTFS Master File Table (MFT)
- Efficient enumeration of millions of files in seconds
- Outputs to `.json` or custom formats for integration
- Suitable for launchers, system tools, or indexing services
- Can be used as an executable or as a native DLL

---

## Usage

### Building

This project targets Windows and can be compiled with Visual Studio or with
[CMake](https://cmake.org/) using a MinGW toolchain.

```bash
# Example using mingw-w64 on Linux
cmake -B build -DCMAKE_TOOLCHAIN_FILE=<path-to-mingw-toolchain.cmake>
cmake --build build --config Release
```

The resulting `MFTIndexer.dll` library and `MFTIndexer.exe` CLI are produced in
the build directory.

### As an executable

```bash
MFTIndexer.exe --export output.json
```

Exports a full list of file paths to `output.json`.

### As a DLL

```cpp
// Exported from the DLL
void ExportToJson(const wchar_t* outputPath);
```

> Ideal for integration with C++, C#, Rust or any language supporting native interop.

---

## Integration Example (C#)

```csharp
[DllImport("MFTIndexer.dll", CharSet = CharSet.Unicode)]
public static extern void ExportToJson(string outputPath);
```

---

## Requirements

- Windows (NTFS volumes only)
- Admin privileges required to access volume data
- Built for x64 architecture

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for full terms.

---

## Credits

Created and maintained by [Kevin Gregoire](https://github.com/laxyny)  
Inspired by the internal design of `Everything.exe`, but fully independent and written from scratch.
