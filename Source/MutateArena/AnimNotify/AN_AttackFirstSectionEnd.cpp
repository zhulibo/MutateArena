#include "AN_AttackFirstSectionEnd.h"

#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/MutantCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"

void UAN_AttackFirstSectionEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp->GetOwner()->ActorHasTag(TAG_CHARACTER_HUMAN))
	{
		AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(MeshComp->GetOwner());
		if (HumanCharacter && HumanCharacter->CombatComponent)
		{
			HumanCharacter->CombatComponent->AttackFirstSectionEnd();
		}
	}
	else if (MeshComp->GetOwner()->ActorHasTag(TAG_CHARACTER_MUTANT))
	{
		if (AMutantCharacter* MutantCharacter = Cast<AMutantCharacter>(MeshComp->GetOwner()))
		{
			MutantCharacter->AttackFirstSectionEnd();
		}
	}
}
