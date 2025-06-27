#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CommonAsset.generated.h"

USTRUCT(BlueprintType)
struct FSprayPaint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FString Name;
	UPROPERTY(EditAnywhere)
	UMaterialInterface* Material =  nullptr;
	
};

UCLASS()
class MUTATEARENA_API UCommonAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// System Sound
	UPROPERTY(EditAnywhere, Category = "SystemSound")
	class USoundMix* SoundMix;
	UPROPERTY(EditAnywhere, Category = "SystemSound")
	class USoundClass* MasterSound;
	UPROPERTY(EditAnywhere, Category = "SystemSound")
	USoundClass* EffectsSound;
	UPROPERTY(EditAnywhere, Category = "SystemSound")
	USoundClass* MusicSound;
	UPROPERTY(EditAnywhere, Category = "SystemSound")
	USoundClass* DialogueSound;

	// UI
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UNotifyLayout> NotifyLayoutClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UConfirmScreen> ConfirmScreenClass;

	// 倒计时
	UPROPERTY(EditAnywhere, Category = "CountdownSound")
	class UMetaSoundSource* CountdownSound;

	// 补给箱音效
	UPROPERTY(EditAnywhere, Category = "SpawnPickupSound")
	class UMetaSoundSource* SpawnPickupSound;

	// 战斗图标
	UPROPERTY(EditAnywhere, Category = "CombatIconSound")
	UMetaSoundSource* Cause1000DamageSound;
	UPROPERTY(EditAnywhere, Category = "CombatIconSound")
	UMetaSoundSource* BeImmuneSound;

	// 喷漆
	UPROPERTY(EditAnywhere, Category = "SprayPaint")
	TArray<FSprayPaint> SprayPaints;

};
