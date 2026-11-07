#include "AnimInstHuman_Layer.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"

void UAnimInstHuman_Layer::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	HumanChar = Cast<AHumanCharacter>(TryGetPawnOwner());
	if (HumanChar)
	{
		CombatComp = HumanChar->CombatComp;
		MovementComp = HumanChar->MovementComp;
	}
}

void UAnimInstHuman_Layer::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (HumanChar == nullptr) HumanChar = Cast<AHumanCharacter>(TryGetPawnOwner());
	if (CombatComp == nullptr) CombatComp = HumanChar ? HumanChar->CombatComp : nullptr;
	if (MovementComp == nullptr) MovementComp = HumanChar ? HumanChar->MovementComp : nullptr;
}

void UAnimInstHuman_Layer::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (!HumanChar || !CombatComp || !MovementComp) return;
	
	AimPitch = HumanChar->AimPitch;
}
