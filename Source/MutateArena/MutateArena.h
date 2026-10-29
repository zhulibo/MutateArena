// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace GameConstants
{
	inline constexpr float TraceLength = 100000.f;

	// 小地图正交宽度
	inline constexpr float MinimapOrthoWidth = 4000.f;

	// 地图最高的物体高度
	inline constexpr float MapMaxHeight = 10000.f;
}

// 颜色
namespace Colors
{
	inline constexpr FColor Red(170, 0, 0, 255);
	inline constexpr FColor Green(0, 130, 0, 255);
	inline constexpr FColor Blue(0, 100, 200, 255);
	inline constexpr FColor Yellow(238, 119, 0, 255);
	inline constexpr FColor White(255, 255, 255, 255);
	inline constexpr FColor Grey(120, 120, 120, 255);
	inline constexpr FColor Black(50, 50, 50, 255);
}

// Socket
namespace Sockets
{
	inline const FName Camera(TEXT("Camera"));
	inline const FName Muzzle(TEXT("Muzzle"));
	inline const FName ShellEject(TEXT("ShellEject"));
	inline const FName WeaponRoot(TEXT("WeaponRoot"));
	inline const FName SuffixR(TEXT("_R"));
	inline const FName SuffixL(TEXT("_L"));
}

// 蒙太奇片段
namespace MontageSections
{
	inline const FName ReloadEnd(TEXT("ReloadEnd"));
}

// DataAsset
namespace DataAsset
{
	inline const FName DNA(TEXT("DNA"));
	inline const FName Common(TEXT("Common"));
	inline const FName CharacterCommon(TEXT("CharacterCommon"));
	inline const FName CharacterHuman(TEXT("CharacterHuman"));
	inline const FName CharacterMutant(TEXT("CharacterMutant"));
	inline const FName EquipmentCommon(TEXT("EquipmentCommon"));
	inline const FName Equipment(TEXT("Equipment"));
	inline const FName Input(TEXT("Input"));
	inline const FName Environment(TEXT("Environment"));
}

// tag
namespace Tags
{
	inline const FName CharacterBase(TEXT("BaseCharacter"));
	inline const FName CharacterHuman(TEXT("HumanCharacter"));
	inline const FName CharacterMutant(TEXT("MutantCharacter"));
	inline const FName Equipment(TEXT("Equipment"));
	inline const FName SmokeActor(TEXT("SmokeActor"));
	inline const FName Herb(TEXT("PickupHerb"));
	inline const FName GameStateMutation(TEXT("GameStateMutation"));
	inline const FName GameStateMelee(TEXT("GameStateMelee"));
	inline const FName GameStateTDM(TEXT("GameStateTDM"));
	inline const FName Ladder(TEXT("Ladder"));
}

// 地图路径
namespace MapPaths
{
	inline const FString Login = TEXT("/Game/Maps/UI_Login");
	inline const FString Menu = TEXT("/Game/Maps/UI_Menu");
}

// 模式
namespace GameModes
{
	inline const FName All(TEXT("All"));
	inline const FName Mutation(TEXT("Mutation"));
	inline const FName TeamDeadMatch(TEXT("TeamDeadMatch"));
	inline const FName Melee(TEXT("Melee"));

	inline constexpr int32 DefaultMatchRound = 12;
	inline constexpr int32 DefaultMatchTime = 10;
}

// 设置
namespace Settings
{
	inline const FName On(TEXT("on.")); // TODO on作为UComboBoxKey的key有bug
	inline const FName Off(TEXT("off."));
	inline const FName Fullscreen(TEXT("Fullscreen"));
	inline const FName WindowedFullscreen(TEXT("WindowedFullscreen"));
	inline const FName Windowed(TEXT("Windowed"));
	inline const FName EN(TEXT("en"));
	inline const FName ZH(TEXT("zh"));
}

// 碰撞通道
namespace ECC
{
	inline constexpr ECollisionChannel MeshTeam1 = ECC_GameTraceChannel1;
	inline constexpr ECollisionChannel MeshTeam2 = ECC_GameTraceChannel2;
	inline constexpr ECollisionChannel MeshHerb = ECC_GameTraceChannel3;
	inline constexpr ECollisionChannel Trace = ECC_GameTraceChannel4;
	inline constexpr ECollisionChannel Projectile = ECC_GameTraceChannel5;
}

// EOS大厅属性
namespace LobbyAttrs
{
	inline const FName SchemaId(TEXT("GameLobby"));

	inline const FName Version(TEXT("Version"));
	inline const FName ServerName(TEXT("ServerName"));
	inline const FName ModeName(TEXT("ModeName"));
	inline const FName MapName(TEXT("MapName"));
	inline const FName MatchRound(TEXT("MatchRound"));
	inline const FName MatchTime(TEXT("MatchTime"));
	inline const FName Status(TEXT("Status"));

	inline const FName MemberName(TEXT("Name"));
	inline const FName MemberTeam(TEXT("Team"));
	inline const FName MemberReady(TEXT("Ready"));
	inline const FName MemberMsg(TEXT("Msg"));
	inline const FName MemberTickNum(TEXT("TickNum"));
}
