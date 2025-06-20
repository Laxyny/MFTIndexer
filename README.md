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

### As an executable

```bash
MFTIndexer.exe C output.json
```

Run the tool from an elevated command prompt. The first argument is the target
volume letter and the second is the output path. Using the raw volume path
avoids the common `FSCTL_ENUM_USN_DATA failed (error=87)` message caused by an
invalid handle or insufficient privileges.

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
