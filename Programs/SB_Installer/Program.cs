using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Http.Json;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using System.Threading.Tasks;
using System.Threading;

using FString = string;

class SB_Installer
{
    private static volatile bool isInstallingGit = false;
    private static readonly FString repoUrl = "https://gitlab.com/thelightxen/ShatalovBehavior.git";
    private static readonly FString clonePath = "ShatalovBehavior";
    private static WebClient downloaderClient;
    private static Process cloneProcess;
    private static Process gitInstaller;
    private static ConsoleCtrlDelegate _handler;

    [DllImport("kernel32.dll")]
    private static extern bool SetConsoleCtrlHandler(ConsoleCtrlDelegate handlerRoutine, bool add);

    [DllImport("user32.dll")]
    private static extern IntPtr GetSystemMenu(IntPtr hWnd, bool bRevert);

    [DllImport("user32.dll")]
    private static extern bool RemoveMenu(IntPtr hMenu, uint nPosition, uint wFlags);

    [DllImport("user32.dll")]
    private static extern bool DrawMenuBar(IntPtr hWnd);

    [DllImport("kernel32.dll")]
    private static extern IntPtr GetConsoleWindow();

    private delegate bool ConsoleCtrlDelegate(int ctrlType);

    private const uint SC_CLOSE = 0xF060;
    private const uint MF_BYCOMMAND = 0x00000000;

    private static void DisableClose()
    {
        IntPtr hWnd = GetConsoleWindow();
        if (hWnd != IntPtr.Zero)
        {
            IntPtr hMenu = GetSystemMenu(hWnd, false);
            if (hMenu != IntPtr.Zero)
            {
                RemoveMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
                DrawMenuBar(hWnd);
            }
        }
    }

    private static void EnableClose()
    {
        IntPtr hWnd = GetConsoleWindow();
        if (hWnd != IntPtr.Zero)
        {
            GetSystemMenu(hWnd, true);
            DrawMenuBar(hWnd);
        }
    }

    private static bool ConsoleCtrlCheck(int ctrlType)
    {
        if (isInstallingGit)
        {
            Task.Run(() =>
            {
                MessageBox.Show(
                    "Git installation is in progress. Alt+F4 and Closing are disabled to prevent system corruption.",
                    "Action Blocked",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Warning
                );
            });
            return true;
        }

        CleanupOnExit();
        return false;
    }

    [STAThread]
    static void Main(FString[] args)
    {
        if (args.Length == 0 || args[0] != "--classic")
        {
            try
            {
                ProcessStartInfo psi = new ProcessStartInfo();
                psi.FileName = "conhost.exe";
                psi.Arguments = "\"" + Process.GetCurrentProcess().MainModule.FileName + "\" --classic";
                psi.UseShellExecute = false;
                Process.Start(psi);
                return;
            }
            catch { }
        }

        Console.Title = "ShatalovBehavior Installer";
        _handler = new ConsoleCtrlDelegate(ConsoleCtrlCheck);
        SetConsoleCtrlHandler(_handler, true);

        PrintMessage("Please select project folder...", ConsoleColor.White, "Log");

        FString selectedPath = SelectFolder();

        if (!FString.IsNullOrEmpty(selectedPath))
        {
            FString uprojectFile = GetFileNameByExt(selectedPath, "uproject");
            if (!FString.IsNullOrEmpty(uprojectFile))
            {
                FString uprojectFullPath = Path.Combine(selectedPath, uprojectFile);
                JsonNode? uprojectJson = JsonNode.Parse(File.ReadAllText(uprojectFullPath));
                FString? moduleName = uprojectJson?["Modules"]?[0]?["Name"]?.ToString();
                float? projectVersion = 0;
                
                try {
                    projectVersion = float.Parse(Regex.Match(uprojectJson?["EngineAssociation"]?.ToString()!, @"^\d+\.\d+").Value!, CultureInfo.InvariantCulture);
                } catch {}

                if (projectVersion < 4.27f)
                    PrintMessage($"The project engine version {uprojectJson?["EngineAssociation"]?.ToString()} is lower than the ShatalovBehavior version, build errors are possible.", ConsoleColor.Yellow, "Warning");
                else if (projectVersion > 4.27f)
                    PrintMessage($"The project engine version {uprojectJson?["EngineAssociation"]?.ToString()} is higher than the ShatalovBehavior version, build errors are possible.", ConsoleColor.Yellow, "Warning");

                if (FString.IsNullOrEmpty(moduleName))
                    PrintMessage("Module name not found: most likely the project does not have any C++ class.", ConsoleColor.Red);
                else
                {
                    PrintMessage("Selected module name: " + moduleName, ConsoleColor.Green, "Log");
                    CheckGitInstalled();
                    
                    bool bClone = RunClone(uprojectFullPath);
                    if (bClone || (Path.Exists(clonePath) && Directory.EnumerateFileSystemEntries(clonePath).Any()))
                    {
                        if (!bClone)
                            PrintMessage($"ShatalovBehavior repository is already installed, continue...", ConsoleColor.Yellow, "Warning");

                        PrintMessage("Copying ShatalovBehavior files...", ConsoleColor.White, "Log");
                        FString behaviorFolder = Path.Combine(selectedPath, "Source", moduleName, "Behavior");
                        CopyDirectory(Path.Combine(clonePath, "Source/ShatalovBehavior/Behavior"), behaviorFolder);
                        PrintMessage("Successfully copied", ConsoleColor.Green, "Log");
                        ReplaceAPI(behaviorFolder, moduleName);

                        var dataToSerialize = new
                        {
                            value = new
                            {
                                UPROJECT = uprojectFile,
                                ModuleName = moduleName,
                                Time = DateTime.UtcNow.AddHours(3).ToString("yyyy-MM-dd HH:mm:ss"),
                                Language = CultureInfo.CurrentCulture.Name
                            }
                        };

                        string jsonString = JsonSerializer.Serialize(dataToSerialize);

                        using (var client = new HttpClient())
                        {
                            var content = new StringContent(jsonString, Encoding.UTF8, "application/json");
                            client.PostAsync("https://api.xenffly.com/api/shatalovbehavior/track", content).GetAwaiter().GetResult();
                        }
                    
                        PrintMessage($"INSTALLATION COMPLETED!", ConsoleColor.Green, "Log");
                    }
                    else
                    {
                        PrintMessage("Clone failed: Source files not found.", ConsoleColor.Red);
                    }
                    CleanupOnExit();
                }
            }
            else PrintMessage($"Can't find the uproject file in the path: {selectedPath}", ConsoleColor.Red); 
        }
        Console.WriteLine("Press any key to exit...");
        Console.ReadKey();
    }

    private static void CleanupOnExit()
    {
        if (isInstallingGit) return;
        if (downloaderClient != null) try { downloaderClient.CancelAsync(); } catch { }
        
        KillProcessTree("git.exe");
        KillProcessTree("git-installer.exe");
        
        Thread.Sleep(800);
        
        FString tempPath = Path.Combine(Path.GetTempPath(), "git-installer.exe");
        if (File.Exists(tempPath)) try { File.SetAttributes(tempPath, FileAttributes.Normal); File.Delete(tempPath); } catch { }
        
        if (Directory.Exists(clonePath)) DeleteDirectorySafe(clonePath);
    }

    private static void CheckGitInstalled()
    {
        try
        {
            Process.Start(new ProcessStartInfo("git", "--version") { RedirectStandardOutput = true, UseShellExecute = false, CreateNoWindow = true })?.WaitForExit();
        }
        catch
        {
            PrintMessage(@"Git is not installed. Start installation? [y/n]: ", ConsoleColor.Yellow, "Warning", false);
            if (Console.ReadKey().Key != ConsoleKey.Y) { Console.WriteLine(); return; }
            Console.WriteLine();
            
            isInstallingGit = true;
            DisableClose();
            FString tempFile = Path.Combine(Path.GetTempPath(), "git-installer.exe");
            try
            {
                PrintMessage(@"Downloading Git...", ConsoleColor.White, "Log");
                DownloadFile("https://github.com/git-for-windows/git/releases/download/v2.50.1.windows.1/Git-2.50.1-64-bit.exe", tempFile);
                PrintMessage(@"Installing Git...", ConsoleColor.White, "Log");
                InstallGit(tempFile);
                PrintMessage(@"Successfully installed Git v2.50.1", ConsoleColor.Green, "Log");
            }
            finally
            {
                isInstallingGit = false;
                EnableClose();
            }
        }
    }

    private static void DownloadFile(FString url, FString filePath)
    {
        downloaderClient = new WebClient();
        try { downloaderClient.DownloadFile(new Uri(url), filePath); }
        catch { }
        finally { downloaderClient.Dispose(); downloaderClient = null; }
    }

    private static void InstallGit(FString path)
    {
        var psi = new ProcessStartInfo(path) { Arguments = "/VERYSILENT /NORESTART", UseShellExecute = false, CreateNoWindow = true };
        Process.Start(psi)?.WaitForExit();
    }

    private static bool RunClone(FString installDir)
    {
        if (Directory.Exists(clonePath)) DeleteDirectorySafe(clonePath);

        PrintMessage("Executing Git process...", ConsoleColor.White, "Log");
        var psi = new ProcessStartInfo { 
            FileName = "git", 
            Arguments = $"clone --progress --depth 1 {repoUrl} {clonePath}", 
            UseShellExecute = false, 
            CreateNoWindow = false 
        };
        
        try {
            cloneProcess = Process.Start(psi);
            cloneProcess?.WaitForExit();
            if (cloneProcess.ExitCode != 0)
            {
                PrintMessage($"Git clone failed.\nExit code: {cloneProcess.ExitCode}", ConsoleColor.Red);
                return false;
            }
            else
            {
                PrintMessage($"Git clone finished.", ConsoleColor.Green, "Log");
                return true;
            }
        } catch (Exception ex)
        {
            PrintMessage($"Exception during git clone: {ex.Message}", ConsoleColor.Red);
            return false;
        }
    }

    private static void KillProcessTree(string imageName)
    {
        try { Process.Start(new ProcessStartInfo { FileName = "taskkill", Arguments = $"/F /IM {imageName} /T", CreateNoWindow = true, UseShellExecute = false })?.WaitForExit(1000); } catch { }
    }

    public static void DeleteDirectorySafe(string targetDir)
    {
        if (!Directory.Exists(targetDir)) return;

        for (int i = 0; i < 5; i++)
        {
            try
            {
                foreach (string file in Directory.GetFiles(targetDir, "*", SearchOption.AllDirectories))
                {
                    File.SetAttributes(file, FileAttributes.Normal);
                    File.Delete(file);
                }
                Directory.Delete(targetDir, true);
                return;
            }
            catch { Thread.Sleep(500); }
        }
        
        try {
            Process.Start(new ProcessStartInfo { FileName = "cmd.exe", Arguments = $"/C timeout /t 1 & rd /s /q \"{Path.GetFullPath(targetDir)}\"", CreateNoWindow = true, UseShellExecute = false });
        } catch {}
    }

    static FString SelectFolder()
    {
        using (FolderBrowserDialog fbd = new FolderBrowserDialog { Description = "Select the project folder", UseDescriptionForTitle = true }) 
        { if (fbd.ShowDialog() == DialogResult.OK) return fbd.SelectedPath; }
        return null;
    }

    static FString GetFileNameByExt(FString dirPath, FString Ext)
    {
        if (Directory.Exists(dirPath))
        {
            FString[] files = Directory.GetFiles(dirPath, "*" + (Ext.StartsWith(".") ? Ext : "." + Ext));
            if (files.Length > 0) return Path.GetFileName(files[0]);
        }
        return "";
    }

    static void PrintMessage(FString message, ConsoleColor color, FString type = "Error", bool newLine = true)
    {
        Console.ForegroundColor = color;
        if (newLine) Console.WriteLine($"[{type}] {message}"); else Console.Write($"[{type}] {message}");
        Console.ResetColor();
    }

    public static void CopyDirectory(string sourceDir, string destinationDir)
    {
        if (!Directory.Exists(sourceDir)) return;
        Directory.CreateDirectory(destinationDir);
        foreach (FileInfo file in new DirectoryInfo(sourceDir).GetFiles()) file.CopyTo(Path.Combine(destinationDir, file.Name), true);
        foreach (DirectoryInfo subDir in new DirectoryInfo(sourceDir).GetDirectories()) CopyDirectory(subDir.FullName, Path.Combine(destinationDir, subDir.Name));
    }

    public static void ReplaceAPI(FString folderPath, FString moduleName)
    {
        FString shatalovAPI = "SHATALOVBEHAVIOR_API";
        FString newAPI = moduleName.ToUpper() + "_API";
        
        if (!Directory.Exists(folderPath)) return;

        var files = Directory.EnumerateFiles(folderPath, "*.*", SearchOption.AllDirectories)
            .Where(f => f.EndsWith(".cpp") || f.EndsWith(".h"));

        PrintMessage($"Updating API to {newAPI}...", ConsoleColor.White, "Log");

        foreach (FString filePath in files)
        {
            try
            {
                string[] lines = File.ReadAllLines(filePath);
                bool fileModified = false;
                List<string> logForThisFile = new List<string>();

                for (int i = 0; i < lines.Length; i++)
                {
                    if (lines[i].Contains(shatalovAPI))
                    {
                        string oldLine = lines[i].Trim();
                        lines[i] = lines[i].Replace(shatalovAPI, newAPI);
                        logForThisFile.Add($"  [Line {i + 1}]: {oldLine}  -->  {lines[i].Trim()}");
                        fileModified = true;
                    }
                }

                if (fileModified)
                {
                    File.WriteAllLines(filePath, lines);
                    PrintMessage($"Updated at {filePath}", ConsoleColor.Green, "Log");
                    foreach (var log in logForThisFile) Console.WriteLine(log);
                }
            }
            catch (Exception ex)
            {
                PrintMessage($"Error processing {filePath}: {ex.Message}", ConsoleColor.Red);
            }
        }
    }
}