#include "CombatComponent.h"

#include "AbilitySystemComponent.h"
#include "InputActionValue.h"
#include "RecoilComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "..\..\Equipments\Throwing.h"
#include "MutateArena/Characters/AnimInstHuman.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "MutateArena/Equipments/Melee.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MetaSoundSource.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/Tags/ProjectTags.h"
#include "Net/UnrealNetwork.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	SetIsReplicatedByDefault(true);

	// 模拟正在使用副武器，以便开局切换到主武器后，LastEquipmentType默认为副武器
	CurEquipmentType = EEquipmentType::Secondary;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UCombatComponent, PrimaryEquipment);
	DOREPLIFETIME(UCombatComponent, SecondaryEquipment);
	DOREPLIFETIME(UCombatComponent, MeleeEquipment);
	DOREPLIFETIME(UCombatComponent, ThrowingEquipment);
	DOREPLIFETIME(UCombatComponent, CurEquipmentType);
	DOREPLIFETIME(UCombatComponent, LastEquipmentType);
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
		CalcCameraToScopeRotator();

		InterpFOV(DeltaSeconds);
	}
}

AEquipment* UCombatComponent::GetCurEquipment()
{
	return GetEquipmentByType(CurEquipmentType);
}

AEquipment* UCombatComponent::GetLastEquipment()
{
	return GetEquipmentByType(LastEquipmentType);
}

AWeapon* UCombatComponent::GetCurWeapon()
{
	switch (CurEquipmentType)
	{
	case EEquipmentType::Primary:
		return PrimaryEquipment;
	case EEquipmentType::Secondary:
		return SecondaryEquipment;
	default:
		return nullptr;
	}
}

class AMelee* UCombatComponent::GetCurMelee()
{
	switch (CurEquipmentType)
	{
	case EEquipmentType::Melee:
		return MeleeEquipment;
	default:
		return nullptr;
	}
}

AEquipment* UCombatComponent::GetEquipmentByType(EEquipmentType EquipmentType)
{
	switch (EquipmentType)
	{
	case EEquipmentType::Primary:
		return PrimaryEquipment;
	case EEquipmentType::Secondary:
		return SecondaryEquipment;
	case EEquipmentType::Melee:
		return MeleeEquipment;
	case EEquipmentType::Throwing:
		return ThrowingEquipment;
	default:
		return nullptr;
	}
}

bool UCombatComponent::HasEquippedEquipment(EEquipmentType EquipmentType)
{
	switch (EquipmentType)
	{
	case EEquipmentType::Primary:
		return PrimaryEquipment != nullptr;
	case EEquipmentType::Secondary:
		return SecondaryEquipment != nullptr;
	case EEquipmentType::Melee:
		return MeleeEquipment != nullptr;
	case EEquipmentType::Throwing:
		return ThrowingEquipment != nullptr;
	default:
		return false;
	}
}

void UCombatComponent::OnRep_PrimaryEquipment()
{
	EquipEquipment(PrimaryEquipment, CurEquipmentType == EEquipmentType::Primary);
}

void UCombatComponent::OnRep_SecondaryEquipment()
{
	EquipEquipment(SecondaryEquipment, CurEquipmentType == EEquipmentType::Secondary);
}

void UCombatComponent::OnRep_MeleeEquipment()
{
	EquipEquipment(MeleeEquipment, CurEquipmentType == EEquipmentType::Melee);
}

void UCombatComponent::OnRep_ThrowingEquipment()
{
	EquipEquipment(ThrowingEquipment, CurEquipmentType == EEquipmentType::Throwing);
}

void UCombatComponent::OnRep_CurEquipmentType(EEquipmentType OldEquipmentType)
{
	EEquipmentType TargetType = CurEquipmentType; 
	
	CurEquipmentType = TargetType;

	if (AEquipment* TargetEquipment = GetEquipmentByType(TargetType))
	{
		// 如果装备实体已经存在，为了让 InstantSwap 的内部检查通过，暂时设回旧值并调用
		CurEquipmentType = OldEquipmentType;
		InstantSwap(TargetType);
	}
}

void UCombatComponent::EquipEquipment(AEquipment* Equipment, bool bIsCurrent)
{
	if (Equipment == nullptr || HumanChar == nullptr) return;

	// 移除旧装备的GA
	if (bIsCurrent && HumanChar->HasAuthority() && GetCurEquipment())
	{
		GetCurEquipment()->ClearAbilities(HumanChar->GetAbilitySystemComponent());
	}

	AssignEquipment(Equipment);
	
	Equipment->OnEquip(HumanChar);

	if (bIsCurrent)
	{
		AttachToHand(Equipment, SOCKET_SUFFIX_R);
	}
	else
	{
		AttachToBody(Equipment);
	}

	// 添加新装备的GA
	if (bIsCurrent && HumanChar->HasAuthority())
	{
		Equipment->GiveAbilities(HumanChar->GetAbilitySystemComponent());
	}
}

void UCombatComponent::AssignEquipment(AEquipment* Equipment)
{
	if (Equipment == nullptr) return;

	switch (Equipment->EquipmentType)
	{
	case EEquipmentType::Primary:
		if (AWeapon* TempEquipment = Cast<AWeapon>(Equipment)) PrimaryEquipment = TempEquipment;
		break;
	case EEquipmentType::Secondary:
		if (AWeapon* TempEquipment = Cast<AWeapon>(Equipment)) SecondaryEquipment = TempEquipment;
		break;
	case EEquipmentType::Melee:
		if (AMelee* TempEquipment = Cast<AMelee>(Equipment)) MeleeEquipment = TempEquipment;
		break;
	case EEquipmentType::Throwing:
		if (AThrowing* TempEquipment = Cast<AThrowing>(Equipment)) ThrowingEquipment = TempEquipment;
		break;
	}
}

void UCombatComponent::ClientPlayEquipSound_Implementation()
{
	if (HumanChar)
	{
		if (AssetSubsystem == nullptr) AssetSubsystem = HumanChar->GetGameInstance()->GetSubsystem<UAssetSubsystem>();
		if (AssetSubsystem && AssetSubsystem->EquipmentAsset)
		{
			UGameplayStatics::PlaySound2D(this, AssetSubsystem->EquipmentAsset->EquipSound);
		}
	}
}

void UCombatComponent::InstantSwap(EEquipmentType TargetType)
{
	// UE_LOG(LogTemp, Warning, TEXT("GetPlayInEditorID %d %d"), UE::GetPlayInEditorID(), GetWorld()->GetOutermost()->GetPIEInstanceID());
	if (TargetType == CurEquipmentType) return;
	
	AEquipment* TargetEquipment = GetEquipmentByType(TargetType);
	if (!HumanChar || !TargetEquipment) return;
	
	// 移除旧装备的GA
	if (HumanChar->HasAuthority() && GetCurEquipment())
	{
		GetCurEquipment()->ClearAbilities(HumanChar->GetAbilitySystemComponent());
	}
	
	AttachToBody(GetCurEquipment());
	
	LastEquipmentType = CurEquipmentType;
	CurEquipmentType = TargetType;
	DesiredEquipmentType = TargetType;
	
	AttachToHand(TargetEquipment, SOCKET_SUFFIX_R);
	
	// 添加新装备的GA
	if (HumanChar->HasAuthority())
	{
		TargetEquipment->GiveAbilities(HumanChar->GetAbilitySystemComponent());
	}
	
	// 更新子弹
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(HumanChar->Controller);
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

void UCombatComponent::AttachToBody(AEquipment* Equipment)
{
	if (Equipment == nullptr || HumanChar == nullptr || HumanChar->GetMesh() == nullptr) return;

	FName BodySocketName;
	switch (Equipment->EquipmentType)
	{
	case EEquipmentType::Primary:
		BodySocketName = SOCKET_SHOULDER_R;
		break;
	case EEquipmentType::Secondary:
		BodySocketName = SOCKET_THIGH_R;
		break;
	case EEquipmentType::Melee:
		BodySocketName = SOCKET_SHOULDER_L;
		break;
	case EEquipmentType::Throwing:
		BodySocketName = SOCKET_THIGH_L;
		break;
	}

	if (const USkeletalMeshSocket* BodySocket = HumanChar->GetMesh()->GetSocketByName(BodySocketName))
	{
		BodySocket->AttachActor(Equipment, HumanChar->GetMesh());
	}
}

void UCombatComponent::AttachToHand(AEquipment* Equipment, FName SocketNameSuffix)
{
	if (HumanChar == nullptr || HumanChar->GetMesh() == nullptr || Equipment == nullptr) return;

	FString EquipmentName = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(Equipment->EquipmentParentName)) + SocketNameSuffix.ToString();

	if (const USkeletalMeshSocket* HandSocket = HumanChar->GetMesh()->GetSocketByName(*EquipmentName))
	{
		HandSocket->AttachActor(Equipment, HumanChar->GetMesh());
	}
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
	if (HumanAnimInst == nullptr) HumanAnimInst = Cast<UAnimInstHuman>(HumanChar->GetMesh()->GetAnimInstance());
	if (HumanChar == nullptr || GetCurWeapon() == nullptr || HumanAnimInst == nullptr) return;

	if (HumanChar->IsLocallyControlled()) // TODO 非本地瞄准动画暂时禁用了
	{
		// 角色瞄准动画
		if (bIsAiming) // 瞄准
		{
			if (HumanAnimInst->Montage_IsPlaying(GetCurWeapon()->ADSMontage_C))
			{
				HumanAnimInst->Montage_SetPlayRate(GetCurWeapon()->ADSMontage_C, 1.f);

				float TempFOVInterpTime = GetCurWeapon()->ADSMontage_C->GetPlayLength() - HumanAnimInst->Montage_GetPosition(GetCurWeapon()->ADSMontage_C);
				StartInterpFOV(DefaultFOV * GetCurWeapon()->AimingFOVMul, TempFOVInterpTime);
			}
			else
			{
				HumanAnimInst->Montage_Play(GetCurWeapon()->ADSMontage_C);
				
				StartInterpFOV(DefaultFOV * GetCurWeapon()->AimingFOVMul, GetCurWeapon()->ADSMontage_C->GetPlayLength());
			}
		}
		else // 取消瞄准
		{
			if (HumanAnimInst->Montage_IsPlaying(GetCurWeapon()->ADSMontage_C))
			{
				HumanAnimInst->Montage_SetPlayRate(GetCurWeapon()->ADSMontage_C, -1.f);

				StartInterpFOV(DefaultFOV, HumanAnimInst->Montage_GetPosition(GetCurWeapon()->ADSMontage_C));
			}
			else
			{
				HumanAnimInst->Montage_Play(GetCurWeapon()->ADSMontage_C, -1.f);
				HumanAnimInst->Montage_SetPosition(GetCurWeapon()->ADSMontage_C, GetCurWeapon()->ADSMontage_C->GetPlayLength());
				
				StartInterpFOV(DefaultFOV, GetCurWeapon()->ADSMontage_C->GetPlayLength());
			}
		}

		// 武器瞄准动画
		if (UAnimInstEquipment* EquipmentAnimInst = GetCurEquipment()->GetEquipmentAnimInst())
		{
			if (bIsAiming) // 瞄准
			{
				if (EquipmentAnimInst->Montage_IsPlaying(GetCurWeapon()->ADSMontage_E))
				{
					EquipmentAnimInst->Montage_SetPlayRate(GetCurWeapon()->ADSMontage_E, 1.f);
				}
				else
				{
					EquipmentAnimInst->Montage_Play(GetCurWeapon()->ADSMontage_E);
				}
			}
			else // 取消瞄准
			{
				if (EquipmentAnimInst->Montage_IsPlaying(GetCurWeapon()->ADSMontage_E))
				{
					EquipmentAnimInst->Montage_SetPlayRate(GetCurWeapon()->ADSMontage_E, -1.f);
				}
				else
				{
					EquipmentAnimInst->Montage_Play(GetCurWeapon()->ADSMontage_E, -1.f);
					EquipmentAnimInst->Montage_SetPosition(GetCurWeapon()->ADSMontage_E, GetCurWeapon()->ADSMontage_E->GetPlayLength());
				}
			}
		}
	}
	
	if (HumanChar->IsLocallyControlled())
	{
		// 打开画中画
		GetCurWeapon()->SetScopeActive(bIsAiming);

		if (BaseController == nullptr) BaseController = Cast<ABaseController>(HumanChar->Controller);
		if (BaseController)
		{
			if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(BaseController->GetLocalPlayer()))
			{
				UISubsystem->OnCrosshairHidden.Broadcast(bIsAiming);
			}
		}
		
		// 调整FOV
		if (bIsAiming)
		{
			StartInterpFOV(DefaultFOV * GetCurWeapon()->AimingFOVMul, 0.2f);
		}
		else
		{
			StartInterpFOV(DefaultFOV, 0.2f);
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
	if (GetCurWeapon() && HumanAnimInst && HumanAnimInst->Montage_IsPlaying(GetCurWeapon()->ADSMontage_C))
	{
		float DiffFOV = DefaultFOV - GetCurWeapon()->AimingFOVMul * DefaultFOV;
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

void UCombatComponent::CalcCameraToScopeRotator()
{
	// double Time1 = FPlatformTime::Seconds();

	if (HumanChar == nullptr || GetCurWeapon() == nullptr || !GetCurWeapon()->bIsPIP) return;

	const UCameraComponent* CameraComponent = HumanChar->FindComponentByClass<UCameraComponent>();
	const USkeletalMeshComponent* EquipmentMesh = GetCurWeapon()->EquipmentMesh;
	if (CameraComponent == nullptr || EquipmentMesh == nullptr) return;

	const FVector CameraLocation = CameraComponent->GetComponentLocation();
	const FVector ScopeStartLocation = EquipmentMesh->GetSocketLocation(SOCKET_SCOPE_START);
	const FVector ScopeEndLocation = EquipmentMesh->GetSocketLocation(SOCKET_SCOPE_END);
	
	FVector WorldToScopeStartVector = ScopeStartLocation - CameraLocation;
	FVector WorldToScopeEndVector = ScopeEndLocation - ScopeStartLocation;
	
	WorldToScopeStartVector.Normalize();
	WorldToScopeEndVector.Normalize();

	// 转换为摄像机本地坐标系
	const FTransform& CameraTransform = CameraComponent->GetComponentTransform();
	const FVector LocalToScopStartVector = CameraTransform.InverseTransformVector(WorldToScopeStartVector);
	const FVector LocalToScopeEndVector = CameraTransform.InverseTransformVector(WorldToScopeEndVector);

	const FQuat RotationQuat = FQuat::FindBetween(LocalToScopStartVector, LocalToScopeEndVector);
	const FRotator CameraToScopeRotator = RotationQuat.Rotator();

	// 镜片效果
	if (AssetSubsystem == nullptr) AssetSubsystem = HumanChar->GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem && AssetSubsystem->EquipmentAsset && AssetSubsystem->EquipmentAsset->MPC_Scope)
	{
		if (UMaterialParameterCollectionInstance* MPCI = GetWorld()->GetParameterCollectionInstance(AssetSubsystem->EquipmentAsset->MPC_Scope))
		{
			const FVector RotatorAsVector = FVector(CameraToScopeRotator.Pitch, CameraToScopeRotator.Yaw, CameraToScopeRotator.Roll);
			MPCI->SetVectorParameterValue(TEXT("CameraToScopeRotator"), RotatorAsVector);
			
			// float Num = FMath::RandRange(0, 100);
			// if (Num < 2)
			// {
			// 	UE_LOG(LogTemp, Warning, TEXT("LocalToScopStartVector %s LocalToScopeEndVector %s CameraToScopeRotator %s"), *LocalToScopStartVector.ToString(), *LocalToScopeEndVector.ToString(), *CameraToScopeRotator.ToString());
			// 	UE_LOG(LogTemp, Warning, TEXT("RotatorAsVector %s"), *RotatorAsVector.ToString());
			// }
		}
	}

	// double Time2 = FPlatformTime::Seconds();
	// UE_LOG(LogTemp, Warning, TEXT("CalcCameraToScopeRotator cost time %f"), Time2 - Time1);
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
		FVector End = Start + Direction * TRACE_LENGTH;
		
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(HumanChar);
		Params.AddIgnoredActor(GetCurEquipment());
		GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECollisionChannel::ECC_Visibility);
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
	}
}

void UCombatComponent::ServerDestroyEquipments_Implementation()
{
	LocalDestroyEquipments();
}

void UCombatComponent::LocalDestroyEquipments(bool bIsDestroyPrimary)
{
	if (PrimaryEquipment && bIsDestroyPrimary) PrimaryEquipment->Destroy();
	if (SecondaryEquipment) SecondaryEquipment->Destroy();
	if (MeleeEquipment) MeleeEquipment->Destroy();
	if (ThrowingEquipment) ThrowingEquipment->Destroy();
}

void UCombatComponent::SetAttackCollisionEnabled(bool bIsEnabled)
{
	if (MeleeEquipment)
	{
		MeleeEquipment->SetAttackCollisionEnabled(bIsEnabled);
	}
}
