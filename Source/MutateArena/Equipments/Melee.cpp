#include "Melee.h"
#include "DataRegistrySubsystem.h"
#include "DrawDebugHelpers.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/DamageTypeMelee.h"
#include "Kismet/GameplayStatics.h"
#include "Herbs/Herb.h"
#include "Net/UnrealNetwork.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "MetasoundSource.h"
#include "Data/EquipmentAsset.h"
#include "Engine/GameInstance.h"
#include "MutateArena/System/AssetSubsystem.h"

AMelee::AMelee()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMelee::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bIsLightAttack);
}

void AMelee::BeginPlay()
{
	Super::BeginPlay();

	FString EnumValue = StaticEnum<EEquipmentName>()->GetNameStringByValue(static_cast<int64>(EquipmentParentName));
	FDataRegistryId DataRegistryId(DR_MELEE_DATA, FName(EnumValue));
	if (const FMeleeData* MeleeData = UDataRegistrySubsystem::Get()->GetCachedItem<FMeleeData>(DataRegistryId))
	{
		LightAttackDamage = MeleeData->LightAttackDamage;
		HeavyAttackDamage = MeleeData->HeavyAttackDamage;
		WalkSpeedMul = MeleeData->WalkSpeedMul;
		TraceSockets = MeleeData->TraceSockets;
		TraceRadius = MeleeData->TraceRadius;
	}

	SetMeleeTraceObjectTypes();
}

void AMelee::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsAttacking && EquipmentMesh)
	{
		// 对每个插槽进行上一帧位置到当前帧位置的球体扫掠检测
		for (const FName& SocketName : TraceSockets)
		{
			FVector CurrentLocation = EquipmentMesh->GetSocketLocation(SocketName);

			if (PreviousSocketLocations.Contains(SocketName))
			{
				FVector PreviousLocation = PreviousSocketLocations[SocketName];

				// 计算这一帧扫掠的方向（挥刀方向）
				FVector TraceDirection = (CurrentLocation - PreviousLocation).GetSafeNormal();

				FHitResult HitResult;
				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActor(this);
				QueryParams.AddIgnoredActor(GetOwner());
				QueryParams.bTraceComplex = true;
				// 必须开启此项，否则下方 GetSurfaceType 将始终返回默认材质
				QueryParams.bReturnPhysicalMaterial = true;

				// 创建球体检测形状
				FCollisionShape SphereShape = FCollisionShape::MakeSphere(TraceRadius);

				// 使用 SweepSingleByObjectType 进行球体连线扫掠
				bool bHit = GetWorld()->SweepSingleByObjectType(
					HitResult,
					PreviousLocation,
					CurrentLocation,
					FQuat::Identity,
					MeleeObjectQueryParams,
					SphereShape,
					QueryParams
				);

				// 绘制胶囊体来可视化扫掠轨迹
				// FVector TraceVector = CurrentLocation - PreviousLocation;
				// float TraceDistance = TraceVector.Size();
				// if (TraceDistance > UE_KINDA_SMALL_NUMBER)
				// {
				//    FVector Center = PreviousLocation + TraceVector * 0.5f;
				//    float HalfHeight = (TraceDistance * 0.5f) + TraceRadius;
				//    FQuat CapsuleRot = FQuat::FindBetweenNormals(FVector::UpVector, TraceVector.GetSafeNormal());
				//    DrawDebugCapsule(GetWorld(), Center, HalfHeight, TraceRadius, CapsuleRot, FColor::Red, false, 10.f, 0, 1.f);
				// }

				if (bHit)
				{
					ProcessMeleeHit(HitResult, TraceDirection);
				}
			}

			// 更新上一帧位置为当前帧，供下一帧使用
			PreviousSocketLocations.Add(SocketName, CurrentLocation);
		}
	}
}

void AMelee::OnEquip(class AHumanCharacter* HumanChar)
{
	Super::OnEquip(HumanChar);

	SetMeleeTraceObjectTypes();
}

void AMelee::SetMeleeTraceObjectTypes()
{
	MeleeObjectQueryParams = FCollisionObjectQueryParams();

	if (AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(GetOwner()))
	{
		switch (OwnerTeam)
		{
		case ETeam::Team1:
			MeleeObjectQueryParams.AddObjectTypesToQuery(ECC_MESH_TEAM2);
			break;
		case ETeam::Team2:
			MeleeObjectQueryParams.AddObjectTypesToQuery(ECC_MESH_TEAM1);
			break;
		}

		MeleeObjectQueryParams.AddObjectTypesToQuery(ECC_MESH_HREB);
		MeleeObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
		MeleeObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	}
}

void AMelee::OnStartSwapOut()
{
	Super::OnStartSwapOut();

	SetAttackCollisionEnabled(false);
}

void AMelee::SetAttackCollisionEnabled(bool bIsEnabled)
{
	bIsAttacking = bIsEnabled;

	if (bIsAttacking)
	{
		ClearHitEnemies();
		PreviousSocketLocations.Empty();

		// 攻击开始时记录第一帧位置，防止从原点(0,0,0)发出射线
		if (EquipmentMesh)
		{
			for (const FName& SocketName : TraceSockets)
			{
				PreviousSocketLocations.Add(SocketName, EquipmentMesh->GetSocketLocation(SocketName));
			}
		}
	}
}

void AMelee::ProcessMeleeHit(const FHitResult& HitResult, const FVector& TraceDirection)
{
	AActor* OtherActor = HitResult.GetActor();
	UPrimitiveComponent* OtherComp = HitResult.GetComponent();

	AHumanCharacter* InstigatorCharacter = Cast<AHumanCharacter>(GetOwner());
	if (InstigatorCharacter && OtherActor && !HitEnemies.Contains(OtherActor))
	{
		HitEnemies.Add(OtherActor);

		float Damage = bIsLightAttack ? LightAttackDamage : HeavyAttackDamage;

		if (OtherActor->ActorHasTag(TAG_CHARACTER_BASE))
		{
			DropBlood(EquipmentMesh, OtherActor, OtherComp, Damage, HitResult);
		}
		else if (OtherActor->ActorHasTag(TAG_HERB))
		{
			if (AHerb* Herb = Cast<AHerb>(OtherActor))
			{
				Herb->ServerDestroy();
			}
		}
		else if (OtherComp && OtherComp->GetCollisionObjectType() == ECC_WorldStatic)
		{
			// 处理击中墙体/静态网格体的贴花与音效，传入挥刀方向
			SpawnHitWallEffects(HitResult, TraceDirection);
		}

		if (InstigatorCharacter->IsLocallyControlled())
		{
			ServerApplyDamage(OtherActor, InstigatorCharacter, Damage);
		}
	}
}

void AMelee::SpawnHitWallEffects(const FHitResult& HitResult, const FVector& TraceDirection)
{
	if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem && AssetSubsystem->EquipmentAsset)
	{
		// 1. 生成墙壁刀痕贴花
		if (AssetSubsystem->EquipmentAsset->WallMarkDecal)
		{
			// 贴花的投影方向 (向墙壁内部投影，取法线的反方向)
			FVector ProjectionDirection = -HitResult.ImpactNormal;

			// 将挥刀方向投影到墙面上，得到正确的切线（贴花应有的朝向）
			FVector SurfaceTangent = FVector::VectorPlaneProject(TraceDirection, HitResult.ImpactNormal).
				GetSafeNormal();

			// 构建贴花旋转
			FRotator DecalRotation = FRotationMatrix::MakeFromXY(ProjectionDirection, SurfaceTangent).Rotator();

			UGameplayStatics::SpawnDecalAttached(
				AssetSubsystem->EquipmentAsset->WallMarkDecal,
				FVector(10.f, 20.f, 20.f), // 贴花尺寸，可根据需要调整大小
				HitResult.GetComponent(),
				NAME_None,
				HitResult.ImpactPoint,
				DecalRotation,
				EAttachLocation::KeepWorldPosition,
				10.f // 寿命 10 秒
			);
		}

		// 2. 根据物理材质播放对应的击打音效
		UMetaSoundSource* HitSound = nullptr;
		switch (UGameplayStatics::GetSurfaceType(HitResult))
		{
		case EPhysicalSurface::SurfaceType1:
			HitSound = AssetSubsystem->EquipmentAsset->HitWall_Concrete;
			break;
		case EPhysicalSurface::SurfaceType2:
			HitSound = AssetSubsystem->EquipmentAsset->HitWall_Dirt;
			break;
		case EPhysicalSurface::SurfaceType3:
			HitSound = AssetSubsystem->EquipmentAsset->HitWall_Metal;
			break;
		case EPhysicalSurface::SurfaceType4:
			HitSound = AssetSubsystem->EquipmentAsset->HitWall_Wood;
			break;
		default:
			HitSound = AssetSubsystem->EquipmentAsset->HitWall_Concrete;
			break;
		}

		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, HitResult.ImpactPoint);
		}
	}
}

void AMelee::ServerApplyDamage_Implementation(AActor* OtherActor, AHumanCharacter* InstigatorCharacter, float Damage)
{
	if (InstigatorCharacter && OtherActor)
	{
		FVector HitDirection = (OtherActor->GetActorLocation() - InstigatorCharacter->GetActorLocation()).
			GetSafeNormal();

		FHitResult FakeHit;
		FakeHit.ImpactPoint = OtherActor->GetActorLocation();

		UGameplayStatics::ApplyPointDamage(
			OtherActor,
			Damage,
			HitDirection,
			FakeHit,
			InstigatorCharacter->Controller,
			this,
			UDamageTypeMelee::StaticClass()
		);
	}
}

void AMelee::ClearHitEnemies()
{
	HitEnemies.Empty();
}

void AMelee::DropBlood(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, float Damage,
                       const FHitResult& TraceHitResult)
{
	ABaseCharacter* OverlappedCharacter = Cast<ABaseCharacter>(OtherActor);

	if (OverlappedCharacter == nullptr || OverlappedCharacter->BloodEffect == nullptr) return;

	auto BloodEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		OverlappedCharacter->BloodEffect,
		TraceHitResult.ImpactPoint,
		TraceHitResult.ImpactNormal.Rotation()
	);
	if (BloodEffectComponent)
	{
		BloodEffectComponent->SetVariableInt(TEXT("Count"), ULibraryCommon::GetBloodParticleCount(Damage));
		BloodEffectComponent->SetVariableLinearColor(TEXT("Color"), OverlappedCharacter->BloodColor);
	}

	auto BloodSmokeEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		OverlappedCharacter->BloodSmokeEffect,
		TraceHitResult.ImpactPoint,
		TraceHitResult.ImpactNormal.Rotation()
	);
	if (BloodSmokeEffectComponent)
	{
		BloodSmokeEffectComponent->SetVariableLinearColor(TEXT("SmokeColor"), OverlappedCharacter->BloodColor);
	}
}
