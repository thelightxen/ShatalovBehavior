// (c) XenFFly

using UnrealBuildTool;
using System.Collections.Generic;

public class ShatalovBehaviorTarget : TargetRules
{
	public ShatalovBehaviorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		ExtraModuleNames.AddRange( new string[] { "ShatalovBehavior" } );
	}
}
