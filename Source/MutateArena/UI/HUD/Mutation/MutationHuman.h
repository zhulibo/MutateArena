#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "MutationHuman.generated.h"

class UImage;

enum class ETeam : uint8;

UCLASS()
class MUTATEARENA_API UMutationHuman : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Health;
	
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* Ammo;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CarriedAmmo;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;

	void OnHumanHealthChange(float TempHealth);
	void OnAmmoChange(int32 TempAmmo);
	void OnCarriedAmmoChange(int32 TempCarriedAmmo);
	
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* DamageLabel;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* DamageMul;
	UPROPERTY(meta = (BindWidget))
	UImage* DamageMulImage;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* MeleeDamageLabel;
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* MeleeDamageMul;
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* DamageMulMID;
	void OnDamageMulChange(float TempDamageMul);
	void OnMeleeDamageMulChange(float TempMeleeDamageMul);

};
