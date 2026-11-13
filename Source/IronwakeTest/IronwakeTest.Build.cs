using UnrealBuildTool;

public class IronwakeTest : ModuleRules
{
	public IronwakeTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false; // I1: files share anonymous-namespace names (Section, G, ...); unity grouping changes when files are added
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Water", "Niagara", "UMG", "Slate", "SlateCore", "ProceduralMeshComponent", "AssetRegistry", "Json", "JsonUtilities" });
	}
}
