#include "AN_ThrowOut.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"

void UAN_ThrowOut::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
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

		// if (HumanCharacter->HasAuthority())
		// {
		// 	UE_LOG(LogTemp, Warning, TEXT("Notify HasAuthority"));
		// }
		// else
		// {
		// 	UE_LOG(LogTemp, Warning, TEXT("Notify"));
		// }

		// 在服务端播放投掷动画时，动画通知和DetachFromComponent同时到达客户端，客户端动画通知不触发
		if (HumanCharacter->IsLocallyControlled())
		{
			HumanCharacter->CombatComponent->ServerThrowOut();
			HumanCharacter->CombatComponent->ThrowOut();
		}
	}
}
