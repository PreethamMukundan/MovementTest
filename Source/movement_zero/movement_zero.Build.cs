// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class movement_zero : ModuleRules
{
	public movement_zero(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
