#include "AN_Foot.h"
#include "MutateArena/Characters/BaseCharacter.h"

void UAN_Foot::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(MeshComp->GetOwner()))
	{
		BaseCharacter->PlayFootSound();
	}
}
