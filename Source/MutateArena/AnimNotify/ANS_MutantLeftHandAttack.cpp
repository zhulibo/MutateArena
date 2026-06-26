#include "ANS_MutantLeftHandAttack.h"

#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/Characters/MutantCharacter.h"

void UANS_MutantLeftHandAttack::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AMutantCharacter* MutantCharacter = Cast<AMutantCharacter>(MeshComp->GetOwner()))
	{
		MutantCharacter->LeftHandAttackBegin();
	}
}

void UANS_MutantLeftHandAttack::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
}

void UANS_MutantLeftHandAttack::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AMutantCharacter* MutantCharacter = Cast<AMutantCharacter>(MeshComp->GetOwner()))
	{
		MutantCharacter->LeftHandAttackEnd();
	}
}
