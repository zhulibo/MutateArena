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
	UPROPERTY(EditAnywhere, Category = "Audio")
	class USoundMix* SoundMix;
	UPROPERTY(EditAnywhere, Category = "Audio")
	class USoundClass* MasterSound;
	UPROPERTY(EditAnywhere, Category = "Audio")
	USoundClass* EffectsSound;
	UPROPERTY(EditAnywhere, Category = "Audio")
	USoundClass* MusicSound;
	UPROPERTY(EditAnywhere, Category = "Audio")
	USoundClass* DialogueSound;
	UPROPERTY(EditAnywhere, Category = "Audio")
	class USoundControlBus* CB_EquipmentMech;
	
	// UI
	UPROPERTY(EditAnywhere, Category = "UI")
	class UMetaSoundSource* TabSwitchSound;
	
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UNotifyLayout> NotifyLayoutClass;
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UConfirmScreen> ConfirmScreenClass;

	// 倒计时
	UPROPERTY(EditAnywhere, Category = "CountdownSound")
	UMetaSoundSource* CountdownSound;

	// 补给箱音效
	UPROPERTY(EditAnywhere, Category = "SpawnPickupSound")
	UMetaSoundSource* SpawnPickupSound;

	// 战斗图标
	UPROPERTY(EditAnywhere, Category = "CombatIconSound")
	UMetaSoundSource* Cause1000DamageSound;
	UPROPERTY(EditAnywhere, Category = "CombatIconSound")
	UMetaSoundSource* BeImmuneSound;

	// 无线电文本
	UPROPERTY(EditAnywhere, Category = "Radio")
	TArray<FString> RadioTexts;
	
	// 喷漆
	UPROPERTY(EditAnywhere, Category = "SprayPaint")
	TArray<FSprayPaint> SprayPaints;

};
