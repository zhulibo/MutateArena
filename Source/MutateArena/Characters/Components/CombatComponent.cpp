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
#include "MutateArena/Equipments/Data/EquipmentAsset.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	CombatState = ECombatState::Ready;

	// 模拟正在使用副武器，以便开局切换到主武器后，LastEquipmentType默认为副武器
	CurEquipmentType = EEquipmentType::Secondary;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (HumanCharacter)
	{
		HumanCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if (HumanCharacter->Camera)
		{
			DefaultFOV = HumanCharacter->Camera->FieldOfView;
			CurrentFOV = DefaultFOV;
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

		InterpFOV(DeltaSeconds);

		CalcCameraToScopeRotator();
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
		GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECollisionChannel::ECC_Visibility);
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaSeconds)
{
	if (GetUsingWeapon() == nullptr) return;

	if (bIsAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV * GetUsingWeapon()->AimingFOVMul, DeltaSeconds, GetUsingWeapon()->AimSpeed);
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaSeconds, GetUsingWeapon()->AimSpeed);
	}

	if (HumanCharacter && HumanCharacter->Camera)
	{
		HumanCharacter->Camera->SetFieldOfView(CurrentFOV);
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

AWeapon* UCombatComponent::GetUsingWeapon()
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
	if (HumanCharacter == nullptr) return;
	if (AssetSubsystem == nullptr)
	{
		AssetSubsystem = HumanCharacter->GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	}
	if (AssetSubsystem && AssetSubsystem->EquipmentAsset)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AssetSubsystem->EquipmentAsset->EquipSound, HumanCharacter->GetActorLocation());
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
}

// 播放当前装备切出动画
void UCombatComponent::PlaySwapOutMontage(AEquipment* NewEquipment)
{
	// 记录旧装备（不等切出动画完成，适应快速切枪）
	if (NewEquipment)
	{
		LastEquipmentType = CurEquipmentType;
		CurEquipmentType = NewEquipment->EquipmentType;
	}

	if (GetLastEquipment())
	{
		GetLastEquipment()->OnStartSwapOut();

		if (HumanCharacter == nullptr) return;
		if (HumanAnimInstance == nullptr) HumanAnimInstance = Cast<UAnimInstance_Human>(HumanCharacter->GetMesh()->GetAnimInstance());
		if (HumanAnimInstance)
		{
			// 播放切出时的角色动画
			HumanAnimInstance->Montage_Play(GetLastEquipment()->SwapOutMontage_C);

			// 播放切出时的装备动画
			if (GetLastEquipment()->GetEquipmentAnimInstance())
			{
				GetLastEquipment()->GetEquipmentAnimInstance()->Montage_Play(GetLastEquipment()->SwapOutMontage_E);
			}

			// 切出动画播放完后播放切入动画
			// HACK 依赖混出时间
			FOnMontageBlendingOutStarted OnMontageBlendingOutStarted;
			OnMontageBlendingOutStarted.BindWeakLambda(this, [this, NewEquipment](UAnimMontage* AnimMontage, bool bInterrupted)
			{
				PlaySwapInMontage(bInterrupted, NewEquipment);
			});
			// Don't use Montage_SetEndDelegate, if SwapOutMontage ended, the current frame will be the base pose.
			HumanAnimInstance->Montage_SetBlendingOutDelegate(OnMontageBlendingOutStarted, GetLastEquipment()->SwapOutMontage_C);
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
	if (bInterrupted) return;

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

void UCombatComponent::FinishSwap()
{
	CombatState = ECombatState::Ready;
}

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
}

void UCombatComponent::UseEquipment(AEquipment* Equipment)
{
	if (Equipment == nullptr || HumanCharacter == nullptr) return;

	AttachToRightHand(Equipment);

	// 更新子弹
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(HumanCharacter->Controller);
	if (HumanCharacter->IsLocallyControlled() && BaseController)
	{
		if (GetUsingWeapon())
		{
			BaseController->SetHUDAmmo(GetUsingWeapon()->Ammo);
			BaseController->SetHUDCarriedAmmo(GetUsingWeapon()->CarriedAmmo);
		}
		else
		{
			BaseController->SetHUDAmmo(0);
			BaseController->SetHUDCarriedAmmo(0);
		}

		BaseController->OnCrosshairHidden.Broadcast(bIsAiming);
	}

	// 更新玩家速度
	HumanCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * Equipment->MoveSpeedMul;
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
	if (HumanCharacter == nullptr || GetUsingWeapon() == nullptr || HumanAnimInstance == nullptr) return;

	bIsAiming = TempBIsAiming;

	// 移速
	float MoveSpeedMul = bIsAiming ? GetUsingWeapon()->AimMoveSpeedMul : GetUsingWeapon()->MoveSpeedMul;
	HumanCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * MoveSpeedMul;

	// 瞄准镜
	if (GetUsingWeapon()->bHasScope)
	{
		// 本地玩家打开画中画
		if (HumanCharacter->IsLocallyControlled())
		{
			GetUsingWeapon()->SetScopeActive(bIsAiming);
			
			if (BaseController == nullptr) BaseController = Cast<ABaseController>(HumanCharacter->Controller);
			if (BaseController)
			{
				BaseController->OnCrosshairHidden.Broadcast(bIsAiming);
			}
		}

		// 蒙太奇已取消勾选bEnableAutoBlendOut，无论正向或反向播放到末尾，Montage_IsActive返回ture
		// 开镜
		if (bIsAiming)
		{
			if (!HumanAnimInstance->Montage_IsActive(GetUsingWeapon()->ADSMontage_C))
			{
				HumanAnimInstance->Montage_Play(GetUsingWeapon()->ADSMontage_C, 1.f);
			}
			else
			{
				HumanAnimInstance->Montage_Resume(GetUsingWeapon()->ADSMontage_C);
				HumanAnimInstance->Montage_SetPlayRate(GetUsingWeapon()->ADSMontage_C, 1.f);
			}
		}
		// 关镜
		else
		{
			if (!HumanAnimInstance->Montage_IsActive(GetUsingWeapon()->ADSMontage_C))
			{
				// 不知道为什么此处反向播放ADSMontage_C无效
				// HumanAnimInstance->Montage_Play(GetUsingWeapon()->ADSMontage_C, -1.f);
				// 只能把动画反转了
				HumanAnimInstance->Montage_Play(GetUsingWeapon()->ADSReverseMontage_C, 1.f);
			}
			else
			{
				HumanAnimInstance->Montage_Resume(GetUsingWeapon()->ADSMontage_C);
				HumanAnimInstance->Montage_SetPlayRate(GetUsingWeapon()->ADSMontage_C, -1.f);
			}
		}

		// 武器动画其实没有动，不需要做开关镜处理（甚至不需要播放这个动画）
		if (GetUsingWeapon()->GetEquipmentAnimInstance())
		{
			GetUsingWeapon()->GetEquipmentAnimInstance()->Montage_Play(GetUsingWeapon()->ADSMontage_E);
		}
	}
}

void UCombatComponent::CalcCameraToScopeRotator()
{
	// double Time1 = FPlatformTime::Seconds();

	if (HumanCharacter == nullptr || GetUsingWeapon() == nullptr || !GetUsingWeapon()->bHasScope) return;
	UCameraComponent* CameraComponent = HumanCharacter->FindComponentByClass<UCameraComponent>();
	if (CameraComponent == nullptr) return;

	FVector WorldCameraVector = GetUsingWeapon()->EquipmentMesh->GetSocketLocation(TEXT("ScopeSocket")) - HumanCharacter->Camera->GetComponentLocation();
	FVector WorldScopeVector = GetUsingWeapon()->EquipmentMesh->GetSocketLocation(TEXT("ScopeEndSocket")) - GetUsingWeapon()->EquipmentMesh->GetSocketLocation(TEXT("ScopeSocket"));

	WorldCameraVector.Normalize();
	WorldScopeVector.Normalize();

	// 转换为摄像机本地坐标系
	const FTransform& CameraTransform = CameraComponent->GetComponentTransform();
	FVector LocalCameraVector = CameraTransform.InverseTransformVector(WorldCameraVector);
	FVector LocalScopeVector = CameraTransform.InverseTransformVector(WorldScopeVector);
	
	FQuat RotationQuat = FQuat::FindBetween(LocalCameraVector, LocalScopeVector);
	FRotator CameraToScopeRotator = RotationQuat.Rotator();

	// float Num = FMath::RandRange(0, 100);
	// if (Num < 2)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("LocalCameraVector %s LocalScopeVector %s CameraToScopeRotator %s"), *LocalCameraVector.ToString(), *LocalScopeVector.ToString(), *CameraToScopeRotator.ToString());
	// }

	// 镜片效果
	if (AssetSubsystem == nullptr) AssetSubsystem = HumanCharacter->GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem && AssetSubsystem->EquipmentAsset && AssetSubsystem->EquipmentAsset->MPC_Scope)
	{
		if (UMaterialParameterCollectionInstance* MPCI = GetWorld()->GetParameterCollectionInstance(AssetSubsystem->EquipmentAsset->MPC_Scope))
		{
			FVector RotatorAsVector = FVector(CameraToScopeRotator.Pitch, CameraToScopeRotator.Yaw, CameraToScopeRotator.Roll);
			MPCI->SetVectorParameterValue(TEXT("CameraToScopeRotator"), RotatorAsVector);
		}
	}
	
	// double Time2 = FPlatformTime::Seconds();
	// UE_LOG(LogTemp, Warning, TEXT("CalcCameraToScopeRotator cost time %f"), Time2 - Time1);
}

void UCombatComponent::StartFire()
{
	bFireButtonPressed = true;

	bIsFirstShot = true;

	Fire();
}

void UCombatComponent::StopFire()
{
	bFireButtonPressed = false;

	// 重置水平后坐力方向
	if (HumanCharacter && HumanCharacter->RecoilComponent)
	{
		HumanCharacter->RecoilComponent->SetRecoilHorDirection(ERecoilHorDirection::Random);
	}
}

void UCombatComponent::Fire()
{
	if (HumanCharacter == nullptr || HumanCharacter->RecoilComponent == nullptr || GetUsingWeapon() == nullptr) return;

	if (CanFire())
	{
		bCanFire = false;

		// 获取后坐力
		float RecoilVert = HumanCharacter->RecoilComponent->GetCurRecoilVert();
		float RecoilHor = HumanCharacter->RecoilComponent->GetCurRecoilHor();

		LocalFire(HitTarget, RecoilVert, RecoilHor);
		ServerFire(HitTarget, RecoilVert, RecoilHor);

		// 开火后增加后坐力
		HumanCharacter->RecoilComponent->IncRecoil();

		// 子弹上膛
		GetWorld()->GetTimerManager().SetTimer(LoadNewBulletTimerHandle, this, &ThisClass::LoadNewBulletFinished, GetUsingWeapon()->GetFireDelay());
	}
}

bool UCombatComponent::CanFire()
{
	if (!bCanFire || GetUsingWeapon() == nullptr || HumanCharacter == nullptr) return false;

	// 子弹耗尽
	if (GetUsingWeapon()->IsEmpty())
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
	if (CombatState == ECombatState::Reloading && GetUsingWeapon()->EquipmentCate == EEquipmentCate::Shotgun)
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
	if (bFireButtonPressed && GetUsingWeapon() && GetUsingWeapon()->bIsAutomatic)
	{
		if (bIsFirstShot == true) bIsFirstShot = false;

		Fire();
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float RecoilVert, float RecoilHor)
{
	MulticastFire(TraceHitTarget, RecoilVert, RecoilHor);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget, float RecoilVert, float RecoilHor)
{
	if (HumanCharacter && !HumanCharacter->IsLocallyControlled())
	{
		LocalFire(TraceHitTarget, RecoilVert, RecoilHor);
	}
}

void UCombatComponent::LocalFire(const FVector_NetQuantize& TraceHitTarget, float RecoilVert, float RecoilHor)
{
	if (HumanCharacter == nullptr || GetUsingWeapon() == nullptr) return;

	PlayFireMontage();
	GetUsingWeapon()->Fire(TraceHitTarget, RecoilVert, RecoilHor);

	if (CombatState == ECombatState::Reloading && GetUsingWeapon()->EquipmentCate == EEquipmentCate::Shotgun)
	{
		CombatState = ECombatState::Ready;
	}
}

void UCombatComponent::PlayFireMontage()
{
	if (HumanCharacter == nullptr || GetUsingWeapon() == nullptr) return;

	if (HumanAnimInstance == nullptr) HumanAnimInstance = Cast<UAnimInstance_Human>(HumanCharacter->GetMesh()->GetAnimInstance());
	if (HumanAnimInstance)
	{
		HumanAnimInstance->Montage_Play(bIsAiming ? GetUsingWeapon()->FireADSMontage_C : GetUsingWeapon()->FireMontage_C);
	}

	if (GetUsingWeapon()->GetEquipmentAnimInstance())
	{
		GetUsingWeapon()->GetEquipmentAnimInstance()->Montage_Play(bIsAiming ? GetUsingWeapon()->FireADSMontage_E : GetUsingWeapon()->FireMontage_E);
	}
}

void UCombatComponent::Reload()
{
	if (GetUsingWeapon() && GetUsingWeapon()->CarriedAmmo > 0 && !GetUsingWeapon()->IsFull() && CombatState == ECombatState::Ready)
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
	
	PlayReloadMontage();
}

void UCombatComponent::PlayReloadMontage()
{
	if (HumanCharacter == nullptr) return;
	if (HumanAnimInstance == nullptr) HumanAnimInstance = Cast<UAnimInstance_Human>(HumanCharacter->GetMesh()->GetAnimInstance());
	if (HumanAnimInstance && GetUsingWeapon())
	{
		HumanAnimInstance->Montage_Play(GetUsingWeapon()->ReloadMontage_C);

		if (GetUsingWeapon()->GetEquipmentAnimInstance())
		{
			GetUsingWeapon()->GetEquipmentAnimInstance()->Montage_Play(GetUsingWeapon()->ReloadMontage_E);
		}
	}
}

void UCombatComponent::FinishReload()
{
	if (HumanCharacter == nullptr || GetUsingWeapon() == nullptr) return;

	CombatState = ECombatState::Ready;

	int32 Ammo = GetUsingWeapon()->Ammo;
	int32 MagCapacity = GetUsingWeapon()->MagCapacity;
	int32 CarriedAmmo = GetUsingWeapon()->CarriedAmmo;
	int32 NeedAmmoNum = MagCapacity - Ammo;

	if (CarriedAmmo > NeedAmmoNum)
	{
		GetUsingWeapon()->SetAmmo(MagCapacity);
		GetUsingWeapon()->SetCarriedAmmo(CarriedAmmo - NeedAmmoNum);
	}
	else
	{
		GetUsingWeapon()->SetAmmo(Ammo + CarriedAmmo);
		GetUsingWeapon()->SetCarriedAmmo(0);
	}
}

void UCombatComponent::ShellReload()
{
	if (HumanCharacter == nullptr || GetUsingWeapon() == nullptr) return;

	GetUsingWeapon()->SetAmmo(GetUsingWeapon()->Ammo + 1);
	GetUsingWeapon()->SetCarriedAmmo(GetUsingWeapon()->CarriedAmmo - 1);

	// 装入一发可立即开火
	bCanFire = true;

	// 弹匣装满或没有备弹时结束换弹
	if (GetUsingWeapon()->IsFull() || GetUsingWeapon()->CarriedAmmo == 0)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	if (HumanCharacter == nullptr) return;
	if (HumanAnimInstance == nullptr) HumanAnimInstance = Cast<UAnimInstance_Human>(HumanCharacter->GetMesh()->GetAnimInstance());
	if (HumanAnimInstance && GetUsingWeapon())
	{
		HumanAnimInstance->Montage_JumpToSection(TEXT("ReloadEnd"));

		if (GetUsingWeapon()->GetEquipmentAnimInstance())
		{
			GetUsingWeapon()->GetEquipmentAnimInstance()->Montage_JumpToSection(TEXT("ReloadEnd"));
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
		GetEquipmentByType(EquipmentType)->OnDrop();

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
			ServerSetIsCombo(true);

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
	if (HumanCharacter && !HumanCharacter->IsLocallyControlled())
	{
		bIsCombo = TempBIsCombo;
	}
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
			HumanAnimInstance->Montage_Stop(.25,
				CombatState == ECombatState::LightAttacking ? MeleeEquipment->LightAttackMontage_C : MeleeEquipment->HeavyAttackMontage_C);
		}
		if (MeleeEquipment && MeleeEquipment->GetEquipmentAnimInstance())
		{
			MeleeEquipment->GetEquipmentAnimInstance()->Montage_Stop(.25,
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
