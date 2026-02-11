// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class MutateArena : ModuleRules
{
	public MutateArena(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"EnhancedInput",
				"OnlineServicesInterface",
				"CoreOnline",
				"CommonInput",
				"Niagara",
				"GameplayAbilities",
				"ModularGameplayActors",
				"GameplayTags",
				"GameplayTasks",
				"DataRegistry",
				"MetasoundEngine",
				"Json",
				"JsonUtilities",
				"AudioModulation",
				"StateTreeModule",
				"GameplayStateTreeModule",
				"AIModule",
				"NavigationSystem",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Slate",
				"SlateCore",
				"UMG",
				"CommonUI",
				"RHI",
			}
		);

	}
}
