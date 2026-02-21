#include "Shell.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "MetaSoundSource.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/Interfaces/ObjectPoolSubsystem.h"

AShell::AShell()
{
	PrimaryActorTick.bCanEverTick = false;

	ShellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShellMesh"));
	SetRootComponent(ShellMesh);

	ShellMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	ShellMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

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
	// 1. 恢复显示与物理状态
	SetActorHiddenInGame(false);
	ShellMesh->SetSimulatePhysics(true);
	ShellMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	ShellMesh->SetNotifyRigidBodyCollision(true); // 恢复声音触发

	// 2. 弹壳刚生成时，避免与角色和武器碰撞
	ShellMesh->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);
	ShellMesh->SetCollisionResponseToChannel(ECC_MESH_TEAM1, ECollisionResponse::ECR_Ignore);
	ShellMesh->SetCollisionResponseToChannel(ECC_MESH_TEAM2, ECollisionResponse::ECR_Ignore);
	ShellMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);

	// 4. 定时器：延迟恢复与角色的碰撞
	GetWorldTimerManager().SetTimer(CollisionDelayTimer, FTimerDelegate::CreateWeakLambda(this, [this]() {
		ShellMesh->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Block);
		ShellMesh->SetCollisionResponseToChannel(ECC_MESH_TEAM1, ECollisionResponse::ECR_Block);
		ShellMesh->SetCollisionResponseToChannel(ECC_MESH_TEAM2, ECollisionResponse::ECR_Block);
		ShellMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECollisionResponse::ECR_Block);
	}), 0.2f, false);

	// 5. 定时器：替代原先的 SetLifeSpan，时间到后回收对象
	float LifeTime = FMath::FRandRange(3.f, 5.f);
	GetWorldTimerManager().SetTimer(LifeSpanTimer, this, &ThisClass::ReturnToPool, LifeTime, false);
}
void AShell::LaunchShell(const FVector& CharacterVelocity)
{
	// 更新初速度
	InitVelocity = CharacterVelocity;
    
	// 叠加角色移速
	ShellMesh->SetPhysicsLinearVelocity(InitVelocity); 
    
	// 施加抛壳的随机推力
	const FVector RandomShell = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(GetActorForwardVector(), 10.f);
	ShellMesh->AddImpulse(RandomShell * 300.f, NAME_None, true);
}
void AShell::OnReturnedToPool()
{
	// 清除所有定时器，防止在池中意外触发
	GetWorldTimerManager().ClearTimer(LifeSpanTimer);
	GetWorldTimerManager().ClearTimer(CollisionDelayTimer);

	// 隐藏模型并关闭物理与碰撞（节省开销）
	SetActorHiddenInGame(true);
	ShellMesh->SetSimulatePhysics(false);
	ShellMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// 清除动量
	ShellMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	ShellMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
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
	FHitResult HitResult;
	FVector Start = GetActorLocation();
	FVector End = Start - FVector(0.f, 0.f, 100.f);

	FCollisionQueryParams Params;
	Params.bReturnPhysicalMaterial = true;

	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, Params);
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
			// 避免重复播放
			ShellMesh->SetNotifyRigidBodyCollision(false);
			
			UGameplayStatics::PlaySoundAtLocation(this, Sound, HitResult.Location);
		}
	}
}
