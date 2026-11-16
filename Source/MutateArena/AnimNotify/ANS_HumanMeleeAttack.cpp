#include "ANS_HumanMeleeAttack.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Equipments/Equipment.h"

namespace
{
	// 检查指定蒙太奇中是否已经配置了 UANS_HumanMeleeAttack 通知
	bool MontageHasMeleeAttackNotify(const UAnimMontage* Montage)
	{
		if (!Montage) return false;
		for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
		{
			if (NotifyEvent.NotifyStateClass && NotifyEvent.NotifyStateClass->IsA<UANS_HumanMeleeAttack>())
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * 统一解析通知所属的 HumanCharacter，并判断当前这次通知是否应该生效。
	 * 因为 1P 视角下 3P 网格体仍会在后台播放蒙太奇（用于投射地面阴影），如果 1P/3P 都挂了该通知，
	 * 不做过滤就会互相开关碰撞。规则：
	 * - 3P 视角（看别人）：只认 3P 动画的通知。
	 * - 1P 视角（自己/1P观战）：优先只认 1P 动画的通知；只有当 1P 忘了配通知时，才兜底用 3P 的。
	 */
	AHumanCharacter* ResolveHumanCharacter(USkeletalMeshComponent* MeshComp, bool& bOutShouldHandle)
	{
		bOutShouldHandle = false;
		if (!MeshComp || !MeshComp->GetOwner()) return nullptr;

		// 兼容通知挂在“角色蒙太奇”或“武器蒙太奇”上两种情况
		AActor* OwnerActor = MeshComp->GetOwner();
		AHumanCharacter* HumanChar = Cast<AHumanCharacter>(OwnerActor);
		if (!HumanChar)
		{
			// 如果是武器骨骼触发的，它的 Owner 是武器 Actor，再往上一层才是角色
			HumanChar = Cast<AHumanCharacter>(OwnerActor->GetOwner());
		}
		if (!HumanChar) return nullptr;

		// 判断当前触发通知的骨骼网格体是不是 1P 的（1P 手臂 Mesh1P 或 1P 刀 EquipmentMesh1P）
		const AEquipment* CurEquip = HumanChar->CombatComp ? HumanChar->CombatComp->GetCurEquipment() : nullptr;
		const bool bIs1PMesh = (MeshComp == HumanChar->Mesh1P) || (CurEquip && MeshComp == CurEquip->EquipmentMesh1P);

		// 情况 A：当前是 3P 视角（比如看别的玩家挥刀）-> 只允许 3P 网格体的通知生效，忽略 1P
		if (!HumanChar->bIsCurrentViewTarget)
		{
			bOutShouldHandle = !bIs1PMesh;
			return HumanChar;
		}

		// 情况 B：当前是 1P 视角，且本次就是 1P 网格体触发的 -> 直接允许生效
		if (bIs1PMesh)
		{
			bOutShouldHandle = true;
			return HumanChar;
		}

		// 情况 C：当前是 1P 视角，但本次是 3P 隐藏阴影网格体触发的
		// 检查 1P 手臂和 1P 武器当前播的蒙太奇里有没有配这个通知：都没有配时才用 3P 兜底，配了就忽略 3P 防止打架
		const UAnimInstance* AnimInst1P = HumanChar->Mesh1P ? HumanChar->Mesh1P->GetAnimInstance() : nullptr;
		const UAnimMontage* ActiveMontage1P_C = AnimInst1P ? AnimInst1P->GetCurrentActiveMontage() : nullptr;
		const UAnimInstance* EquipAnimInst1P = (CurEquip && CurEquip->EquipmentMesh1P) ? CurEquip->EquipmentMesh1P->GetAnimInstance() : nullptr;
		const UAnimMontage* ActiveMontage1P_E = EquipAnimInst1P ? EquipAnimInst1P->GetCurrentActiveMontage() : nullptr;

		bOutShouldHandle = !MontageHasMeleeAttackNotify(ActiveMontage1P_C) && !MontageHasMeleeAttackNotify(ActiveMontage1P_E);
		return HumanChar;
	}
}

void UANS_HumanMeleeAttack::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	bool bShouldHandle = false;
	AHumanCharacter* HumanCharacter = ResolveHumanCharacter(MeshComp, bShouldHandle);
	if (bShouldHandle && HumanCharacter && HumanCharacter->CombatComp)
	{
		HumanCharacter->CombatComp->SetAttackCollisionEnabled(true);
	}
}

void UANS_HumanMeleeAttack::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
}

void UANS_HumanMeleeAttack::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	bool bShouldHandle = false;
	AHumanCharacter* HumanCharacter = ResolveHumanCharacter(MeshComp, bShouldHandle);
	if (bShouldHandle && HumanCharacter && HumanCharacter->CombatComp)
	{
		HumanCharacter->CombatComp->SetAttackCollisionEnabled(false);
	}
}
