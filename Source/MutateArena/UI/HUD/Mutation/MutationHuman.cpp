#include "MutationHuman.h"

#include "CommonTextBlock.h"
#include "MetaSoundSource.h"
#include "MutateArena/PlayerControllers/MutationController.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/System/UISubsystem.h"

void UMutationHuman::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->OnHumanHealthChange.AddUObject(this, &ThisClass::OnHumanHealthChange);
		UISubsystem->OnAmmoChange.AddUObject(this, &ThisClass::OnAmmoChange);
		UISubsystem->OnCarriedAmmoChange.AddUObject(this, &ThisClass::OnCarriedAmmoChange);
		UISubsystem->OnDamageMulChange.AddUObject(this, &ThisClass::OnDamageMulChange);
		UISubsystem->OnCause1000Damage.AddUObject(this, &ThisClass::OnCombatIconChange, ECombatIconType::Cause1000Damage);
		UISubsystem->OnBeImmune.AddUObject(this, &ThisClass::OnCombatIconChange, ECombatIconType::BeImmune);
	}
}

void UMutationHuman::OnHumanHealthChange(float TempHealth)
{
	FNumberFormattingOptions Opts;
	Opts.RoundingMode = ERoundingMode::ToPositiveInfinity; // 向上取整
	Opts.SetUseGrouping(false); // 不使用千位分隔符
	Opts.SetMaximumFractionalDigits(0);
	Health->SetText(FText::AsNumber(TempHealth, &Opts));
}

void UMutationHuman::OnAmmoChange(int32 TempAmmo)
{
	Ammo->SetText(FText::AsNumber(TempAmmo));
}

void UMutationHuman::OnCarriedAmmoChange(int32 TempCarriedAmmo)
{
	CarriedAmmo->SetText(FText::AsNumber(TempCarriedAmmo));
}

void UMutationHuman::OnDamageMulChange(float TempDamageMul)
{
	DamageMul->SetText(FText::AsPercent(TempDamageMul));
}

void UMutationHuman::OnCombatIconChange(ECombatIconType CombatIconType)
{
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem == nullptr || AssetSubsystem->CommonAsset == nullptr) return;

	switch (CombatIconType)
	{
	case ECombatIconType::Cause1000Damage:
		CombatIcon->SetText(FText::FromString(TEXT("1000 DAMAGE")));
		
		if (UAudioComponent* AudioComponent = UGameplayStatics::SpawnSound2D(this, AssetSubsystem->CommonAsset->Cause1000DamageSound))
		{
			// AudioComponent->SetFloatParameter(TEXT("Index"), 1);
		}
		
		break;
		
	case ECombatIconType::BeImmune:
		CombatIcon->SetText(FText::FromString(TEXT("BeImmune")));
		
		if (UAudioComponent* AudioComponent = UGameplayStatics::SpawnSound2D(this, AssetSubsystem->CommonAsset->Cause1000DamageSound))
		{
			// AudioComponent->SetFloatParameter(TEXT("Index"), 1);
		}
		
		break;
	}

	GetWorld()->GetTimerManager().SetTimer(CombatIconTimerHandle, this, &ThisClass::ClearCombatIcon, 3.f);
}

void UMutationHuman::ClearCombatIcon()
{
	CombatIcon->SetText(FText());
}
