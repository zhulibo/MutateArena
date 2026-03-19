#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MutationHuman.generated.h"

class UImage;

UENUM(BlueprintType)
enum class ECombatIconType : uint8
{
	Cause1000Damage,
	BeImmune,
};

enum class ETeam : uint8;

UCLASS()
class MUTATEARENA_API UMutationHuman : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Health;

	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* DamageLabel;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* DamageMul;
	UPROPERTY(meta = (BindWidget))
	UImage* DamageMulImage;
	
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Ammo;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CarriedAmmo;

	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CombatIcon;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;

	void OnHumanHealthChange(float TempHealth);
	void OnAmmoChange(int32 TempAmmo);
	void OnCarriedAmmoChange(int32 TempCarriedAmmo);
	
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* DamageMulMID;
	void OnDamageMulChange(float TempDamageMul);

	void OnCombatIconChange(ECombatIconType CombatIconType);
	FTimerHandle CombatIconTimerHandle;
	void ClearCombatIcon();

};
