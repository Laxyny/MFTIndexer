using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Principal;

internal class Program
{
    [DllImport("MFTIndexer.dll", CharSet = CharSet.Unicode)]
    public static extern bool ExportMFTToJson(string volumePath, string outputFile);

    private static bool IsAdministrator()
    {
        using WindowsIdentity identity = WindowsIdentity.GetCurrent();
        WindowsPrincipal principal = new(identity);
        return principal.IsInRole(WindowsBuiltInRole.Administrator);
    }

    private static void PrintUsage()
    {
        Console.WriteLine("Usage: MFTIndexerCLI [-d DRIVE] [-o OUTPUT] [--silent]");
        Console.WriteLine("  -d, --drive   Drive letter to index (default: C)");
        Console.WriteLine("  -o, --output  Path to output json file (default: output.json)");
        Console.WriteLine("  -s, --silent  Suppress console output");
    }

    static int Main(string[] args)
    {
        string drive = "C";
        string output = "output.json";
        bool silent = false;

        for (int i = 0; i < args.Length; i++)
        {
            switch (args[i])
            {
                case "-d":
                case "--drive":
                    if (i + 1 >= args.Length)
                    {
                        Console.Error.WriteLine("Missing value for --drive");
                        return 1;
                    }
                    drive = args[++i];
                    break;
                case "-o":
                case "--output":
                    if (i + 1 >= args.Length)
                    {
                        Console.Error.WriteLine("Missing value for --output");
                        return 1;
                    }
                    output = args[++i];
                    break;
                case "-s":
                case "--silent":
                    silent = true;
                    break;
                case "-h":
                case "--help":
                    PrintUsage();
                    return 0;
                default:
                    Console.Error.WriteLine($"Unknown argument: {args[i]}");
                    PrintUsage();
                    return 1;
            }
        }

        if (!Directory.Exists($"{drive}:\\"))
        {
            if (!silent)
                Console.Error.WriteLine($"Invalid drive letter: {drive}");
            return 1;
        }

        if (!IsAdministrator())
        {
            if (!silent)
                Console.Error.WriteLine("Administrator privileges are required.");
            return 1;
        }

        string volumePath = $"\\\\.\\{drive.TrimEnd(':')}:";

        if (!silent)
            Console.WriteLine($"Exporting {volumePath} to {output}...");

        bool success = ExportMFTToJson(volumePath, output);

        if (!silent)
        {
            if (success)
                Console.WriteLine($"MFT exported to {output}");
            else
                Console.WriteLine("Export failed");
        }

        return success ? 0 : 1;
    }
}

