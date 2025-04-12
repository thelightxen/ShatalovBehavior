// (c) XenFFly

using UnrealBuildTool;
using System.Collections.Generic;

public class ShatalovBehaviorEditorTarget : TargetRules
{
	public ShatalovBehaviorEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		ExtraModuleNames.AddRange( new string[] { "ShatalovBehavior" } );
	}
}
