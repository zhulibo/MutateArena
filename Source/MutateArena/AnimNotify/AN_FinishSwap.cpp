#include "AN_FinishSwap.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"

void UAN_FinishSwap::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(MeshComp->GetOwner());
	if (HumanCharacter && HumanCharacter->CombatComponent)
	{
		HumanCharacter->CombatComponent->FinishSwap();
	}
}
