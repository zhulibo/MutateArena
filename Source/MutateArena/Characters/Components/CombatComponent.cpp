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

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	SetIsReplicatedByDefault(true);

	// 模拟正在使用副武器，以便开局切换到主武器后，LastEquipmentSlot默认为副武器
	CurEquipmentSlot = EEquipmentSlot::Slot2;
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
	return GetEquipmentByType(CurEquipmentSlot);
}

AEquipment* UCombatComponent::GetLastEquipment()
{
	return GetEquipmentByType(LastEquipmentSlot);
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

class AMelee* UCombatComponent::GetCurMelee()
{
	switch (CurEquipmentSlot)
	{
	case EEquipmentSlot::Slot3:
		return Slot3Equipment;
	default:
		return nullptr;
	}
}

AEquipment* UCombatComponent::GetEquipmentByType(EEquipmentSlot EquipmentSlot)
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

bool UCombatComponent::OwnEquipment(EEquipmentSlot EquipmentSlot)
{
	switch (EquipmentSlot)
	{
	case EEquipmentSlot::Slot1:
		return Slot1Equipment != nullptr;
	case EEquipmentSlot::Slot2:
		return Slot2Equipment != nullptr;
	case EEquipmentSlot::Slot3:
		return Slot3Equipment != nullptr;
	case EEquipmentSlot::Slot4:
		return Slot4Equipment != nullptr;
	default:
		return false;
	}
}

void UCombatComponent::OnRep_Slot1Equipment()
{
	GiveEquipment(Slot1Equipment);
}

void UCombatComponent::OnRep_Slot2Equipment()
{
	GiveEquipment(Slot2Equipment);
}

void UCombatComponent::OnRep_Slot3Equipment()
{
	GiveEquipment(Slot3Equipment);
}

void UCombatComponent::OnRep_Slot4Equipment()
{
	GiveEquipment(Slot4Equipment);
}

void UCombatComponent::OnRep_CurEquipmentSlot(EEquipmentSlot OldEquipmentSlot)
{
	EEquipmentSlot TargetType = CurEquipmentSlot; 
	
	CurEquipmentSlot = TargetType;

	if (AEquipment* TargetEquipment = GetEquipmentByType(TargetType))
	{
		// 如果装备实体已经存在，为了让 InstantSwap 的内部检查通过，暂时设回旧值并调用
		CurEquipmentSlot = OldEquipmentSlot;
		InstantSwap(TargetType);
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

	// 如果替换的是当前手持槽位，且之前该槽位已有旧武器，先移除旧武器的 GA
	AEquipment* OldEquipment = GetEquipmentByType(EquipSlot);
	if (bIsCurrent && HumanChar->HasAuthority() && OldEquipment && OldEquipment != Equipment)
	{
		OldEquipment->ClearAbilities(HumanChar->GetAbilitySystemComponent());
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

	// 2. 转换装备自身为被持有状态（关闭物理、设置Owner）
	Equipment->OnGiven(HumanChar);

	// 3. 根据是否为当前激活槽位，决定挂载到手部还是收起隐藏
	if (bIsCurrent)
	{
		AttachToHand(Equipment, Sockets::SuffixR);

		if (HumanChar->HasAuthority())
		{
			Equipment->GiveAbilities(HumanChar->GetAbilitySystemComponent());
		}

		// 刷新弹药UI与角色移速（解决跨帧OnRep或原地换枪后UI与移速不刷新的问题）
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

	if (Equipment->EquipmentMesh)
	{
		Equipment->EquipmentMesh->SetVisibility(false);
	}
	if (Equipment->EquipmentMesh1P)
	{
		Equipment->EquipmentMesh1P->SetVisibility(false);
	}

	Equipment->AttachToActor(HumanChar, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

void UCombatComponent::AttachToHand(AEquipment* Equipment, FName SocketNameSuffix)
{
	UE_LOG(LogTemp, Warning, TEXT("AttachToHand %d"), Equipment->EquipmentAsset->GetParentEquipmentName());
	if (HumanChar == nullptr) HumanChar = Cast<AHumanCharacter>(GetOwner());
	if (HumanChar == nullptr || Equipment == nullptr || Equipment->EquipmentAsset == nullptr) return;

	if (HumanChar->BodyMesh && Equipment->EquipmentMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("1"));
		FString EquipmentSocketStr = StaticEnum<EEquipmentName>()->GetNameStringByValue(static_cast<int64>(Equipment->EquipmentAsset->GetParentEquipmentName())) + SocketNameSuffix.ToString();
		Equipment->EquipmentMesh->SetVisibility(true);
		Equipment->EquipmentMesh->SetOwnerNoSee(true);
		Equipment->EquipmentMesh->AttachToComponent(HumanChar->BodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName(*EquipmentSocketStr));
	}

	if (HumanChar->Mesh1P && Equipment->EquipmentMesh1P)
	{
		UE_LOG(LogTemp, Warning, TEXT("2"));
		Equipment->EquipmentMesh1P->SetVisibility(true);
		Equipment->EquipmentMesh1P->SetOnlyOwnerSee(true);
		Equipment->EquipmentMesh1P->AttachToComponent(HumanChar->Mesh1P, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Sockets::WeaponRoot);
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

void UCombatComponent::InstantSwap(EEquipmentSlot TargetType)
{
	UE_LOG(LogTemp, Warning, TEXT("InstantSwap"));
	// UE_LOG(LogTemp, Warning, TEXT("GetPlayInEditorID %d %d"), UE::GetPlayInEditorID(), GetWorld()->GetOutermost()->GetPIEInstanceID());
	if (TargetType == CurEquipmentSlot) return;
	
	if (HumanChar == nullptr) HumanChar = Cast<AHumanCharacter>(GetOwner());
	AEquipment* TargetEquipment = GetEquipmentByType(TargetType);
	if (!HumanChar || !TargetEquipment) return;
	
	AEquipment* OldCurEquipment = GetCurEquipment();

	// 移除旧装备的GA
	if (HumanChar->HasAuthority() && OldCurEquipment)
	{
		OldCurEquipment->ClearAbilities(HumanChar->GetAbilitySystemComponent());
	}
	
	HolsterEquipment(OldCurEquipment);
	
	LastEquipmentSlot = CurEquipmentSlot; 
	// 若为丢枪则把 LastEquipmentSlot 置为近战武器
	if (!OldCurEquipment) LastEquipmentSlot = EEquipmentSlot::Slot3;
	
	CurEquipmentSlot = TargetType;
	
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
}

void UCombatComponent::SetAttackCollisionEnabled(bool bIsEnabled)
{
	if (Slot3Equipment)
	{
		Slot3Equipment->SetAttackCollisionEnabled(bIsEnabled);
	}
}