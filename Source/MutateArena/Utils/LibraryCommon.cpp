#include "LibraryCommon.h"

#include "MutateArena/System/Storage/SaveGameSetting.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "Kismet/KismetMathLibrary.h"

FString ULibraryCommon::GetProjectVersion()
{
	FString ProjectVersion;
	GConfig->GetString(
		TEXT("/Script/EngineSettings.GeneralProjectSettings"),
		TEXT("ProjectVersion"),
		ProjectVersion,
		GGameIni
	);
	return ProjectVersion;
}

int32 ULibraryCommon::GetBloodParticleCount(float Damage)
{
	return UKismetMathLibrary::MapRangeClamped(Damage, 40, 100, 4, 10);
}

FString ULibraryCommon::GetFormatTime(int32 CountdownTime)
{
	int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
	int32 Seconds = CountdownTime - Minutes * 60;
	return  FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}

FString ULibraryCommon::GetNowFormatTime()
{
	FDateTime Time = FDateTime::Now();
	int32 Hour = Time.GetHour();
	int32 Minute = Time.GetMinute();
	int32 Second = Time.GetSecond();
	return FString::Printf(TEXT("%02d:%02d:%02d"), Hour, Minute, Second);
}

FString ULibraryCommon::GetEnumValue(const FString& EnumValue)
{
	return EnumValue.Right(EnumValue.Len() - EnumValue.Find("::") - 2);
}

FColor ULibraryCommon::GetProgressColor(double Value, double InRangeA, double InRangeB, FColor InColor, FColor OutColor)
{
	int32 R = UKismetMathLibrary::MapRangeClamped(Value, InRangeA, InRangeB, InColor.R, OutColor.R);
	int32 G = UKismetMathLibrary::MapRangeClamped(Value, InRangeA, InRangeB, InColor.G, OutColor.G);
	int32 B = UKismetMathLibrary::MapRangeClamped(Value, InRangeA, InRangeB, InColor.B, OutColor.B);
	int32 A = UKismetMathLibrary::MapRangeClamped(Value, InRangeA, InRangeB, InColor.A, OutColor.A);
	
	return FColor(R, G, B, A);
}

FString ULibraryCommon::ObfuscatePlayerName(FString PlayerName, const UObject* Context)
{
	if (Context == nullptr) return PlayerName;

	if (UGameInstance* GameInstance = Context->GetWorld()->GetGameInstance())
	{
		UStorageSubsystem* StorageSubsystem = GameInstance->GetSubsystem<UStorageSubsystem>();
		// 已开启混淆
		if (StorageSubsystem && StorageSubsystem->CacheSetting && StorageSubsystem->CacheSetting->ObfuscatePlayerName == true)
		{
			UEOSSubsystem* EOSSubsystem = GameInstance->GetSubsystem<UEOSSubsystem>();
			// 非本人名字
			if (EOSSubsystem && EOSSubsystem->GetPlayerName() != PlayerName)
			{
				return ObfuscateText(PlayerName);
			}
		}
	}

	return PlayerName;
}

FString ULibraryCommon::ObfuscateTextChat(FString Msg, const UObject* Context)
{
	if (Context && Context->GetWorld() && Context->GetWorld()->GetGameInstance())
	{
		if (UStorageSubsystem* StorageSubsystem = Context->GetWorld()->GetGameInstance()->GetSubsystem<UStorageSubsystem>())
		{
			if (StorageSubsystem->CacheSetting && StorageSubsystem->CacheSetting->ObfuscateTextChat == true)
			{
				return ObfuscateText(Msg);
			}
		}
	}

	return Msg;
}

FString ULibraryCommon::ObfuscateText(FString Text)
{
	FString ObfuscatedString;

	const TCHAR ReplacementTable[] = TEXT("!@#$%^&*()_+");
	const int32 TableSize = UE_ARRAY_COUNT(ReplacementTable) - 1; // 去掉结尾的'\0'

	for (TCHAR Character : Text)
	{
		// 获取字符的ASCII码偏移量
		int32 AsciiOffset = static_cast<int32>(Character) % TableSize;

		// 替换字符
		TCHAR ReplacedCharacter = ReplacementTable[AsciiOffset];

		// 追加到结果字符串
		ObfuscatedString.AppendChar(ReplacedCharacter);
	}

	return ObfuscatedString;
}
