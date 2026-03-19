#include "LibraryCommon.h"

#include "ObfuscateWords.h"
#include "GameFramework/GameStateBase.h"
#include "Internationalization/Culture.h"
#include "MutateArena/System/Storage/SaveGameSetting.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "Kismet/KismetMathLibrary.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"

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
	return UKismetMathLibrary::MapRangeClamped(Damage, 40, 100, 2, 6);
}

FString ULibraryCommon::GetFormatTime(int32 CountdownTime)
{
	if (CountdownTime < 0) CountdownTime = 0;
	
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

FString ULibraryCommon::GetHashPrefix(const FString& Name)
{
	uint32 NameHash = GetTypeHash(Name);
	
	int32 AdjIndex = NameHash % ObfuscateDict::Adjectives.Num();
	int32 NounIndex = (NameHash / ObfuscateDict::Adjectives.Num()) % ObfuscateDict::Nouns.Num();
	
	return ObfuscateDict::Adjectives[AdjIndex] + ObfuscateDict::Nouns[NounIndex];
}

FString ULibraryCommon::ObfuscateName(FString Name, const UObject* Context)
{
	if (Context == nullptr || Name.IsEmpty()) return Name;

	if (UGameInstance* GameInstance = Context->GetWorld()->GetGameInstance())
	{
		UStorageSubsystem* StorageSubsystem = GameInstance->GetSubsystem<UStorageSubsystem>();
		// 已开启混淆
		if (StorageSubsystem && StorageSubsystem->CacheSetting && StorageSubsystem->CacheSetting->bObfuscateName == true)
		{
			UEOSSubsystem* EOSSubsystem = GameInstance->GetSubsystem<UEOSSubsystem>();
			// 非本人名字
			UE_LOG(LogTemp, Warning, TEXT("EOSSubsystem->GetPlayerName() %s Name %s"), *EOSSubsystem->GetPlayerName(), *Name);
			if (EOSSubsystem && EOSSubsystem->GetPlayerName() != Name)
			{
				// 静态缓存
				static TMap<FString, FString> ObfuscatedNameCache;
				
				if (ObfuscatedNameCache.Num() > 1000)
				{
					ObfuscatedNameCache.Empty();
				}
				
				if (FString* CachedName = ObfuscatedNameCache.Find(Name))
				{
					return *CachedName;
				}

				// 如果没命中缓存，则进行首次计算
				FString XX = GetHashPrefix(Name);
				FString YY = ObfuscateText(Name);
				FString ResultName = FString::Printf(TEXT("%s-%s"), *XX, *YY);

				ObfuscatedNameCache.Add(Name, ResultName);

				return ResultName;
			}
		}
	}

	return Name;
}

FString ULibraryCommon::ObfuscateServerName(FString Msg, const UObject* Context)
{
	if (Context && Context->GetWorld() && Context->GetWorld()->GetGameInstance())
	{
		if (UStorageSubsystem* StorageSubsystem = Context->GetWorld()->GetGameInstance()->GetSubsystem<UStorageSubsystem>())
		{
			if (StorageSubsystem->CacheSetting && StorageSubsystem->CacheSetting->bObfuscateName == true)
			{
				return ObfuscateText(Msg);
			}
		}
	}

	return Msg;
}

FString ULibraryCommon::ObfuscateTextChat(FString Msg, const UObject* Context)
{
	if (Context && Context->GetWorld() && Context->GetWorld()->GetGameInstance())
	{
		if (UStorageSubsystem* StorageSubsystem = Context->GetWorld()->GetGameInstance()->GetSubsystem<UStorageSubsystem>())
		{
			if (StorageSubsystem->CacheSetting && StorageSubsystem->CacheSetting->bObfuscateTextChat == true)
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

	static constexpr TCHAR ReplacementTable[] = TEXT("~!@#$%^&*()_+-={}|[]<>?,./");
	static constexpr int32 TableSize = UE_ARRAY_COUNT(ReplacementTable) - 1; // 去掉结尾的'\0'

	for (TCHAR Character : Text)
	{
		// 获取字符的ASCII码偏移量
		int32 AsciiOffset = static_cast<int32>(Character) % TableSize;

		// 替换字符
		TCHAR ReplacedCharacter = ReplacementTable[AsciiOffset];

		// 追加到结果字符串
		ObfuscatedString.AppendChar(ReplacedCharacter);
	}

	return ObfuscatedString + FString::FromInt(Text.Len());
}

FString ULibraryCommon::GetLanguage()
{
	FCulturePtr CurCulture = FInternationalization::Get().GetCurrentCulture();

	if (CurCulture.IsValid())
	{
		return CurCulture->GetName();
	}

	return TEXT("en");
}
