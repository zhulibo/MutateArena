#include "CombatComponent.h"

#include "CombatStateType.h"
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
#include "MutateArena/Characters/AnimInstance_Human.h"
#include "MutateArena/Equipments/AnimInstance_Equipment.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "MutateArena/Equipments/Melee.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MetaSoundSource.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "Net/UnrealNetwork.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	CombatState = ECombatState::Ready;

	// 模拟正在使用副武器，以便开局切换到主武器后，LastEquipmentType默认为副武器
	CurEquipmentType = EEquipmentType::Secondary;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (HumanCharacter)
	{
		if (HumanCharacter->Camera)
		{
			DefaultFOV = HumanCharacter->Camera->FieldOfView;
		}
	}
}

void UCombatComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	if (HumanCharacter && HumanCharacter->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshair(HitResult);
		HitTarget = HitResult.ImpactPoint;

		CalcCameraToScopeRotator();

		InterpFOV(DeltaSeconds);
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
		FVector End = Start + Direction * TRACE_LENGTH;
		
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(HumanCharacter);
		Params.AddIgnoredActor(GetCurEquipment());
		GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECollisionChannel::ECC_Visibility);
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
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

bool UCombatComponent::HasEquippedThisTypeEquipment(EEquipmentType EquipmentType)
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

void UCombatComponent::EquipEquipment(AEquipment* Equipment)
{
	LocalEquipEquipment(Equipment);
	ServerEquipEquipment(Equipment);
}

void UCombatComponent::ServerEquipEquipment_Implementation(AEquipment* Equipment)
{
	MulticastEquipEquipment(Equipment);
}

void UCombatComponent::MulticastEquipEquipment_Implementation(AEquipment* Equipment)
{
	if (HumanCharacter && !HumanCharacter->IsLocallyControlled())
	{
		LocalEquipEquipment(Equipment);
	}
}

void UCombatComponent::MulticastEquipEquipment2_Implementation(AEquipment* Equipment)
{
	if (Equipment == nullptr) return;

	// 取消补给箱装备隐藏
	Equipment->EquipmentMesh->SetVisibility(true);

	LocalEquipEquipment(Equipment);

	// 播放装备音效
	if (HumanCharacter && HumanCharacter->IsLocallyControlled())
	{
		if (AssetSubsystem == nullptr) AssetSubsystem = HumanCharacter->GetGameInstance()->GetSubsystem<UAssetSubsystem>();
		if (AssetSubsystem && AssetSubsystem->EquipmentAsset)
		{
			UGameplayStatics::PlaySoundAtLocation(this, AssetSubsystem->EquipmentAsset->EquipSound, HumanCharacter->GetActorLocation());
		}
	}
}

// 直接替换当前正在使用的装备
void UCombatComponent::MulticastReplaceCurEquipment_Implementation(AEquipment* Equipment)
{
	if (Equipment == nullptr) return;

	// 取消补给箱装备隐藏
	Equipment->EquipmentMesh->SetVisibility(true);

	Equipment->SetOwner(HumanCharacter);
	Equipment->OnEquip();

	bIsAiming = false;

	AssignEquipment(Equipment);
	UseEquipment(Equipment);

	// 播放装备音效
	if (HumanCharacter && HumanCharacter->IsLocallyControlled())
	{
		if (AssetSubsystem == nullptr) AssetSubsystem = HumanCharacter->GetGameInstance()->GetSubsystem<UAssetSubsystem>();
		if (AssetSubsystem && AssetSubsystem->EquipmentAsset)
		{
			UGameplayStatics::PlaySoundAtLocation(this, AssetSubsystem->EquipmentAsset->EquipSound, HumanCharacter->GetActorLocation());
		}
	}
}

void UCombatComponent::LocalEquipEquipment(AEquipment* Equipment)
{
	if (Equipment == nullptr || HumanCharacter == nullptr) return;

	Equipment->SetOwner(HumanCharacter);
	Equipment->OnEquip();

	AssignEquipment(Equipment);

	AttachToBodySocket(Equipment);
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

void UCombatComponent::AttachToBodySocket(AEquipment* Equipment)
{
	if (Equipment == nullptr || HumanCharacter == nullptr || HumanCharacter->GetMesh() == nullptr) return;

	FName BodySocketName;
	switch (Equipment->EquipmentType)
	{
	case EEquipmentType::Primary:
		BodySocketName = TEXT("Shoulder_R");
		break;
	case EEquipmentType::Secondary:
		BodySocketName = TEXT("Thigh_R");
		break;
	case EEquipmentType::Melee:
		BodySocketName = TEXT("Shoulder_L");
		break;
	case EEquipmentType::Throwing:
		BodySocketName = TEXT("Thigh_L");
		break;
	}

	if (const USkeletalMeshSocket* BodySocket = HumanCharacter->GetMesh()->GetSocketByName(BodySocketName))
	{
		BodySocket->AttachActor(Equipment, HumanCharacter->GetMesh());
	}
}

void UCombatComponent::SwapEquipment(EEquipmentType EquipmentType)
{
	if (CurEquipmentType != EquipmentType)
	{
		LocalSwapEquipment(EquipmentType);
		ServerSwapEquipment(EquipmentType);
	}
}

void UCombatComponent::ServerSwapEquipment_Implementation(EEquipmentType EquipmentType)
{
	MulticastSwapEquipment(EquipmentType);
}

void UCombatComponent::MulticastSwapEquipment_Implementation(EEquipmentType EquipmentType)
{
	if (HumanCharacter && !HumanCharacter->IsLocallyControlled())
	{
		LocalSwapEquipment(EquipmentType);
	}
}

void UCombatComponent::MulticastSwapEquipment2_Implementation(EEquipmentType EquipmentType)
{
	LocalSwapEquipment(EquipmentType);
}

void UCombatComponent::LocalSwapEquipment(EEquipmentType EquipmentType)
{
	if (AEquipment* NewEquipment = GetEquipmentByType(EquipmentType))
	{
		PreLocalSwapEquipment();

		PlaySwapOutMontage(NewEquipment);
	}
}

void UCombatComponent::PreLocalSwapEquipment()
{
	GetWorld()->GetTimerManager().ClearTimer(LoadNewBulletTimerHandle);

	bCanFire = true;

	bIsAiming = false;

	CombatState = ECombatState::Swapping;

	StartInterpFOV(DefaultFOV, 0.2f);

	AimingProgress = 0.f;
}

// 播放当前装备切出动画
void UCombatComponent::PlaySwapOutMontage(AEquipment* NewEquipment)
{
	// 兼容玩家快速切枪，新旧EquipmentType未及时更新，直接反向播放即可
	if (bIsSwappingOut)
	{
		PlaySwapOutMontage_Reverse(false, NewEquipment);
		
		return;
	}

	bIsSwappingOut = true;
	
	if (GetCurEquipment())
	{
		GetCurEquipment()->OnStartSwapOut();

		if (HumanCharacter == nullptr) return;
		if (HumanAnimInstance == nullptr) HumanAnimInstance = Cast<UAnimInstance_Human>(HumanCharacter->GetMesh()->GetAnimInstance());
		if (HumanAnimInstance)
		{
			// 播放切出时的角色动画
			HumanAnimInstance->Montage_Play(GetCurEquipment()->SwapOutMontage_C);

			// 播放切出时的装备动画
			if (GetCurEquipment()->GetEquipmentAnimInstance())
			{
				GetCurEquipment()->GetEquipmentAnimInstance()->Montage_Play(GetCurEquipment()->SwapOutMontage_E);
			}

			// 缓存要切换的装备
			TargetEquipment = NewEquipment;
		}
	}
	// 投掷装备扔出后切换到上一个武器 / 开局赋予武器时，当前武器为空
	else
	{
		PlaySwapInMontage(false, NewEquipment);
	}
}

// 播放新装备切入动画
void UCombatComponent::PlaySwapInMontage(bool bInterrupted, AEquipment* NewEquipment)
{
	bIsSwappingOut = false;

	if (bInterrupted || NewEquipment == nullptr) return;

	LastEquipmentType = CurEquipmentType;
	CurEquipmentType = NewEquipment->EquipmentType;

	// 切出旧装备
	if (AEquipment* LastEquipment = GetLastEquipment())
	{
		AttachToBodySocket(LastEquipment);
	}

	if (HumanCharacter == nullptr) return;
	if (HumanAnimInstance == nullptr) HumanAnimInstance = Cast<UAnimInstance_Human>(HumanCharacter->GetMesh()->GetAnimInstance());
	if (HumanAnimInstance)
	{
		// 播放切入时的角色动画
		HumanAnimInstance->Montage_Play(NewEquipment->SwapInMontage_C);

		// 播放切入时的装备动画
		if (NewEquipment->GetEquipmentAnimInstance())
		{
			NewEquipment->GetEquipmentAnimInstance()->Montage_Play(NewEquipment->SwapInMontage_E);
		}
	}

	UseEquipment(NewEquipment);
}

// 切出动画未完成，又切回去了
void UCombatComponent::PlaySwapOutMontage_Reverse(bool bInterrupted, AEquipment* NewEquipment)
{
	bIsSwappingOut = false;

	if (HumanCharacter == nullptr) return;
	if (HumanAnimInstance == nullptr) HumanAnimInstance = Cast<UAnimInstance_Human>(HumanCharacter->GetMesh()->GetAnimInstance());
	if (HumanAnimInstance)
	{
		HumanAnimInstance->Montage_SetPlayRate(HumanAnimInstance->GetCurrentActiveMontage(), -1.f);
		
		if (NewEquipment->GetEquipmentAnimInstance())
		{
			NewEquipment->GetEquipmentAnimInstance()->Montage_SetPlayRate(NewEquipment->GetEquipmentAnimInstance()->GetCurrentActiveMontage(), -1.f);
		}
	}
}

void UCombatComponent::FinishSwap()
{
	CombatState = ECombatState::Ready;
}

void UCombatComponent::UseEquipment(AEquipment* Equipment)
{
	if (Equipment == nullptr || HumanCharacter == nullptr) return;

	AttachToRightHand(Equipment);

	// 更新子弹
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(HumanCharacter->Controller);
	if (HumanCharacter->IsLocallyControlled() && BaseController)
	{
		if (GetCurWeapon())
		{
			BaseController->SetHUDAmmo(GetCurWeapon()->Ammo);
			BaseController->SetHUDCarriedAmmo(GetCurWeapon()->CarriedAmmo);
		}
		else
		{
			BaseController->SetHUDAmmo(0);
			BaseController->SetHUDCarriedAmmo(0);
		}

		BaseController->OnCrosshairHidden.Broadcast(bIsAiming);
	}

	HumanCharacter->UpdateMaxWalkSpeed();
}

void UCombatComponent::AttachToRightHand(AEquipment* Equipment)
{
	AttachToHand(Equipment, FString(TEXT("_R")));
}

void UCombatComponent::AttachToLeftHand(AEquipment* Equipment)
{
	AttachToHand(Equipment, FString(TEXT("_L")));
}

void UCombatComponent::AttachToHand(AEquipment* Equipment, FString SocketNameSuffix)
{
	if (HumanCharacter == nullptr || HumanCharacter->GetMesh() == nullptr || Equipment == nullptr || SocketNameSuffix.IsEmpty()) return;

	FString EquipmentName = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(Equipment->EquipmentParentName)) + SocketNameSuffix;

	if (const USkeletalMeshSocket* HandSocket = HumanCharacter->GetMesh()->GetSocketByName(*EquipmentName))
	{
		HandSocket->AttachActor(Equipment, HumanCharacter->GetMesh());
	}
}

void UCombatComponent::SetAiming(bool TempBIsAiming)
{
	if (CombatState != ECombatState::Ready) return;

	LocalSetAiming(TempBIsAiming);
	ServerSetAiming(TempBIsAiming);
}

void UCombatComponent::ServerSetAiming_Implementation(bool TempBIsAiming)
{
	MulticastSetAiming(TempBIsAiming);
}

void UCombatComponent::MulticastSetAiming_Implementation(bool TempBIsAiming)
{
	if (HumanCharacter && !HumanCharacter->IsLocallyControlled())
	{
		LocalSetAiming(TempBIsAiming);
	}
}

void UCombatComponent::LocalSetAiming(bool TempBIsAiming)
{
	if (HumanAnimInstance == nullptr) HumanAnimInstance = Cast<UAnimInstance_Human>(HumanCharacter->GetMesh()->GetAnimInstance());
	if (HumanCharacter == nullptr || GetCurWeapon() == nullptr || HumanAnimInstance == nullptr) return;

	bIsAiming = TempBIsAiming;

	HumanCharacter->UpdateMaxWalkSpeed();

	if (HumanCharacter->IsLocallyControlled()) // TODO 非本地瞄准动画暂时禁用了
	{
		// 角色瞄准动画
		if (bIsAiming) // 瞄准
		{
			if (HumanAnimInstance->Montage_IsPlaying(GetCurWeapon()->ADSMontage_C))
			{
				HumanAnimInstance->Montage_SetPlayRate(GetCurWeapon()->ADSMontage_C, 1.f);

				float TempFOVInterpTime = GetCurWeapon()->ADSMontage_C->GetPlayLength() - HumanAnimInstance->Montage_GetPosition(GetCurWeapon()->ADSMontage_C);
				StartInterpFOV(DefaultFOV * GetCurWeapon()->AimingFOVMul, TempFOVInterpTime);
			}
			else
			{
				HumanAnimInstance->Montage_Play(GetCurWeapon()->ADSMontage_C);
				
				StartInterpFOV(DefaultFOV * GetCurWeapon()->AimingFOVMul, GetCurWeapon()->ADSMontage_C->GetPlayLength());
			}
		}
		else // 取消瞄准
		{
			if (HumanAnimInstance->Montage_IsPlaying(GetCurWeapon()->ADSMontage_C))
			{
				HumanAnimInstance->Montage_SetPlayRate(GetCurWeapon()->ADSMontage_C, -1.f);

				StartInterpFOV(DefaultFOV, HumanAnimInstance->Montage_GetPosition(GetCurWeapon()->ADSMontage_C));
			}
			else
			{
				HumanAnimInstance->Montage_Play(GetCurWeapon()->ADSMontage_C, -1.f);
				HumanAnimInstance->Montage_SetPosition(GetCurWeapon()->ADSMontage_C, GetCurWeapon()->ADSMontage_C->GetPlayLength());
				
				StartInterpFOV(DefaultFOV, GetCurWeapon()->ADSMontage_C->GetPlayLength());
			}
		}

		// 武器瞄准动画
		if (UAnimInstance_Equipment* EquipmentAnimInstance = GetCurEquipment()->GetEquipmentAnimInstance())
		{
			if (bIsAiming) // 瞄准
			{
				if (EquipmentAnimInstance->Montage_IsPlaying(GetCurWeapon()->ADSMontage_E))
				{
					EquipmentAnimInstance->Montage_SetPlayRate(GetCurWeapon()->ADSMontage_E, 1.f);
				}
				else
				{
					EquipmentAnimInstance->Montage_Play(GetCurWeapon()->ADSMontage_E);
				}
			}
			else // 取消瞄准
			{
				if (EquipmentAnimInstance->Montage_IsPlaying(GetCurWeapon()->ADSMontage_E))
				{
					EquipmentAnimInstance->Montage_SetPlayRate(GetCurWeapon()->ADSMontage_E, -1.f);
				}
				else
				{
					EquipmentAnimInstance->Montage_Play(GetCurWeapon()->ADSMontage_E, -1.f);
					EquipmentAnimInstance->Montage_SetPosition(GetCurWeapon()->ADSMontage_E, GetCurWeapon()->ADSMontage_E->GetPlayLength());
				}
			}
		}
	}
	
	if (HumanCharacter->IsLocallyControlled())
	{
		// 打开画中画
		GetCurWeapon()->SetScopeActive(bIsAiming);

		if (BaseController == nullptr) BaseController = Cast<ABaseController>(HumanCharacter->Controller);
		if (BaseController)
		{
			BaseController->OnCrosshairHidden.Broadcast(bIsAiming);
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
	if (HumanCharacter == nullptr || HumanCharacter->Camera == nullptr || !HumanCharacter->IsLocallyControlled()) return;
		
	StartFOV = HumanCharacter->Camera->FieldOfView;
	TargetFOV = TempTargetFOV;
	FOVInterpTime = TempFOVInterpTime;
	CurrentFOVInterpElapsed = 0.f;
	bNeedInterpFOV = true;
}

void UCombatComponent::InterpFOV(float DeltaSeconds)
{
	if (!bNeedInterpFOV) return;
	if (HumanCharacter == nullptr || HumanCharacter->Camera == nullptr) return;
	
	CurrentFOVInterpElapsed += DeltaSeconds;

	float Alpha = 0.0f;
	if (FOVInterpTime > 0.0f)
	{
		Alpha = FMath::Clamp(CurrentFOVInterpElapsed / FOVInterpTime, 0.0f, 1.0f);
	}

	HumanCharacter->Camera->FieldOfView = FMath::Lerp(StartFOV, TargetFOV, Alpha);

	// HACK 计算瞄准进度，子弹偏移受瞄准进度影响，完全进入瞄准状态子弹偏移为0
	if (GetCurWeapon() && HumanAnimInstance && HumanAnimInstance->Montage_IsPlaying(GetCurWeapon()->ADSMontage_C))
	{
		float DiffFOV = DefaultFOV - GetCurWeapon()->AimingFOVMul * DefaultFOV;
		if (DiffFOV != 0.f)
		{
			AimingProgress = FMath::Abs(DefaultFOV - HumanCharacter->Camera->FieldOfView) / DiffFOV;

			// 强制校准末尾值
			if (AimingProgress < 0.1f) AimingProgress = 0.f;
			if (AimingProgress > 0.9f) AimingProgress = 1.f;
			// UE_LOG(LogTemp, Warning, TEXT("AimingProgress %f"), AimingProgress);
		}
	}

	if (FMath::IsNearlyEqual(HumanCharacter->Camera->FieldOfView, TargetFOV))
	{
		HumanCharacter->Camera->FieldOfView = TargetFOV;
		
		bNeedInterpFOV = false; 
	}
}

void UCombatComponent::CalcCameraToScopeRotator()
{
	// double Time1 = FPlatformTime::Seconds();

	if (HumanCharacter == nullptr || GetCurWeapon() == nullptr || !GetCurWeapon()->bIsPIP) return;

	const UCameraComponent* CameraComponent = HumanCharacter->FindComponentByClass<UCameraComponent>();
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
	if (AssetSubsystem == nullptr) AssetSubsystem = HumanCharacter->GetGameInstance()->GetSubsystem<UAssetSubsystem>();
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

void UCombatComponent::StartFire()
{
	bFireButtonPressed = true;

	CurShotCount = 1;
	
	Fire();
}

void UCombatComponent::StopFire()
{
	bFireButtonPressed = false;
}

void UCombatComponent::Fire()
{
	if (HumanCharacter == nullptr || HumanCharacter->RecoilComponent == nullptr || GetCurWeapon() == nullptr) return;

	if (CanFire())
	{
		bCanFire = false;

		// 获取后坐力
		float RecoilVert = HumanCharacter->RecoilComponent->GetCurRecoilVert();
		float RecoilHor = HumanCharacter->RecoilComponent->GetCurRecoilHor();
		FVector2D Spread = HumanCharacter->RecoilComponent->GetCurSpreadVector();

		LocalFire(HitTarget, RecoilVert, RecoilHor, Spread.X, Spread.Y);
		ServerFire(HitTarget, RecoilVert, RecoilHor, Spread.X, Spread.Y);

		// 开火后增加后坐力
		HumanCharacter->RecoilComponent->IncRecoil();

		// 子弹上膛
		GetWorld()->GetTimerManager().SetTimer(LoadNewBulletTimerHandle, this, &ThisClass::LoadNewBulletFinished, GetCurWeapon()->GetFireDelay());
	}
}

bool UCombatComponent::CanFire()
{
	if (!bCanFire || GetCurWeapon() == nullptr || HumanCharacter == nullptr) return false;

	// 子弹耗尽
	if (GetCurWeapon()->IsEmpty())
	{
		// 播放击锤音效
		if (AssetSubsystem == nullptr) AssetSubsystem = HumanCharacter->GetGameInstance()->GetSubsystem<UAssetSubsystem>();
		if (AssetSubsystem && AssetSubsystem->EquipmentAsset)
		{
			UGameplayStatics::PlaySoundAtLocation(this, AssetSubsystem->EquipmentAsset->ClickSound, HumanCharacter->GetActorLocation());
		}

		return false;
	}

	// Shotgun正在上膛时可以开火
	if (CombatState == ECombatState::Reloading && GetCurWeapon()->EquipmentCate == EEquipmentCate::Shotgun)
	{
		return true;
	}

	return CombatState == ECombatState::Ready;
}

// 子弹上膛完成
void UCombatComponent::LoadNewBulletFinished()
{
	bCanFire = true;
	
	// 自动武器持续射击
	if (bFireButtonPressed && GetCurWeapon() && GetCurWeapon()->bIsAutomatic)
	{
		CurShotCount++;

		Fire();
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw)
{
	MulticastFire(TraceHitTarget, RecoilVert, RecoilHor, SpreadPitch, SpreadYaw);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw)
{
	if (HumanCharacter && !HumanCharacter->IsLocallyControlled())
	{
		LocalFire(TraceHitTarget, RecoilVert, RecoilHor, SpreadPitch, SpreadYaw);
	}
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget, float RecoilVert, float RecoilHor, float SpreadPitch, float SpreadYaw)
{
	if (HumanCharacter == nullptr || GetCurWeapon() == nullptr) return;

	PlayFireMontage();
	GetCurWeapon()->Fire(TraceHitTarget, RecoilVert, RecoilHor, SpreadPitch, SpreadYaw);

	if (CombatState == ECombatState::Reloading && GetCurWeapon()->EquipmentCate == EEquipmentCate::Shotgun)
	{
		CombatState = ECombatState::Ready;
	}
}

void UCombatComponent::PlayFireMontage()
{
	if (HumanCharacter == nullptr || GetCurWeapon() == nullptr) return;

	if (HumanAnimInstance == nullptr) HumanAnimInstance = Cast<UAnimInstance_Human>(HumanCharacter->GetMesh()->GetAnimInstance());
	if (HumanAnimInstance)
	{
		UAnimMontage* MontageToPlay = bIsAiming ? GetCurWeapon()->FireADSMontage_C : GetCurWeapon()->FireMontage_C;
		
		// TODO 非本地瞄准动画暂时禁用了
		if (!HumanCharacter->IsLocallyControlled())
		{
			MontageToPlay = GetCurWeapon()->FireMontage_C;
		}
		
		if (HumanAnimInstance->Montage_IsPlaying(GetCurWeapon()->ADSMontage_C))
		{
			// 打断ADSMontage_C会影响FOV，FOV依赖ADSMontage_C的播放进度
			// FAlphaBlendArgs BlendInArgs(0.2f);
			// HumanAnimInstance->Montage_PlayWithBlendIn(MontageToPlay, BlendInArgs);
		}
		else
		{
			HumanAnimInstance->Montage_Play(MontageToPlay);
		}
	}

	if (GetCurWeapon()->GetEquipmentAnimInstance())
	{
		UAnimMontage* MontageToPlay = bIsAiming ? GetCurWeapon()->FireADSMontage_E : GetCurWeapon()->FireMontage_E;
		
		// TODO 非本地瞄准动画暂时禁用了
		if (!HumanCharacter->IsLocallyControlled())
		{
			MontageToPlay = GetCurWeapon()->FireMontage_E;
		}
		GetCurWeapon()->GetEquipmentAnimInstance()->Montage_Play(MontageToPlay);
	}
}

void UCombatComponent::Reload()
{
	if (GetCurWeapon() && GetCurWeapon()->CarriedAmmo > 0 && !GetCurWeapon()->IsFull() && CombatState == ECombatState::Ready)
	{
		LocalReload();
		ServerReload();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	MulticastReload();
}

void UCombatComponent::MulticastReload_Implementation()
{
	if (HumanCharacter && !HumanCharacter->IsLocallyControlled())
	{
		LocalReload();
	}
}

void UCombatComponent::LocalReload()
{
	CombatState = ECombatState::Reloading;
	
	bIsAiming = false;

	// 瞄准状态下换弹，重置行走速度
	if (HumanCharacter)
	{
		HumanCharacter->UpdateMaxWalkSpeed();
	}

	PlayReloadMontage();
}

void UCombatComponent::PlayReloadMontage()
{
	if (HumanCharacter == nullptr) return;
	if (HumanAnimInstance == nullptr) HumanAnimInstance = Cast<UAnimInstance_Human>(HumanCharacter->GetMesh()->GetAnimInstance());
	if (HumanAnimInstance && GetCurWeapon())
	{
		HumanAnimInstance->Montage_Play(GetCurWeapon()->ReloadMontage_C);

		if (GetCurWeapon()->GetEquipmentAnimInstance())
		{
			GetCurWeapon()->GetEquipmentAnimInstance()->Montage_Play(GetCurWeapon()->ReloadMontage_E);
		}
	}
}

void UCombatComponent::FinishReload()
{
	if (HumanCharacter == nullptr || GetCurWeapon() == nullptr) return;

	CombatState = ECombatState::Ready;

	int32 Ammo = GetCurWeapon()->Ammo;
	int32 MagCapacity = GetCurWeapon()->MagCapacity;
	int32 CarriedAmmo = GetCurWeapon()->CarriedAmmo;
	int32 NeedAmmoNum = MagCapacity - Ammo;

	if (CarriedAmmo > NeedAmmoNum)
	{
		GetCurWeapon()->SetAmmo(MagCapacity);
		GetCurWeapon()->SetCarriedAmmo(CarriedAmmo - NeedAmmoNum);
	}
	else
	{
		GetCurWeapon()->SetAmmo(Ammo + CarriedAmmo);
		GetCurWeapon()->SetCarriedAmmo(0);
	}
}

void UCombatComponent::ShellReload()
{
	if (HumanCharacter == nullptr || GetCurWeapon() == nullptr) return;

	GetCurWeapon()->SetAmmo(GetCurWeapon()->Ammo + 1);
	GetCurWeapon()->SetCarriedAmmo(GetCurWeapon()->CarriedAmmo - 1);

	// 装入一发可立即开火
	bCanFire = true;

	// 弹匣装满或没有备弹时结束换弹
	if (GetCurWeapon()->IsFull() || GetCurWeapon()->CarriedAmmo == 0)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	if (HumanCharacter == nullptr) return;
	if (HumanAnimInstance == nullptr) HumanAnimInstance = Cast<UAnimInstance_Human>(HumanCharacter->GetMesh()->GetAnimInstance());
	if (HumanAnimInstance && GetCurWeapon())
	{
		HumanAnimInstance->Montage_JumpToSection(TEXT("ReloadEnd"));

		if (GetCurWeapon()->GetEquipmentAnimInstance())
		{
			GetCurWeapon()->GetEquipmentAnimInstance()->Montage_JumpToSection(TEXT("ReloadEnd"));
		}
	}
}

void UCombatComponent::DropEquipment(EEquipmentType EquipmentType)
{
	LocalDropEquipment(EquipmentType);
	ServerDropEquipment(EquipmentType);
}

void UCombatComponent::ServerDropEquipment_Implementation(EEquipmentType EquipmentType)
{
	MulticastDropEquipment(EquipmentType);

	if (HumanCharacter)
	{
		HumanCharacter->OnServerDropEquipment();
	}
}

void UCombatComponent::MulticastDropEquipment_Implementation(EEquipmentType EquipmentType)
{
	if (HumanCharacter && !HumanCharacter->IsLocallyControlled())
	{
		LocalDropEquipment(EquipmentType);
	}
}

void UCombatComponent::MulticastDropEquipment2_Implementation(EEquipmentType EquipmentType)
{
	LocalDropEquipment(EquipmentType);
}

void UCombatComponent::LocalDropEquipment(EEquipmentType EquipmentType)
{
	if (GetEquipmentByType(EquipmentType))
	{
		GetEquipmentByType(EquipmentType)->Drop();

		switch (EquipmentType)
		{
		case EEquipmentType::Primary:
			PrimaryEquipment = nullptr;
			break;
		case EEquipmentType::Secondary:
			SecondaryEquipment = nullptr;
			break;
		}
	}
}

void UCombatComponent::ServerDestroyEquipments_Implementation()
{
	LocalDestroyEquipments();
}

void UCombatComponent::LocalDestroyEquipments()
{
	if (PrimaryEquipment) PrimaryEquipment->Destroy();
	if (SecondaryEquipment) SecondaryEquipment->Destroy();
	if (MeleeEquipment) MeleeEquipment->Destroy();
	if (ThrowingEquipment) ThrowingEquipment->Destroy();
}

void UCombatComponent::MeleeAttack(ECombatState TempCombatState)
{
	if (CombatState != ECombatState::Ready) return;

	LocalMeleeAttack(TempCombatState);
	ServerMeleeAttack(TempCombatState);
}

void UCombatComponent::ServerMeleeAttack_Implementation(ECombatState TempCombatState)
{
	MulticastMeleeAttack(TempCombatState);
}

void UCombatComponent::MulticastMeleeAttack_Implementation(ECombatState TempCombatState)
{
	if (HumanCharacter && !HumanCharacter->IsLocallyControlled())
	{
		LocalMeleeAttack(TempCombatState);
	}
}

void UCombatComponent::LocalMeleeAttack(ECombatState TempCombatState)
{
	if (HumanCharacter == nullptr) return;
	if (HumanAnimInstance == nullptr) HumanAnimInstance = Cast<UAnimInstance_Human>(HumanCharacter->GetMesh()->GetAnimInstance());
	if (HumanAnimInstance && CurEquipmentType == EEquipmentType::Melee && MeleeEquipment)
	{
		CombatState = TempCombatState;

		if (CombatState == ECombatState::LightAttacking)
		{
			HumanAnimInstance->Montage_Play(MeleeEquipment->LightAttackMontage_C);
			if (MeleeEquipment->GetEquipmentAnimInstance())
			{
				MeleeEquipment->GetEquipmentAnimInstance()->Montage_Play(MeleeEquipment->LightAttackMontage_E);
			}

			bIsCombo = true;

			// 动画结束后，重置状态
			FOnMontageEnded OnMontageEnded;
			OnMontageEnded.BindWeakLambda(this, [this](UAnimMontage* AnimMontage, bool bInterrupted)
			{
				CombatState = ECombatState::Ready;
				MeleeEquipment->ClearHitEnemies();
			
				if (HumanCharacter->IsLocallyControlled() && bIsCombo)
				{
					HumanCharacter->FireButtonPressed(FInputActionValue());
				}
			});
			HumanAnimInstance->Montage_SetEndDelegate(OnMontageEnded, MeleeEquipment->LightAttackMontage_C);
		}
		else if (CombatState == ECombatState::HeavyAttacking)
		{
			HumanAnimInstance->Montage_Play(MeleeEquipment->HeavyAttackMontage_C);
			if (MeleeEquipment->GetEquipmentAnimInstance())
			{
				MeleeEquipment->GetEquipmentAnimInstance()->Montage_Play(MeleeEquipment->HeavyAttackMontage_E);
			}

			// 动画结束后，重置状态
			FOnMontageEnded OnMontageEnded;
			OnMontageEnded.BindWeakLambda(this, [this](UAnimMontage* AnimMontage, bool bInterrupted)
			{
				CombatState = ECombatState::Ready;
				MeleeEquipment->ClearHitEnemies();
			});
			HumanAnimInstance->Montage_SetEndDelegate(OnMontageEnded, MeleeEquipment->HeavyAttackMontage_C);
		}
	}
}

void UCombatComponent::ServerSetIsCombo_Implementation(bool TempBIsCombo)
{
	MulticastSetIsCombo(TempBIsCombo);
}

void UCombatComponent::MulticastSetIsCombo_Implementation(bool TempBIsCombo)
{
	bIsCombo = TempBIsCombo;
}

void UCombatComponent::AttackFirstSectionEnd()
{
	if (bIsCombo)
	{
		if (MeleeEquipment)
		{
			MeleeEquipment->ClearHitEnemies();
		}
	}
	else
	{
		if (HumanAnimInstance == nullptr) HumanAnimInstance = Cast<UAnimInstance_Human>(HumanCharacter->GetMesh()->GetAnimInstance());
		if (HumanAnimInstance)
		{
			HumanAnimInstance->Montage_Stop(.1f,
				CombatState == ECombatState::LightAttacking ? MeleeEquipment->LightAttackMontage_C : MeleeEquipment->HeavyAttackMontage_C);
		}
		if (MeleeEquipment && MeleeEquipment->GetEquipmentAnimInstance())
		{
			MeleeEquipment->GetEquipmentAnimInstance()->Montage_Stop(.1f,
				CombatState == ECombatState::LightAttacking ? MeleeEquipment->LightAttackMontage_E : MeleeEquipment->HeavyAttackMontage_E);
		}
	}
}

void UCombatComponent::SetAttackCollisionEnabled(bool bIsEnabled)
{
	if (MeleeEquipment)
	{
		MeleeEquipment->SetAttackCollisionEnabled(bIsEnabled);
	}
}

void UCombatComponent::Throw()
{
	LocalThrow();
	ServerThrow();
}

void UCombatComponent::ServerThrow_Implementation()
{
	MulticastThrow();
}

void UCombatComponent::MulticastThrow_Implementation()
{
	if (HumanCharacter && !HumanCharacter->IsLocallyControlled())
	{
		LocalThrow();
	}
}

void UCombatComponent::LocalThrow()
{
	if (CombatState == ECombatState::Ready)
	{
		if (HumanAnimInstance == nullptr) HumanAnimInstance = Cast<UAnimInstance_Human>(HumanCharacter->GetMesh()->GetAnimInstance());
		if (HumanAnimInstance && CurEquipmentType == EEquipmentType::Throwing && ThrowingEquipment)
		{
			HumanAnimInstance->Montage_Play(ThrowingEquipment->ThrowMontage_C);

			if (ThrowingEquipment->GetEquipmentAnimInstance())
			{
				ThrowingEquipment->GetEquipmentAnimInstance()->Montage_Play(ThrowingEquipment->ThrowMontage_E);
			}

			CombatState = ECombatState::Throwing;
		}
	}
}

void UCombatComponent::ServerThrowOut_Implementation()
{
	MulticastThrowOut();
}

void UCombatComponent::MulticastThrowOut_Implementation()
{
	if (HumanCharacter && !HumanCharacter->IsLocallyControlled())
	{
		ThrowOut();
	}
}

void UCombatComponent::ThrowOut()
{
	if (ThrowingEquipment)
	{
		ThrowingEquipment->ThrowOut();

		ThrowingEquipment = nullptr;
		CombatState = ECombatState::Ready;

		// 切换至上一个装备
		if (HumanCharacter && HumanCharacter->IsLocallyControlled())
		{
			switch (LastEquipmentType)
			{
			case EEquipmentType::Primary:
				if (PrimaryEquipment) SwapEquipment(EEquipmentType::Primary);
				break;
			case EEquipmentType::Secondary:
				if (SecondaryEquipment) SwapEquipment(EEquipmentType::Secondary);
				break;
			case EEquipmentType::Melee:
				if (MeleeEquipment) SwapEquipment(EEquipmentType::Melee);
				break;
			}
		}
	}
}
