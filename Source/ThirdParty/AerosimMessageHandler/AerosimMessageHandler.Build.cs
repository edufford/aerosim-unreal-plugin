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
    }
}
