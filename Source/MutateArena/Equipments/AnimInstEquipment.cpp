#include "AnimInstEquipment.h"

#include "Equipment.h"
#include "Weapon.h"
#include "MutateArena/Characters/AnimInstHuman.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"

UAnimInstEquipment::UAnimInstEquipment()
{
}

void UAnimInstEquipment::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (Equipment == nullptr) Equipment = Cast<AEquipment>(GetOwningActor());
	if (Equipment == nullptr) return;

	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(Equipment->GetOwner());
	if (HumanCharacter == nullptr || HumanCharacter->CombatComp == nullptr || HumanCharacter->CombatComp->GetCurWeapon() == nullptr) return;

	if (HumanCharacter->IsLocallyControlled()) // TODO 非本地瞄准动画暂时禁用了
	{
		if (!Montage_IsPlaying(HumanCharacter->CombatComp->GetCurWeapon()->ADSMontage_E))
		{
			bIsAiming = HumanCharacter->CombatComp->IsAiming();
		}
	}
}
