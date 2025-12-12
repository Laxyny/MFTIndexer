using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Principal;

internal class Program
{
    [DllImport("MFTIndexer.dll", CharSet = CharSet.Unicode)]
    public static extern bool ExportMFTToJson(string volumePath, string outputFile);

    private static bool IsAdministrator()
    {
        try
        {
            using WindowsIdentity identity = WindowsIdentity.GetCurrent();
            WindowsPrincipal principal = new(identity);
            return principal.IsInRole(WindowsBuiltInRole.Administrator);
        }
        catch
        {
            return false;
        }
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

        try
        {
            for (int i = 0; i < args.Length; i++)
            {
                switch (args[i])
                {
                    case "-d":
                    case "--drive":
                        if (i + 1 >= args.Length)
                        {
                            if (!silent) Console.Error.WriteLine("Missing value for --drive");
                            return 1;
                        }
                        drive = args[++i];
                        break;
                    case "-o":
                    case "--output":
                        if (i + 1 >= args.Length)
                        {
                            if (!silent) Console.Error.WriteLine("Missing value for --output");
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
                        if (!silent) Console.Error.WriteLine($"Unknown argument: {args[i]}");
                        PrintUsage();
                        return 1;
                }
            }

            // Normalize drive letter
            drive = drive.TrimEnd(':', '\\', '/').ToUpper();
            if (drive.Length != 1 || drive[0] < 'A' || drive[0] > 'Z')
            {
                if (!silent) Console.Error.WriteLine($"Invalid drive letter: {drive}");
                return 1;
            }

            if (!Directory.Exists($"{drive}:\\"))
            {
                if (!silent) Console.Error.WriteLine($"Drive {drive}: does not exist or is not accessible.");
                return 1;
            }

            if (!IsAdministrator())
            {
                if (!silent) Console.Error.WriteLine("Administrator privileges are required to access MFT.");
                return 1;
            }

            string volumePath = $"\\\\.\\{drive}:";
            string absoluteOutput = Path.GetFullPath(output);

            if (!silent)
            {
                Console.WriteLine($"Indexing drive {drive}: ...");
                Console.WriteLine($"Output file: {absoluteOutput}");
            }

            Stopwatch sw = Stopwatch.StartNew();
            bool success = ExportMFTToJson(volumePath, absoluteOutput);
            sw.Stop();

            if (!success)
            {
                if (!silent) Console.Error.WriteLine("Export failed. Check disk access or errors.");
                return 1;
            }

            if (!silent)
            {
                Console.WriteLine($"Done in {sw.Elapsed.TotalSeconds:F2}s.");
                if (File.Exists(absoluteOutput))
                {
                    var info = new FileInfo(absoluteOutput);
                    Console.WriteLine($"File size: {info.Length / 1024.0 / 1024.0:F2} MB");
                }
            }

            return 0;
        }
        catch (Exception ex)
        {
            if (!silent) Console.Error.WriteLine($"An unexpected error occurred: {ex.Message}");
            return 1;
        }
    }
}
