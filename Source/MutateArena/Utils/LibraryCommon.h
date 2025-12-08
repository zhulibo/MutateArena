#pragma once

#include "CoreMinimal.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "UObject/NoExportTypes.h"
#include "LibraryCommon.generated.h"

UCLASS()
class MUTATEARENA_API ULibraryCommon : public UObject
{
	GENERATED_BODY()

public:
	static FString GetProjectVersion();

	static int32 GetBloodParticleCount(float Damage);
	
	static FString GetFormatTime(int32 CountdownTime);
	static FString GetNowFormatTime();
	static FString GetEnumValue(const FString& EnumValue);
	
	static FColor GetProgressColor(double Value, double InRangeA, double InRangeB, FColor InColor, FColor OutColor);

	
	static FString ObfuscateName(FString PlayerName, const UObject* Context);
	static FString ObfuscateTextChat(FString Msg, const UObject* Context);
	static FString ObfuscateText(FString Text);

	static FString GetLanguage();
	
	// 获取UMG子集
	template<typename T>
	static TArray<T*> GetAllChildrenOfClass(UWidget* Parent, bool bRecursive = false)
	{
		TArray<T*> Result;

		if (!Parent) return Result;

		UPanelWidget* Panel = Cast<UPanelWidget>(Parent);
		if (!Panel) return Result;

		for (int32 i = 0; i < Panel->GetChildrenCount(); i++)
		{
			UWidget* Child = Panel->GetChildAt(i);

			if (T* Casted = Cast<T>(Child))
			{
				Result.Add(Casted);
			}

			if (bRecursive)
			{
				auto Sub = GetAllChildrenOfClass<T>(Child);
				Result.Append(Sub);
			}
		}
		
		return Result;
	}
};
