#include "AnimInstEquipment1P.h"

#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"

UAnimInstEquipment1P::UAnimInstEquipment1P()
{
}

void UAnimInstEquipment1P::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (Equipment == nullptr) Equipment = Cast<AEquipment>(GetOwningActor());
	if (Equipment == nullptr) return;

	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(Equipment->GetOwner());
	if (HumanCharacter == nullptr || HumanCharacter->CombatComp == nullptr || HumanCharacter->CombatComp->GetCurWeapon() == nullptr) return;

	// 第一人称武器动画本身只在本地玩家客户端存在并渲染
	if (HumanCharacter->IsLocallyControlled()) 
	{
		// if (!Montage_IsPlaying(HumanCharacter->CombatComp->GetCurWeapon()->ADSMontage_E))
		{
			bIsAiming = HumanCharacter->CombatComp->IsAiming();
		}
	}
}