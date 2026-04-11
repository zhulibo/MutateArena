#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MutateArena/MutateArena.h"
#include "CommonAsset.generated.h"

enum class ECombatIconType : uint8;

USTRUCT(BlueprintType)
struct FSprayPaint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FText Name;
	
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UTexture2D> Texture;
	
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UMaterialInterface> Material;
	
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
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<class UComboBoxItem> ComboBoxItemClass;

	// 倒计时
	UPROPERTY(EditAnywhere, Category = "CountdownSound")
	UMetaSoundSource* CountdownSound;

	// 补给箱音效
	UPROPERTY(EditAnywhere, Category = "SpawnPickupSound")
	UMetaSoundSource* SpawnPickupSound;

	// 战斗图标
	UPROPERTY(EditAnywhere, Category = "CombatIcon")
	UMetaSoundSource* KillSound;
	
	UPROPERTY(EditAnywhere, Category = "CombatIcon")
	TSoftObjectPtr<UTexture2D> Cause1000DamageTexture;
	UPROPERTY(EditAnywhere, Category = "CombatIcon")
	UMetaSoundSource* Cause1000DamageSound;
	
	UPROPERTY(EditAnywhere, Category = "CombatIcon")
	TSoftObjectPtr<UTexture2D> Cause1000RageTexture;
	UPROPERTY(EditAnywhere, Category = "CombatIcon")
	UMetaSoundSource* Cause1000RageSound;
	
	UPROPERTY(EditAnywhere, Category = "CombatIcon")
	TSoftObjectPtr<UTexture2D> BeImmuneTexture;
	UPROPERTY(EditAnywhere, Category = "CombatIcon")
	UMetaSoundSource* BeImmuneSound;
	
	UPROPERTY(EditAnywhere, Category = "CombatIcon")
	UMetaSoundSource* LevelUpSound;

	// 无线电文本
	UPROPERTY(EditAnywhere, Category = "Radio")
	TArray<FString> RadioTexts;
	
	// 喷漆
	UPROPERTY(EditAnywhere, Category = "SprayPaint")
	TArray<FSprayPaint> SprayPaints;
	UPROPERTY(EditAnywhere, Category = "SprayPaint")
	UMetaSoundSource* SprayPaintSound;
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(ASSET_COMMON, GetFName());
	}
	
	// Localization
	UPROPERTY(EditAnywhere, Category = "Localization")
	UStringTable* ST_Common;
	
};
