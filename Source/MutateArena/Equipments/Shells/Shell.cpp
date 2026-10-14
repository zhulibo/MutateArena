#include "Shell.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "MetaSoundSource.h"
#include "TimerManager.h"
#include "Components/StaticMeshComponent.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/ObjectPoolSubsystem.h"

AShell::AShell()
{
	PrimaryActorTick.bCanEverTick = false;

	ShellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShellMesh"));
	SetRootComponent(ShellMesh);

	ShellMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	// 仅开启物理模拟碰撞，省去场景射线查询检测（Query）开销
	ShellMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

	ShellMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	// 只与静态场景碰撞，忽略 ECC_WorldDynamic（防止 50 个弹壳在地面互相碰撞挤压）
	ShellMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECollisionResponse::ECR_Block);

	// 弹壳速度不快，关闭 CCD 可大幅降低 Chaos 物理连续检测开销
	ShellMesh->SetUseCCD(false);
	ShellMesh->SetSimulatePhysics(true);
	ShellMesh->SetEnableGravity(true);
	ShellMesh->SetNotifyRigidBodyCollision(true);

	ShellMesh->OnComponentHit.AddUniqueDynamic(this, &ThisClass::OnHit);
}

void AShell::BeginPlay()
{
	Super::BeginPlay();
}

void AShell::OnSpawnedFromPool()
{
	// 恢复显示与物理状态
	SetActorHiddenInGame(false);
	ShellMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	ShellMesh->SetSimulatePhysics(true);
	ShellMesh->SetNotifyRigidBodyCollision(true); // 恢复声音触发

	// 定时器：替代原先的 SetLifeSpan，时间到后回收对象
	float LifeTime = FMath::FRandRange(3.f, 5.f);
	GetWorldTimerManager().SetTimer(LifeSpanTimer, this, &ThisClass::ReturnToPool, LifeTime, false);
}

void AShell::LaunchShell(const FVector& CharacterVelocity)
{
	// 更新初速度
	InitVelocity = CharacterVelocity;

	// 叠加角色移速并清空旧旋转角速度
	ShellMesh->SetPhysicsLinearVelocity(InitVelocity);
	ShellMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

	// 施加抛壳的随机推力
	const FVector RandomShell = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(GetActorForwardVector(), 10.f);
	ShellMesh->AddImpulse(RandomShell * 300.f, NAME_None, true);
}

void AShell::OnReturnedToPool()
{
	// 清除定时器，防止在池中或被抢占复用后意外触发旧回收逻辑
	GetWorldTimerManager().ClearTimer(LifeSpanTimer);

	// 先清除动量（必须在关闭 SimulatePhysics 之前或同时进行）
	ShellMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	ShellMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

	// 隐藏模型并关闭物理与碰撞（节省开销）
	SetActorHiddenInGame(true);
	ShellMesh->SetSimulatePhysics(false);
	ShellMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AShell::ReturnToPool()
{
	if (UWorld* World = GetWorld())
	{
		if (UObjectPoolSubsystem* PoolSubsystem = World->GetSubsystem<UObjectPoolSubsystem>())
		{
			PoolSubsystem->ReleaseObject(this);
		}
	}
}

void AShell::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 立即关闭碰撞回调通知，确保每颗弹壳落地只触发一次音效与射线检测
	ShellMesh->SetNotifyRigidBodyCollision(false);

	FHitResult HitResult;
	FVector Start = GetActorLocation();
	FVector End = Start - FVector(0.f, 0.f, 100.f);

	FCollisionQueryParams Params;
	Params.bReturnPhysicalMaterial = true;
	Params.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	GetWorld()->LineTraceSingleByObjectType(
		HitResult,
		Start,
		End,
		ObjectQueryParams,
		Params
	);

	if (HitResult.bBlockingHit)
	{
		UMetaSoundSource* Sound = nullptr;
		switch (UGameplayStatics::GetSurfaceType(HitResult))
		{
		case EPhysicalSurface::SurfaceType1:
			Sound = ShellSound_Concrete;
			break;
		case EPhysicalSurface::SurfaceType2:
			Sound = ShellSound_Dirt;
			break;
		case EPhysicalSurface::SurfaceType3:
			Sound = ShellSound_Metal;
			break;
		case EPhysicalSurface::SurfaceType4:
			Sound = ShellSound_Wood;
			break;
		}
		if (Sound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, Sound, HitResult.Location);
		}
	}
}
