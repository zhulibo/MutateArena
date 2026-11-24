#include "CombatComponent.h"

#include "AbilitySystemComponent.h"
#include "RecoilComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "MutateArena/Characters/AnimInstHuman.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "MutateArena/Equipments/Melee.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MetaSoundSource.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Equipments/Throwable.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/GameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "MutateArena/Characters/AnimInstHuman1P_Layer.h"
#include "MutateArena/Characters/AnimInstHuman_Layer.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	SetIsReplicatedByDefault(true);
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UCombatComponent, Slot1Equipment);
	DOREPLIFETIME(UCombatComponent, Slot2Equipment);
	DOREPLIFETIME(UCombatComponent, Slot3Equipment);
	DOREPLIFETIME(UCombatComponent, Slot4Equipment);
	DOREPLIFETIME(UCombatComponent, CurEquipmentSlot);
	DOREPLIFETIME(UCombatComponent, LastEquipmentSlot);
	DOREPLIFETIME_CONDITION(UCombatComponent, bIsAiming, COND_SkipOwner);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	HumanChar = Cast<AHumanCharacter>(GetOwner());
	
	if (HumanChar)
	{
		if (HumanChar->Camera && DefaultFOV <= 0.f)
		{
			DefaultFOV = HumanChar->Camera->FieldOfView;
		}
	}
}

void UCombatComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	if (HumanChar && (HumanChar->IsLocallyControlled() || HumanChar->bIsCurrentViewTarget))
	{
		UpdateLocalAiming(DeltaSeconds);
	}
}

AEquipment* UCombatComponent::GetCurEquipment() const
{
	return GetEquipmentBySlot(CurEquipmentSlot);
}

AEquipment* UCombatComponent::GetLastEquipment() const
{
	return GetEquipmentBySlot(LastEquipmentSlot);
}

AWeapon* UCombatComponent::GetCurWeapon() const
{
	switch (CurEquipmentSlot)
	{
	case EEquipmentSlot::Slot1:
		return Slot1Equipment;
	case EEquipmentSlot::Slot2:
		return Slot2Equipment;
	default:
		return nullptr;
	}
}

AMelee* UCombatComponent::GetCurMelee() const
{
	switch (CurEquipmentSlot)
	{
	case EEquipmentSlot::Slot3:
		return Slot3Equipment;
	default:
		return nullptr;
	}
}

AEquipment* UCombatComponent::GetEquipmentBySlot(EEquipmentSlot EquipmentSlot) const
{
	switch (EquipmentSlot)
	{
	case EEquipmentSlot::Slot1:
		return Slot1Equipment;
	case EEquipmentSlot::Slot2:
		return Slot2Equipment;
	case EEquipmentSlot::Slot3:
		return Slot3Equipment;
	case EEquipmentSlot::Slot4:
		return Slot4Equipment;
	default:
		return nullptr;
	}
}

bool UCombatComponent::HasEquipmentInSlot(EEquipmentSlot EquipmentSlot) const
{
	return GetEquipmentBySlot(EquipmentSlot) != nullptr;
}

bool UCombatComponent::IsAimingMoveSlowActive() const
{
	if (!bIsAiming || GetCurWeapon() == nullptr)
	{
		return false;
	}

	// 换弹与切枪期间已退出机瞄姿态，双端同步通过预测 Tag 恢复常规移速，避免 CMC 移速预测失步
	if (const UAbilitySystemComponent* ASC = HumanChar ? HumanChar->GetAbilitySystemComponent() : nullptr)
	{
		if (ASC->HasMatchingGameplayTag(TAG_STATE_COMBAT_RELOADING) || ASC->HasMatchingGameplayTag(TAG_STATE_COMBAT_SWAPPING))
		{
			return false;
		}
	}

	return true;
}

void UCombatComponent::OnEndFirstPersonView()
{
	AimingProgress = 0.f;
	if (HumanChar && HumanChar->Camera && DefaultFOV > 0.f)
	{
		HumanChar->Camera->FieldOfView = DefaultFOV;
	}

	if (bPrevEffectiveAiming)
	{
		bPrevEffectiveAiming = false;
		if (UWorld* World = GetWorld())
		{
			if (ULocalPlayer* LocalPlayer = World->GetFirstLocalPlayerFromController())
			{
				if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(LocalPlayer))
				{
					UISubsystem->OnCrosshairHidden.Broadcast(false);
				}
			}
		}
	}
}

void UCombatComponent::OnRep_Slot1Equipment(AWeapon* OldEquipment)
{
	// 只有存在旧武器被替换时，才锁死切换背包（避免开局首次生成装备就把 bCanSwitchLoadout 锁死）
	if (HumanChar && OldEquipment)
	{
		HumanChar->bCanSwitchLoadout = false;
	}

	// 只有当 OldEquipment 存在（原地换枪）时，才走切入动画流程；
	// 开局首次同步（OldEquipment == nullptr）直接交给 GiveEquipment 正常挂载到手上。
	const bool bIsReplacingCurrent = (CurEquipmentSlot == EEquipmentSlot::Slot1 && Slot1Equipment && OldEquipment != nullptr && Slot1Equipment != OldEquipment);
	if (bIsReplacingCurrent && HumanChar && HumanChar->IsLocallyControlled())
	{
		// 客机原地替换当前手持主武器时，先置为 None 让 GiveEquipment 以 Holstered 入库，再通过 SendSwapEquipmentEvent 触发本地 1P 切入动画
		CurEquipmentSlot = EEquipmentSlot::None;
	}

	GiveEquipment(Slot1Equipment);

	if (bIsReplacingCurrent && HumanChar)
	{
		// 捡到补给箱播放切入动画
		if (HumanChar->IsLocallyControlled())
		{
			HumanChar->SendSwapEquipmentEvent(EEquipmentSlot::Slot1);
		}
		else
		{
			// 模拟代理（其他玩家视角）直接切换到新武器挂载
			InstantSwap(EEquipmentSlot::Slot1);
		}
	}
}

void UCombatComponent::OnRep_Slot2Equipment(AWeapon* OldEquipment)
{
	if (HumanChar && OldEquipment)
	{
		HumanChar->bCanSwitchLoadout = false;
	}

	const bool bIsReplacingCurrent = (CurEquipmentSlot == EEquipmentSlot::Slot2 && Slot2Equipment && OldEquipment != nullptr && Slot2Equipment != OldEquipment);
	if (bIsReplacingCurrent && HumanChar && HumanChar->IsLocallyControlled())
	{
		CurEquipmentSlot = EEquipmentSlot::None;
	}

	GiveEquipment(Slot2Equipment);

	if (bIsReplacingCurrent && HumanChar)
	{
		if (HumanChar->IsLocallyControlled())
		{
			HumanChar->SendSwapEquipmentEvent(EEquipmentSlot::Slot2);
		}
		else
		{
			InstantSwap(EEquipmentSlot::Slot2);
		}
	}
}

void UCombatComponent::OnRep_Slot3Equipment(AMelee* OldEquipment)
{
	GiveEquipment(Slot3Equipment);
}

void UCombatComponent::OnRep_Slot4Equipment(AThrowable* OldEquipment)
{
	GiveEquipment(Slot4Equipment);
}

void UCombatComponent::OnRep_CurEquipmentSlot(EEquipmentSlot OldEquipmentSlot)
{
	EEquipmentSlot TargetSlot = CurEquipmentSlot;

	// 先切回旧槽位以便在切换函数中正确收起旧武器
	CurEquipmentSlot = OldEquipmentSlot;

	if (GetEquipmentBySlot(TargetSlot))
	{
		// 保存服务端同步过来的 LastEquipmentSlot，避免被本地 InstantSwap 覆盖
		EEquipmentSlot RepLastSlot = LastEquipmentSlot;
		InstantSwap(TargetSlot);
		LastEquipmentSlot = RepLastSlot;
	}
	else
	{
		// 目标槽位装备实体尚未复制到达客户端，先收起旧装备，保持目标槽位值等待 OnRep_SlotXEquipment 触发挂载
		if (AEquipment* OldEquipment = GetEquipmentBySlot(OldEquipmentSlot))
		{
			HolsterEquipment(OldEquipment);
		}
		CurEquipmentSlot = TargetSlot;
	}
}

void UCombatComponent::GiveEquipment(AEquipment* Equipment)
{
	if (HumanChar == nullptr) HumanChar = Cast<AHumanCharacter>(GetOwner());
	if (Equipment == nullptr || HumanChar == nullptr) return;

	const UEquipmentAsset* EquipAsset = Equipment->EnsureEquipmentAsset();
	if (EquipAsset == nullptr) return;

	const EEquipmentSlot EquipSlot = EquipAsset->GetEquipmentSlot();
	const bool bIsCurrent = (CurEquipmentSlot == EquipSlot);

	// 如果服务端替换的是已有旧武器的槽位，先移除旧武器的 GA
	AEquipment* OldEquipment = GetEquipmentBySlot(EquipSlot);
	if (OldEquipment && OldEquipment != Equipment)
	{
		if (HumanChar->HasAuthority())
		{
			OldEquipment->ClearAbilities(HumanChar->GetAbilitySystemComponent());
		}
	}

	// 1. 槽位指针赋值
	switch (EquipSlot)
	{
	case EEquipmentSlot::Slot1:
		if (AWeapon* TempEquipment = Cast<AWeapon>(Equipment)) Slot1Equipment = TempEquipment;
		break;
	case EEquipmentSlot::Slot2:
		if (AWeapon* TempEquipment = Cast<AWeapon>(Equipment)) Slot2Equipment = TempEquipment;
		break;
	case EEquipmentSlot::Slot3:
		if (AMelee* TempEquipment = Cast<AMelee>(Equipment)) Slot3Equipment = TempEquipment;
		break;
	case EEquipmentSlot::Slot4:
		if (AThrowable* TempEquipment = Cast<AThrowable>(Equipment)) Slot4Equipment = TempEquipment;
		break;
	default:
		break;
	}

	// 2. 转换装备自身为被持有状态（关闭地面物理、设置Owner）
	Equipment->OnGiven(HumanChar);

	// 3. 开局生成时 CurEquipmentSlot 为 None，所有武器均走 else 分支进入已入库状态；
	// 仅当客户端晚收到当前手持武器的 OnRep 或原地替换当前手持槽位武器时，才直接激活挂载。
	if (bIsCurrent)
	{
		AttachToHand(Equipment, Sockets::SuffixR);

		if (HumanChar->HasAuthority())
		{
			Equipment->GiveAbilities(HumanChar->GetAbilitySystemComponent());
		}

		RefreshCurEquipmentState();
	}
	else
	{
		HolsterEquipment(Equipment);
	}
}

void UCombatComponent::HolsterEquipment(AEquipment* Equipment)
{
	if (HumanChar == nullptr) HumanChar = Cast<AHumanCharacter>(GetOwner());
	if (Equipment == nullptr || HumanChar == nullptr) return;

	Equipment->AttachToActor(HumanChar, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	Equipment->SetState(EEquipmentState::Holstered);
}

void UCombatComponent::AttachToHand(AEquipment* Equipment, FName SocketNameSuffix)
{
	if (HumanChar == nullptr) HumanChar = Cast<AHumanCharacter>(GetOwner());
	if (HumanChar == nullptr || Equipment == nullptr || Equipment->EnsureEquipmentAsset() == nullptr) return;
	
	// 先让武器Actor自身附着到角色身上，确保在 AHumanCharacter::SetFirstPersonViewMode 中调用 GetAttachedActors 能找到当前手持的武器！
	Equipment->AttachToActor(HumanChar, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	if (HumanChar->BodyMesh && Equipment->EquipmentMesh)
	{
		FString EquipmentSocketStr = StaticEnum<EEquipmentName>()->GetNameStringByValue(static_cast<int64>(Equipment->EquipmentAsset->GetParentEquipmentName())) + SocketNameSuffix.ToString();
		Equipment->EquipmentMesh->AttachToComponent(HumanChar->BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName(*EquipmentSocketStr));
	}

	if (HumanChar->Mesh1P && Equipment->EquipmentMesh1P)
	{
		Equipment->EquipmentMesh1P->AttachToComponent(HumanChar->Mesh1P, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Sockets::WeaponRoot);
	}

	// 切换为手持状态，并根据持有者是否为当前 ViewTarget 设置 1P/3P 显隐与阴影。
	Equipment->SetState(EEquipmentState::Equipped);

	// 挂载动画层
	if (TSubclassOf<UAnimInstHuman1P_Layer> LayerClass = Equipment->EquipmentAsset->GetAnimInstHuman1P_Layer())
	{
		HumanChar->Mesh1P->LinkAnimClassLayers(LayerClass);
	}
	if (TSubclassOf<UAnimInstHuman_Layer> LayerClass = Equipment->EquipmentAsset->GetAnimInstHuman_Layer())
	{
		HumanChar->BodyMesh->LinkAnimClassLayers(LayerClass);
	}
}

void UCombatComponent::RefreshCurEquipmentState()
{
	if (!HumanChar) return;

	if (BaseController == nullptr) BaseController = Cast<ABaseController>(HumanChar->GetController());
	if (HumanChar->IsLocallyControlled() && BaseController)
	{
		if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
		{
			const AWeapon* CurWeapon = GetCurWeapon();
			UISubsystem->OnAmmoChange.Broadcast(CurWeapon ? CurWeapon->Ammo : 0);
			UISubsystem->OnCarriedAmmoChange.Broadcast(CurWeapon ? CurWeapon->CarriedAmmo : 0);
			UISubsystem->OnCrosshairHidden.Broadcast(bIsAiming && CurWeapon != nullptr);
		}
	}

	HumanChar->UpdateMaxWalkSpeed();
}

void UCombatComponent::ClientPlayEquipSound_Implementation()
{
	if (HumanChar)
	{
		if (AssetSubsystem == nullptr) AssetSubsystem = HumanChar->GetGameInstance()->GetSubsystem<UAssetSubsystem>();
		if (AssetSubsystem && AssetSubsystem->EquipmentCommonAsset)
		{
			UGameplayStatics::PlaySound2D(this, AssetSubsystem->EquipmentCommonAsset->EquipSound);
		}
	}
}

void UCombatComponent::InstantSwap(EEquipmentSlot TargetSlot)
{
	UE_LOG(LogTemp, Warning, TEXT("InstantSwap"));

	if (TargetSlot == CurEquipmentSlot) return;

	if (HumanChar == nullptr) HumanChar = Cast<AHumanCharacter>(GetOwner());
	AEquipment* TargetEquipment = GetEquipmentBySlot(TargetSlot);
	if (!HumanChar || !TargetEquipment) return;

	AEquipment* OldCurEquipment = GetCurEquipment();

	// 移除旧装备的GA
	if (HumanChar->HasAuthority() && OldCurEquipment)
	{
		OldCurEquipment->ClearAbilities(HumanChar->GetAbilitySystemComponent());
	}

	if (OldCurEquipment)
	{
		HolsterEquipment(OldCurEquipment);
		LastEquipmentSlot = CurEquipmentSlot;
	}
	else if (CurEquipmentSlot != EEquipmentSlot::None)
	{
		// 若为丢枪导致当前槽位变空后自动切枪，则将 LastEquipmentSlot 回退为近战武器
		LastEquipmentSlot = EEquipmentSlot::Slot3;
	}

	// 切枪时同步取消瞄准 GA 并清空残留后坐力，避免 UGA_Equipment_Aim 悬空激活及跨武器残留压枪拉扯
	if (UAbilitySystemComponent* ASC = HumanChar->GetAbilitySystemComponent())
	{
		FGameplayTagContainer CancelTags(TAG_ABILITY_EQUIPMENT_AIM);
		ASC->CancelAbilities(&CancelTags);
	}
	if (HumanChar->RecoilComp)
	{
		HumanChar->RecoilComp->ResetRecoil();
	}

	CurEquipmentSlot = TargetSlot;
	bIsAiming = false;
	bPrevEffectiveAiming = false;

	AttachToHand(TargetEquipment, Sockets::SuffixR);

	// 添加新装备的GA
	if (HumanChar->HasAuthority())
	{
		TargetEquipment->GiveAbilities(HumanChar->GetAbilitySystemComponent());
	}
	
	RefreshCurEquipmentState();
}

void UCombatComponent::UpdateLocalAiming(float DeltaSeconds)
{
	if (!HumanChar || (!HumanChar->IsLocallyControlled() && !HumanChar->bIsCurrentViewTarget)) return;

	if (HumanChar->Camera && DefaultFOV <= 0.f)
	{
		DefaultFOV = HumanChar->Camera->FieldOfView;
	}

	const AWeapon* CurWeapon = GetCurWeapon();
	const UAnimInstance* CharAnimInst = (HumanChar->bIsCurrentViewTarget && HumanChar->Mesh1P)
		? HumanChar->Mesh1P->GetAnimInstance()
		: (HumanChar->BodyMesh ? HumanChar->BodyMesh->GetAnimInstance() : nullptr);
	const bool bIsPlayingActionMontage = CharAnimInst && CharAnimInst->IsAnyMontagePlaying();

	// 播放换弹、切枪、拉栓等动作蒙太奇期间自动暂退镜头缩放与准星隐藏，与 UAnimInstHumanBase 严格同源
	const bool bEffectiveAiming = bIsAiming && CurWeapon && CurWeapon->FragWeapon && !bIsPlayingActionMontage;
	if (bEffectiveAiming != bPrevEffectiveAiming)
	{
		bPrevEffectiveAiming = bEffectiveAiming;
		if (BaseController == nullptr) BaseController = Cast<ABaseController>(HumanChar->GetController());
		ULocalPlayer* LocalPlayer = BaseController
			? BaseController->GetLocalPlayer()
			: (GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr);
		if (LocalPlayer)
		{
			if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(LocalPlayer))
			{
				UISubsystem->OnCrosshairHidden.Broadcast(bEffectiveAiming);
			}
		}
	}

	if (bEffectiveAiming)
	{
		CachedAimFOVMul = CurWeapon->FragWeapon->AimingFOVMul;
		CachedADSTime = FMath::Max(0.01f, CurWeapon->FragWeapon->ADSTime);
	}

	const float TargetProgress = bEffectiveAiming ? 1.f : 0.f;
	if (AimingProgress != TargetProgress)
	{
		const float AimStep = DeltaSeconds / FMath::Max(0.01f, CachedADSTime);
		AimingProgress = FMath::Clamp(AimingProgress + (bEffectiveAiming ? AimStep : -AimStep), 0.f, 1.f);

		if (HumanChar->Camera && DefaultFOV > 0.f)
		{
			const float SmoothAlpha = FMath::InterpEaseInOut(0.f, 1.f, AimingProgress, 2.f);
			HumanChar->Camera->FieldOfView = FMath::Lerp(DefaultFOV, DefaultFOV * CachedAimFOVMul, SmoothAlpha);
		}
	}
}

void UCombatComponent::ServerDestroyEquipments_Implementation(bool bIsDestroySlot1)
{
	if (!HumanChar || !HumanChar->HasAuthority()) return;

	UAbilitySystemComponent* ASC = HumanChar->GetAbilitySystemComponent();

	auto DestroySlot = [ASC](auto*& EquipPtr)
	{
		if (EquipPtr)
		{
			EquipPtr->ClearAbilities(ASC);
			EquipPtr->Destroy();
			EquipPtr = nullptr;
		}
	};

	if (bIsDestroySlot1)
	{
		DestroySlot(Slot1Equipment);
	}
	DestroySlot(Slot2Equipment);
	DestroySlot(Slot3Equipment);
	DestroySlot(Slot4Equipment);

	if (HumanChar->RecoilComp)
	{
		HumanChar->RecoilComp->ResetRecoil();
	}

	CurEquipmentSlot = EEquipmentSlot::None;
	LastEquipmentSlot = EEquipmentSlot::None;
	bIsAiming = false;
	bPrevEffectiveAiming = false;
}

void UCombatComponent::SetAttackCollisionEnabled(bool bIsEnabled)
{
	if (Slot3Equipment)
	{
		Slot3Equipment->SetAttackCollisionEnabled(bIsEnabled);
	}
}
