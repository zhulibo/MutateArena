#include "AN_SendGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"

void UAN_SendGameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !MeshComp->GetOwner())
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	AActor* AvatarActor = nullptr;  // 真正挂载表现和动画的角色
	AActor* OptionalActor = nullptr; // 触发事件的附带对象（比如武器）

	// 情况 1：动画是在角色身上播放的 (Character Montage)
	if (ACharacter* CharacterSelf = Cast<ACharacter>(OwnerActor))
	{
		AvatarActor = CharacterSelf;
		OptionalActor = nullptr; // 角色自身触发，通常不需要附带对象
	}
	// 情况 2：动画是在武器/装备上播放的 (Weapon Montage)
	// 装备的 Owner 在 EquipEquipment 时被设置为了 HumanCharacter
	else if (ACharacter* CharacterOwner = Cast<ACharacter>(OwnerActor->GetOwner()))
	{
		AvatarActor = CharacterOwner;
		OptionalActor = OwnerActor; // 把武器本身作为附带对象传过去
	}

	// 只要找到了角色，就可以通过它（或者它关联的 PlayerState）拿到 ASC
	if (AvatarActor)
	{
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AvatarActor);

		if (ASC && EventTag.IsValid())
		{
			FGameplayEventData Payload;
			Payload.EventTag = EventTag;
			Payload.Instigator = AvatarActor;    // 事件发起者始终是角色
			Payload.OptionalObject = OptionalActor; // 如果是武器蒙太奇，这里就是 AWeapon 实例

			// 将事件发送给 ASC
			ASC->HandleGameplayEvent(EventTag, &Payload);
		}
	}
}
