using UnrealBuildTool;
using System.Collections.Generic;

public class SurfGameTarget : TargetRules
{
	public SurfGameTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("SurfGame");
	}
}
