// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class MutateArenaTarget : TargetRules
{
	public MutateArenaTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MutateArena");
		
		if (Configuration == UnrealTargetConfiguration.Shipping)
		{
			// BuildEnvironment = TargetBuildEnvironment.Unique;
			
			// GlobalDefinitions.Add("ALLOW_CONSOLE_IN_SHIPPING=1");
			
			// GlobalDefinitions.Add("UE_WITH_CHEAT_MANAGER=1");

			// bUseLoggingInShipping = true;
		}
	}
}
