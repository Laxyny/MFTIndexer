using System;
using System.Runtime.InteropServices;

class Program
{
    [DllImport("MFTIndexer.dll", CharSet = CharSet.Unicode)]
    public static extern bool ExportMFTToJson(string volumePath, string outputFile);

    static void Main(string[] args)
    {
        Console.WriteLine("=== MFTIndexerCLI ===");

        if (args.Length < 1)
        {
            Console.WriteLine("Argument manquant : lettre du lecteur (ex: C)");
            return;
        }

        string driveLetter = args[0];
        string volumePath = driveLetter + ":\\";
        string outputFile = "output.json";

        Console.WriteLine($"Lancement de l'export pour {volumePath}...");
        bool success = ExportMFTToJson(volumePath, outputFile);

        if (success)
        {
            Console.WriteLine($"MFT exportée dans {outputFile}");
        }
        else
        {
            Console.WriteLine($"Échec lors de l'export !");
        }
    }
}
