using UnrealBuildTool;

public class SurfGame : ModuleRules
{
	public SurfGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
			"UMG",
			"Slate",
			"SlateCore",
			"DeveloperSettings",
			"MovieScene",
			"MovieSceneTracks"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
