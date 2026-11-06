#include "CombatComponent.h"

#include "AbilitySystemComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
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
#include "Engine/GameViewportClient.h"
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
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	HumanChar = Cast<AHumanCharacter>(GetOwner());
	
	if (HumanChar)
	{
		if (HumanChar->Camera)
		{
			DefaultFOV = HumanChar->Camera->FieldOfView;
		}
	}
}

void UCombatComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	if (HumanChar && HumanChar->IsLocallyControlled())
	{
		InterpFOV(DeltaSeconds);
	}
}

AEquipment* UCombatComponent::GetCurEquipment()
{
	return GetEquipmentBySlot(CurEquipmentSlot);
}

AEquipment* UCombatComponent::GetLastEquipment()
{
	return GetEquipmentBySlot(LastEquipmentSlot);
}

AWeapon* UCombatComponent::GetCurWeapon()
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

AMelee* UCombatComponent::GetCurMelee()
{
	switch (CurEquipmentSlot)
	{
	case EEquipmentSlot::Slot3:
		return Slot3Equipment;
	default:
		return nullptr;
	}
}

AEquipment* UCombatComponent::GetEquipmentBySlot(EEquipmentSlot EquipmentSlot)
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

bool UCombatComponent::HasEquipmentInSlot(EEquipmentSlot EquipmentSlot)
{
	return GetEquipmentBySlot(EquipmentSlot) != nullptr;
}

void UCombatComponent::OnRep_Slot1Equipment(AWeapon* OldEquipment)
{
	if (HumanChar) HumanChar->bCanSwitchLoadout = false;

	// 切枪 捡枪 捡到补给箱，非开局赋予武器
	const bool bIsReplacingCurrent =
		IsValid(OldEquipment) &&
		IsValid(Slot1Equipment) &&
		Slot1Equipment != OldEquipment &&
		CurEquipmentSlot == EEquipmentSlot::Slot1;

	if (bIsReplacingCurrent && HumanChar && HumanChar->IsLocallyControlled())
	{
		// 客机原地替换当前手持主武器
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
			// 模拟代理直接切换到新武器挂载
			InstantSwap(EEquipmentSlot::Slot1);
		}
	}
}

void UCombatComponent::OnRep_Slot2Equipment(AWeapon* OldEquipment)
{
	if (HumanChar) HumanChar->bCanSwitchLoadout = false;

	const bool bIsReplacingCurrent =
		IsValid(OldEquipment) &&
		IsValid(Slot2Equipment) &&
		Slot2Equipment != OldEquipment &&
		CurEquipmentSlot == EEquipmentSlot::Slot2;

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
	
	// 先让武器Actor自身附着到角色身上，确保在 BaseCharacter::SetFirstPersonViewMode 中调用 GetAttachedActors 能找到当前手持的武器！
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
			UISubsystem->OnAmmoChange.Broadcast(GetCurWeapon() ? GetCurWeapon()->Ammo : 0);
			UISubsystem->OnCarriedAmmoChange.Broadcast(GetCurWeapon() ? GetCurWeapon()->CarriedAmmo : 0);
			UISubsystem->OnCrosshairHidden.Broadcast(IsAiming());
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

	CurEquipmentSlot = TargetSlot;

	AttachToHand(TargetEquipment, Sockets::SuffixR);

	// 添加新装备的GA
	if (HumanChar->HasAuthority())
	{
		TargetEquipment->GiveAbilities(HumanChar->GetAbilitySystemComponent());
	}
	
	RefreshCurEquipmentState();
}

bool UCombatComponent::IsAiming()
{
	if (HumanChar && HumanChar->GetAbilitySystemComponent())
	{
		return HumanChar->GetAbilitySystemComponent()->HasMatchingGameplayTag(TAG_STATE_COMBAT_AIMING);
	}
	return false;
}

void UCombatComponent::LocalSetAiming(bool bIsAiming)
{
	if (HumanChar == nullptr) HumanChar = Cast<AHumanCharacter>(GetOwner());
	if (HumanChar == nullptr || HumanChar->BodyMesh == nullptr) return;
	if (AnimInstHuman == nullptr) AnimInstHuman = Cast<UAnimInstHuman>(HumanChar->BodyMesh->GetAnimInstance());
	if (GetCurWeapon() == nullptr || AnimInstHuman == nullptr) return;

	if (HumanChar->IsLocallyControlled()) // TODO 非本地瞄准动画暂时禁用了
	{
		// 角色瞄准动画
		// if (bIsAiming) // 瞄准
		// {
		// 	if (AnimInstHuman->Montage_IsPlaying(GetCurWeapon()->ADSMontage_C))
		// 	{
		// 		AnimInstHuman->Montage_SetPlayRate(GetCurWeapon()->ADSMontage_C, 1.f);
		//
		// 		float TempFOVInterpTime = GetCurWeapon()->ADSMontage_C->GetPlayLength() - AnimInstHuman->Montage_GetPosition(GetCurWeapon()->ADSMontage_C);
		// 		StartInterpFOV(DefaultFOV * GetCurWeapon()->AimingFOVMul, TempFOVInterpTime);
		// 	}
		// 	else
		// 	{
		// 		AnimInstHuman->Montage_Play(GetCurWeapon()->ADSMontage_C);
		// 		
		// 		StartInterpFOV(DefaultFOV * GetCurWeapon()->ADSMontage_C->GetPlayLength());
		// 	}
		// }
		// else // 取消瞄准
		// {
		// 	if (AnimInstHuman->Montage_IsPlaying(GetCurWeapon()->ADSMontage_C))
		// 	{
		// 		AnimInstHuman->Montage_SetPlayRate(GetCurWeapon()->ADSMontage_C, -1.f);
		//
		// 		StartInterpFOV(DefaultFOV, AnimInstHuman->Montage_GetPosition(GetCurWeapon()->ADSMontage_C));
		// 	}
		// 	else
		// 	{
		// 		AnimInstHuman->Montage_Play(GetCurWeapon()->ADSMontage_C, -1.f);
		// 		AnimInstHuman->Montage_SetPosition(GetCurWeapon()->ADSMontage_C, GetCurWeapon()->ADSMontage_C->GetPlayLength());
		// 		
		// 		StartInterpFOV(DefaultFOV, GetCurWeapon()->ADSMontage_C->GetPlayLength());
		// 	}
		// }

		// 武器瞄准动画
		// if (UAnimInstEquipment* EquipmentAnimInst = GetCurEquipment()->GetEquipmentAnimInst())
		// {
		// 	if (bIsAiming) // 瞄准
		// 	{
		// 		if (EquipmentAnimInst->Montage_IsPlaying(GetCurWeapon()->ADSMontage_E))
		// 		{
		// 			EquipmentAnimInst->Montage_SetPlayRate(GetCurWeapon()->ADSMontage_E, 1.f);
		// 		}
		// 		else
		// 		{
		// 			EquipmentAnimInst->Montage_Play(GetCurWeapon()->ADSMontage_E);
		// 		}
		// 	}
		// 	else // 取消瞄准
		// 	{
		// 		if (EquipmentAnimInst->Montage_IsPlaying(GetCurWeapon()->ADSMontage_E))
		// 		{
		// 			EquipmentAnimInst->Montage_SetPlayRate(GetCurWeapon()->ADSMontage_E, -1.f);
		// 		}
		// 		else
		// 		{
		// 			EquipmentAnimInst->Montage_Play(GetCurWeapon()->ADSMontage_E, -1.f);
		// 			EquipmentAnimInst->Montage_SetPosition(GetCurWeapon()->ADSMontage_E, GetCurWeapon()->ADSMontage_E->GetPlayLength());
		// 		}
		// 	}
		// }
		
		if (BaseController == nullptr) BaseController = Cast<ABaseController>(HumanChar->GetController());
		if (BaseController)
		{
			if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
			{
				UISubsystem->OnCrosshairHidden.Broadcast(bIsAiming);
			}
		}
	}
}

void UCombatComponent::StartInterpFOV(float TempTargetFOV, float TempFOVInterpTime)
{
	if (!HumanChar || !HumanChar->Camera || !HumanChar->IsLocallyControlled()) return;

	StartFOV = HumanChar->Camera->FieldOfView;
	TargetFOV = TempTargetFOV;
	FOVInterpTime = TempFOVInterpTime;
	CurrentFOVInterpElapsed = 0.f;
	bNeedInterpFOV = true;
}

void UCombatComponent::InterpFOV(float DeltaSeconds)
{
	if (!bNeedInterpFOV) return;
	if (!HumanChar || !HumanChar->Camera || !HumanChar->IsLocallyControlled()) return;
	
	CurrentFOVInterpElapsed += DeltaSeconds;

	float Alpha = 0.0f;
	if (FOVInterpTime > 0.0f)
	{
		Alpha = FMath::Clamp(CurrentFOVInterpElapsed / FOVInterpTime, 0.0f, 1.0f);
	}

	HumanChar->Camera->FieldOfView = FMath::Lerp(StartFOV, TargetFOV, Alpha);

	// HACK 计算瞄准进度，子弹偏移受瞄准进度影响，完全进入瞄准状态子弹偏移为0
	if (AWeapon* CurWeapon = GetCurWeapon(); CurWeapon && CurWeapon->FragWeapon)
	{
		float DiffFOV = DefaultFOV - CurWeapon->FragWeapon->AimingFOVMul * DefaultFOV;
		if (DiffFOV != 0.f)
		{
			AimingProgress = FMath::Abs(DefaultFOV - HumanChar->Camera->FieldOfView) / DiffFOV;

			// 强制校准末尾值
			if (AimingProgress < 0.1f) AimingProgress = 0.f;
			if (AimingProgress > 0.9f) AimingProgress = 1.f;
			// UE_LOG(LogTemp, Warning, TEXT("AimingProgress %f"), AimingProgress);
		}
	}

	if (FMath::IsNearlyEqual(HumanChar->Camera->FieldOfView, TargetFOV))
	{
		HumanChar->Camera->FieldOfView = TargetFOV;
		
		bNeedInterpFOV = false; 
	}
}

// 检测准星对应游戏世界的物体
void UCombatComponent::TraceUnderCrosshair(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector Position;
	FVector Direction;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation, Position, Direction
	);

	if (bScreenToWorld)
	{
		FVector Start = Position + Direction * 100.f;
		FVector End = Start + Direction * GameConstants::TraceLength;
		
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(HumanChar);
		Params.AddIgnoredActor(GetCurEquipment());
		GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECC::Trace);
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
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

	CurEquipmentSlot = EEquipmentSlot::None;
	LastEquipmentSlot = EEquipmentSlot::None;
}

void UCombatComponent::SetAttackCollisionEnabled(bool bIsEnabled)
{
	if (Slot3Equipment)
	{
		Slot3Equipment->SetAttackCollisionEnabled(bIsEnabled);
	}
}
