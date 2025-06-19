# AGENTS.md

## 🧠 Project context

This project is called **MFTIndexer**. It is a high-performance NTFS indexing engine written in **native C++** that extracts all file paths directly from the **Master File Table (MFT)** of a volume and **exports them to a JSON file**.

The project includes:
- A core **native DLL** in C++ (MFTIndexer)
- A **.NET CLI wrapper** (`MFTIndexerCLI`) that invokes the DLL via `DllImport` and generates `output.json`.

The goal is to index files rapidly without recursive directory traversal, making the results usable in applications such as file launchers or search tools.

---

## 🛠 Architecture

```
MFTIndexer/
├── MFTIndexer/              # Native C++ code (MFTIndexer.dll)
├── MFTIndexerCLI/           # .NET C# CLI wrapper
├── build_and_run.bat        # Script for quick build & execution
├── .gitignore
├── README.md
```

- Entry point: `MFTIndexerCLI/Program.cs`
- Native function used: `ExportMFTToJson(string volumePath, string outputFile)`
- Output: A JSON file with a flat list of all full file paths on the NTFS volume

---

## 🎯 Final goal

This will be integrated into another project: **[WannaTool](https://github.com/Laxyny/WannaTool)**, a Windows productivity launcher like Spotlight.

WannaTool will:
1. Call MFTIndexer to generate `output.json`
2. Load that file to provide **instant file search**

---

## 🧩 What I want Codex to do

### ✅ 1. Ensure the project works as expected
- The C++ DLL builds successfully
- `MFTIndexerCLI` correctly invokes the DLL and creates the expected JSON
- The file paths are real, complete, and valid for the given volume

### ✅ 2. Add silent mode / custom args
- Allow calling the export function with:
  - A specific drive letter (e.g., `C:`)
  - A fully customizable JSON output path
- Provide clear errors when not run as administrator
- Handle invalid drive input gracefully

### ✅ 3. Make the JSON output easy to parse
- Use a clean structure like:
```json
[
  "C:\Windows\explorer.exe",
  "C:\Users\Kevin\Documents\test.pdf"
]
```

### ✅ 4. (Optional) Add in-memory support
- Add a function like `ExportToMemory()` that returns a list of paths instead of writing to a file
- Or add a callback-based approach if possible

---

## 🧪 Next step on my side

From WannaTool, I’ll:
- Call MFTIndexerCLI or the DLL
- Wait for `output.json`
- Parse it and use it to populate the file search UI

You don’t need to do any UI integration. Just ensure MFTIndexer works reliably as a backend.

---

## ℹ️ Notes

- Repo: [https://github.com/Laxyny/MFTIndexer](https://github.com/Laxyny/MFTIndexer)
- Branch: `experimental`
- I will test this right after your changes