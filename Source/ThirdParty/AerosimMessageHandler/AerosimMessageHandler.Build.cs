using System;
using System.IO;
using UnrealBuildTool;


public class AerosimMessageHandler : ModuleRules
{
    public AerosimMessageHandler(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;
        bool IsWindows = Target.Platform == UnrealTargetPlatform.Win64;

        string AerosimMessageHandlerPluginSourcePath = Path.GetFullPath(ModuleDirectory);
        string AerosimWorldLinkLibVar = Environment.GetEnvironmentVariable("AEROSIM_WORLD_LINK_LIB");
        if (AerosimWorldLinkLibVar == null)
        {
            throw new Exception("AEROSIM_WORLD_LINK_LIB environment variable is not set. Please set it to the path of the aerosim-world-link library.");
        }

        string AerosimWorldLinkLibPath = Path.GetFullPath(AerosimWorldLinkLibVar);
        Console.WriteLine("AEROSIM_WORLD_LINK_LIB environment variable is set to: " + AerosimWorldLinkLibVar);
        PublicIncludePaths.Add(AerosimWorldLinkLibPath);

        string AerosimWorldLinkBaseLibFilename = "aerosim_world_link";
        string AerosimWorldLinkDynLibFilename;
        if (IsWindows)
        {
            AerosimWorldLinkDynLibFilename = AerosimWorldLinkBaseLibFilename + ".dll";
            PublicAdditionalLibraries.Add(Path.Combine(AerosimWorldLinkLibPath, AerosimWorldLinkBaseLibFilename + ".lib"));
        }
        else
        {
            AerosimWorldLinkDynLibFilename = "lib" + AerosimWorldLinkBaseLibFilename + ".so";
            PublicAdditionalLibraries.Add(Path.Combine(AerosimWorldLinkLibPath, AerosimWorldLinkDynLibFilename));
        }

        string AerosimWorldLinkDynLibPath = Path.Combine(AerosimWorldLinkLibPath, AerosimWorldLinkDynLibFilename);
        RuntimeDependencies.Add(Path.Combine("$(BinaryOutputDir)", AerosimWorldLinkDynLibFilename), AerosimWorldLinkDynLibPath);
        PublicDelayLoadDLLs.Add(AerosimWorldLinkDynLibPath);

        if (IsWindows)
        {
            // Manually add Unreal's Python DLLs to packaged binaries for Windows.
            // The versioned DLL name varies by UE version (e.g. python39.dll in UE 5.3, python311.dll in UE 5.7).
            string UnrealEnginePath = Environment.GetEnvironmentVariable("AEROSIM_UNREAL_ENGINE_ROOT");
            string Python3LibPath = Path.Combine(UnrealEnginePath, "Engine/Binaries/ThirdParty/Python3/Win64");
            RuntimeDependencies.Add(Path.Combine("$(BinaryOutputDir)", "python3.dll"), Python3LibPath + "/python3.dll");
            string[] VersionedPythonDlls = Directory.GetFiles(Python3LibPath, "python3*.dll");
            foreach (string VersionedDll in VersionedPythonDlls)
            {
                string DllFilename = Path.GetFileName(VersionedDll);
                if (DllFilename != "python3.dll")
                {
                    RuntimeDependencies.Add(Path.Combine("$(BinaryOutputDir)", DllFilename), VersionedDll);
                }
            }
        }
    }
}
