#include "Shell.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "MetaSoundSource.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/System/AssetSubsystem.h"

AShell::AShell()
{
	PrimaryActorTick.bCanEverTick = false;

	ShellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShellMesh"));
	SetRootComponent(ShellMesh);

	ShellMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	ShellMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	
	// 弹壳刚生成时，避免与角色和武器碰撞
	ShellMesh->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);
	ShellMesh->SetCollisionResponseToChannel(ECC_MESH_TEAM1, ECollisionResponse::ECR_Ignore);
	ShellMesh->SetCollisionResponseToChannel(ECC_MESH_TEAM2, ECollisionResponse::ECR_Ignore);
	ShellMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECollisionResponse::ECR_Ignore);

	ShellMesh->SetSimulatePhysics(true);
	ShellMesh->SetEnableGravity(true);
	ShellMesh->SetNotifyRigidBodyCollision(true);
}

// 使用对象池优化性能
void AShell::BeginPlay()
{
	Super::BeginPlay();

	ShellMesh->OnComponentHit.AddUniqueDynamic(this, &ThisClass::OnHit);

	ShellMesh->SetPhysicsLinearVelocity(InitVelocity); // 叠加角色移速
	const FVector RandomShell = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(GetActorForwardVector(), 10.f);
	ShellMesh->AddImpulse(RandomShell * 300.f, NAME_None, true);

	SetLifeSpan(10.f);

	FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindWeakLambda(this, [this]() {
		ShellMesh->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Block);
		ShellMesh->SetCollisionResponseToChannel(ECC_MESH_TEAM1, ECollisionResponse::ECR_Block);
		ShellMesh->SetCollisionResponseToChannel(ECC_MESH_TEAM2, ECollisionResponse::ECR_Block);
		ShellMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECollisionResponse::ECR_Block);
	});
	GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, .2f, false);
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
