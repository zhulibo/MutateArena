#include "AnimInstance_Equipment.h"

#include "Equipment.h"
#include "Weapon.h"
#include "MutateArena/Characters/AnimInstance_Human.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"

UAnimInstance_Equipment::UAnimInstance_Equipment()
{
	// bUseMultiThreadedAnimationUpdate = false;
}

void UAnimInstance_Equipment::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (Equipment == nullptr) Equipment = Cast<AEquipment>(GetOwningActor());
	if (Equipment == nullptr) return;

	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(Equipment->GetOwner());
	if (HumanCharacter == nullptr || HumanCharacter->CombatComponent == nullptr || HumanCharacter->CombatComponent->GetCurWeapon() == nullptr) return;

	if (HumanCharacter->IsLocallyControlled()) // TODO 非本地瞄准动画暂时禁用了
	{
		if (!Montage_IsPlaying(HumanCharacter->CombatComponent->GetCurWeapon()->ADSMontage_E))
		{
			bIsAiming = HumanCharacter->CombatComponent->bIsAiming;
		}
	}
}
