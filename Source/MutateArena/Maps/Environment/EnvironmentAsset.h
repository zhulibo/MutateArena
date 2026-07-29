#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MutateArena/MutateArena.h"
#include "EnvironmentAsset.generated.h"

class UGameplayEffect;

UCLASS()
class MUTATEARENA_API UEnvironmentAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FGameplayTag EnvironmentTag;
	
	UPROPERTY(EditAnywhere)
	FText ShowName;
	
	UPROPERTY(EditAnywhere)
	FText Desc;

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<UGameplayEffect>> HumanEffects;

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<UGameplayEffect>> MutantEffects;

	// 泛型视觉参数字典 Tag作为Key，支持无限扩展
	// 浮点参数 
	UPROPERTY(EditAnywhere)
	TMap<FGameplayTag, float> ScalarParameters;

	// 颜色参数
	UPROPERTY(EditAnywhere)
	TMap<FGameplayTag, FLinearColor> ColorParameters;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(ASSET_ENVIRONMENT, GetFName());
	}
};
