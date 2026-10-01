using UnrealBuildTool;

public class SurfGame : ModuleRules
{
	public SurfGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput" 
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
