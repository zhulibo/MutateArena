// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// 颜色
#define C_RED FColor(170, 0, 0, 255)
#define C_GREEN FColor(0, 130, 0, 255)
#define C_BLUE FColor(0, 100, 200, 255)
#define C_YELLOW FColor(238, 119, 0, 255)
#define C_WHITE FColor(255, 255, 255, 255)
#define C_GREY FColor(120, 120, 120, 255)
#define C_BLACK FColor(50, 50, 50, 255)

// Socket
#define SOCKET_CAMERA FName(TEXT("CameraSocket"))
#define SOCKET_MUZZLE FName(TEXT("Muzzle"))
#define SOCKET_SHELL_EJECT FName(TEXT("ShellEject"))
#define SOCKET_SCOPE_START FName(TEXT("ScopeStartSocket"))
#define SOCKET_SCOPE_END FName(TEXT("ScopeEndSocket"))
#define SOCKET_HAND_RIGHT FName(TEXT("RightHandCapsule"))
#define SOCKET_HAND_LEFT FName(TEXT("LeftHandCapsule"))

// tag
#define TAG_CHARACTER_BASE FName(TEXT("BaseCharacter"))
#define TAG_CHARACTER_HUMAN FName(TEXT("HumanCharacter"))
#define TAG_CHARACTER_MUTANT FName(TEXT("MutantCharacter"))
#define TAG_EQUIPMENT FName(TEXT("Equipment"))
#define TAG_SMOKE_ACTOR FName(TEXT("SmokeActor"))
#define TAG_HERB FName(TEXT("PickupHerb"))
#define TAG_GAME_STATE_MUTATION FName(TEXT("GameStateMutation"))
#define TAG_GAME_STATE_MELEE FName(TEXT("GameStateMelee"))
#define TAG_GAME_STATE_TDM FName(TEXT("GameStateTDM"))

// 地图路径
#define MAP_LOGIN FString(TEXT("/Game/Maps/UI_Login"))
#define MAP_MENU FString(TEXT("/Game/Maps/UI_Menu"))

// 模式
#define ALL FString(TEXT("All"))
#define MUTATION FString(TEXT("Mutation"))
#define TEAM_DEAD_MATCH FString(TEXT("TeamDeadMatch"))
#define MELEE FString(TEXT("Melee"))

// 角色碰撞通道
#define ECC_MESH_TEAM1 ECollisionChannel::ECC_GameTraceChannel1
#define ECC_MESH_TEAM2 ECollisionChannel::ECC_GameTraceChannel2
#define ECC_MESH_HREB ECollisionChannel::ECC_GameTraceChannel3

// EOS大厅属性
#define LOBBY_SCHEMA_ID FName(TEXT("GameLobby"))

#define LOBBY_VERSION FName(TEXT("Version"))
#define LOBBY_SERVER_NAME FName(TEXT("ServerName"))
#define LOBBY_MODE_NAME FName(TEXT("ModeName"))
#define LOBBY_MAP_NAME FName(TEXT("MapName"))
#define LOBBY_STATUS FName(TEXT("Status"))

#define LOBBY_MEMBER_NAME FName(TEXT("Name"))
#define LOBBY_MEMBER_TEAM FName(TEXT("Team"))
#define LOBBY_MEMBER_READY FName(TEXT("Ready"))
#define LOBBY_MEMBER_MSG FName(TEXT("Msg"))
#define LOBBY_MEMBER_TICK_NUM FName(TEXT("TickNum"))

// GAS
#define TAG_MUTANT_SKILL_CD FName(TEXT("Mutant.Skill.CD"))
#define TAG_MUTANT_CHANGE_ACTIVE FName(TEXT("Mutant.Change.Active"))

// 数据注册表
#define DR_HUMAN_CHARACTER_MAIN FName(TEXT("DR_HumanCharacterMain"))
#define DR_MUTANT_CHARACTER_MAIN FName(TEXT("DR_MutantCharacterMain"))
#define DR_EQUIPMENT_MAIN FName(TEXT("DR_EquipmentMain"))
#define DR_WEAPON_DATA FName(TEXT("DR_WeaponData"))
#define DR_MELEE_DATA FName(TEXT("DR_MeleeData"))
#define DR_EQUIPMENT_RECOIL FName(TEXT("DR_EquipmentRecoil"))

// 多语言
#define CULTURE_HUMAN FString(TEXT("Human"))
#define CULTURE_MUTANT FString(TEXT("Mutant"))
#define CULTURE_EQUIPMENT FString(TEXT("Equipment"))
#define CULTURE_EQUIPMENT_TYPE FString(TEXT("EquipmentType"))
#define CULTURE_SPRAY_PAINT FString(TEXT("SprayPaint"))
