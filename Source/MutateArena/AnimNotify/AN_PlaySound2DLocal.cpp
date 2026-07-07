#include "AN_PlaySound2DLocal.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "MetasoundSource.h"

UAN_PlaySound2DLocal::UAN_PlaySound2DLocal()
{
}

void UAN_PlaySound2DLocal::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MetaSound || !MeshComp)
	{
		return;
	}

	if (APawn* OwnerPawn = Cast<APawn>(MeshComp->GetOwner()))
	{
		if (OwnerPawn->IsLocallyControlled())
		{
			UGameplayStatics::PlaySound2D(OwnerPawn, MetaSound, VolumeMultiplier, PitchMultiplier);
		}
	}
}
