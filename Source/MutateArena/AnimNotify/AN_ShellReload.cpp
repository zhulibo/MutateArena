#include "AN_ShellReload.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"

void UAN_ShellReload::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(MeshComp->GetOwner());
	if (HumanCharacter && HumanCharacter->CombatComponent)
	{
		// HACK 服务端同一角色短时间触发两次，直接退出
		if (HumanCharacter->HasAuthority())
		{
			if (LastUniqueID == HumanCharacter->GetUniqueID() && FPlatformTime::Seconds() - LastTriggerTime < 0.02)
			{
				return;
			}

			LastUniqueID = HumanCharacter->GetUniqueID();
			LastTriggerTime = FPlatformTime::Seconds();
		}

		UE_LOG(LogTemp, Warning, TEXT("------------------------------------------ %d"), HumanCharacter->GetLocalRole());

		HumanCharacter->CombatComponent->ShellReload();
	}
}
