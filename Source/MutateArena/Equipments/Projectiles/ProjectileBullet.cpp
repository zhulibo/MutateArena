#include "ProjectileBullet.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/Equipments/Data/DamageTypeEquipment.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "MetaSoundSource.h"
#include "MutateArena/Equipments/Data/EquipmentAsset.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/Utils/LibraryCommon.h"

AProjectileBullet::AProjectileBullet()
{
	CollisionBox->SetBoxExtent(FVector(4.f, 2.f, 2.f));
	CollisionBox->bReturnMaterialOnMove = true;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionBox);
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed = InitialSpeed;
}

void AProjectileBullet::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(LifeTime);
	
	CurrentPenetrationPower = InitialPenetrationPower;
	
    // 对场景物体是碰撞
	CollisionBox->OnComponentHit.AddUniqueDynamic(this, &ThisClass::OnHit);
    // 对角色是重叠，比较方便处理穿透
	CollisionBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnBeginOverlap);
	CollisionBox->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::OnEndOverlap);
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    LifeTime = FMath::FRandRange(4.f, 5.f);
    
    // 子弹是从枪口发出，射向准星在世界场景的投射点，所以穿透后的弹着点会有视差，这是正常的
    FVector ShotDirection = ProjectileMovement->Velocity.GetSafeNormal();
    if (ShotDirection.IsNearlyZero()) ShotDirection = GetActorForwardVector();
    
    // 生成环境受击特效和正面弹孔
    if (ImpactEffect) UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffect, Hit.ImpactPoint, GetActorRotation());
    if (ImpactDecal && OtherComp)
    {
        auto DecalComponent = UGameplayStatics::SpawnDecalAttached(
            ImpactDecal,
            FVector(5.f, 6.f, 6.f),
            OtherComp, NAME_None,
            Hit.ImpactPoint,
            Hit.ImpactNormal.Rotation(),
            EAttachLocation::KeepWorldPosition,
            LifeTime
        );
        if (DecalComponent)
        {
            DecalComponent->SetSortOrder(999);
            DecalComponent->SetFadeOut(LifeTime - 1.f, 1.f, false);
            DecalComponent->SetFadeScreenSize(0.002f);
        }
    }

    // 播放音效与材质判断
    bool bIsWood = false;
    if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
    if (AssetSubsystem && AssetSubsystem->EquipmentAsset && Hit.PhysMaterial.IsValid())
    {
        UMetaSoundSource* ImpactSound = nullptr;
        switch (Hit.PhysMaterial->SurfaceType)
        {
        case EPhysicalSurface::SurfaceType1:
            ImpactSound = AssetSubsystem->EquipmentAsset->ImpactSound_Concrete;
            break;
        case EPhysicalSurface::SurfaceType2:
            ImpactSound = AssetSubsystem->EquipmentAsset->ImpactSound_Dirt;
            break;
        case EPhysicalSurface::SurfaceType3:
            ImpactSound = AssetSubsystem->EquipmentAsset->ImpactSound_Metal;
            break;
        case EPhysicalSurface::SurfaceType4:
            ImpactSound = AssetSubsystem->EquipmentAsset->ImpactSound_Wood;
            bIsWood = true;
            break;
        }
        if (ImpactSound) UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.ImpactPoint);
    }

    // 先测厚度（此时木板碰撞还在）
    bool bPenetrationSuccess = false;
    FHitResult ExitHit;

    if (bIsWood && CurrentPenetrationPower > 0.f && OtherComp)
    {
        FVector TraceStart = Hit.ImpactPoint + (ShotDirection * 200.f);
        FCollisionQueryParams QueryParams;
        QueryParams.bTraceComplex = true;

        if (OtherComp->LineTraceComponent(ExitHit, TraceStart, Hit.ImpactPoint, QueryParams))
        {
            float PenetrationDistance = FVector::Distance(Hit.ImpactPoint, ExitHit.ImpactPoint);
            float Cost = PenetrationDistance * Resistance1cm;
            // UE_LOG(LogTemp, Warning, TEXT("PenetrationDistance %f Cost %f"), PenetrationDistance, Cost);
            
            if (CurrentPenetrationPower > Cost)
            {
                CurrentPenetrationPower -= Cost;
                bPenetrationSuccess = true;
            }
        }
    }

    // 后施加伤害（如果木板碎了，碰撞在这瞬间才会消失）
    if (HasAuthority() && OtherActor && GetInstigator())
    {
        float FlightDistance = FVector::Distance(SpawnLocation, GetActorLocation());
        float FinalDamage = GetDamage(FlightDistance) * CurrentPenetrationPower / InitialPenetrationPower; // TODO 伤害衰减临时方案
        UGameplayStatics::ApplyPointDamage(
            OtherActor,
            FinalDamage,
            ShotDirection,
            Hit,
            GetInstigator()->Controller,
            this,
            UDamageTypeEquipment::StaticClass()
        );
    }

    // 执行穿透或停滞
    if (bPenetrationSuccess)
    {
        // 生成背面出口弹孔
        if (ImpactDecal && ExitHit.GetComponent())
        {
            auto ExitDecalComp = UGameplayStatics::SpawnDecalAttached(
                ImpactDecalExit, 
                FVector(5.f, 6.f, 6.f), 
                ExitHit.GetComponent(), 
                NAME_None,
                ExitHit.ImpactPoint, 
                ExitHit.ImpactNormal.Rotation(), 
                EAttachLocation::KeepWorldPosition, 
                LifeTime
            );

            if (ExitDecalComp)
            {
                ExitDecalComp->SetSortOrder(999);
                ExitDecalComp->SetFadeOut(LifeTime - 1.f, 1.f, false);
                ExitDecalComp->SetFadeScreenSize(0.002f);
            }
        }
        
        CollisionBox->IgnoreActorWhenMoving(OtherActor, true);
        float SpeedModifier = FMath::Clamp(CurrentPenetrationPower / InitialPenetrationPower, 0.3f, 1.0f);
        FVector NewVelocity = ShotDirection * (ProjectileMovement->InitialSpeed * SpeedModifier);
        GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::RestoreVelocity, NewVelocity));
        return; 
    }

    // 无法穿透，物理停滞
    SetLifeSpan(LifeTime);
    if (TracerEffectComponent) TracerEffectComponent->DestroyComponent();
    if (ProjectileMovement) ProjectileMovement->StopMovementImmediately();
    if (CollisionBox) CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectileBullet::RestoreVelocity(FVector NewVelocity)
{
    if (IsValid(this) && ProjectileMovement && CollisionBox)
    {
        // 由于之前的 Blocking Hit，引擎底层已经把 UpdatedComponent 设为 Null 了 把它重新绑定，否则子弹不会动
        ProjectileMovement->SetUpdatedComponent(CollisionBox);
        
        // 强行把子弹沿着射击方向挪动 2 厘米，让它彻底进入“穿透状态”。
        FVector SafeAdvance = NewVelocity.GetSafeNormal() * 2.0f;
        SetActorLocation(GetActorLocation() + SafeAdvance);
        
        // 强制对齐旋转，保证穿透后视觉轨迹绝对笔直
        SetActorRotation(NewVelocity.Rotation());
        
        // 赋予新速度并重新激活物理模拟
        ProjectileMovement->Velocity = NewVelocity;
        ProjectileMovement->Activate();
    }
}

void AProjectileBullet::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this || OtherActor == GetInstigator()) return;
    ABaseCharacter* DamagedActor = Cast<ABaseCharacter>(OtherActor);

    if (DamagedActor && !DamagedActors.Contains(DamagedActor))
    {
        DamagedActors.Add(DamagedActor);
        
        FVector ShotDirection = ProjectileMovement->Velocity.GetSafeNormal();
        if (ShotDirection.IsNearlyZero()) ShotDirection = GetActorForwardVector();

        // 如果 GetActor() 为空，或者 ImpactPoint 是完美的 (0,0,0)，说明这是非扫掠造成的重叠
        FHitResult ValidHit = SweepResult;
        if (ValidHit.GetActor() == nullptr || ValidHit.ImpactPoint.IsNearlyZero()) 
        {
            ValidHit.ImpactPoint = GetActorLocation(); 
            ValidHit.ImpactNormal = -ShotDirection;
            ValidHit.HitObjectHandle = FActorInstanceHandle(DamagedActor); 
        }

        // 计算最终伤害
        float FlightDistance = FVector::Distance(SpawnLocation, GetActorLocation());
        float FinalDamage = GetDamage(FlightDistance) * (CurrentPenetrationPower / InitialPenetrationPower);

        // 服务端应用伤害
        if (HasAuthority())
        {
            UGameplayStatics::ApplyPointDamage(
                OtherActor,
                FinalDamage,
                ShotDirection,
                ValidHit,
                GetInstigator()->Controller,
                this,
                UDamageTypeEquipment::StaticClass()
            );
        }
        
        FRotator HitRotation = ValidHit.ImpactNormal.Rotation();
        // 血液飞溅特效
        if (DamagedActor->BloodEffect)
        {
            auto BloodEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(),
                DamagedActor->BloodEffect,
                ValidHit.ImpactPoint,
                FRotator(-HitRotation.Pitch, HitRotation.Yaw + 180.f, HitRotation.Roll)
            );
            
            if (BloodEffectComponent)
            {
                BloodEffectComponent->SetVariableInt(TEXT("Count"), ULibraryCommon::GetBloodParticleCount(FinalDamage));
                BloodEffectComponent->SetVariableLinearColor(TEXT("Color"), DamagedActor->BloodColor);
            }
        }
        
        // 血雾特效
        if (DamagedActor->BloodSmokeEffect)
        {
            auto BloodSmokeEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(),
                DamagedActor->BloodSmokeEffect,
                ValidHit.ImpactPoint,
                FRotator(-HitRotation.Pitch, HitRotation.Yaw + 180.f, HitRotation.Roll)
            );
            
            if (BloodSmokeEffectComponent)
            {
                BloodSmokeEffectComponent->SetVariableLinearColor(TEXT("SmokeColor"), DamagedActor->BloodColor);
            }
        }

        // 播放击中肉体音效
        if (AssetSubsystem == nullptr) AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
        if (AssetSubsystem && AssetSubsystem->EquipmentAsset && AssetSubsystem->EquipmentAsset->ImpactSound_Body)
        {
            UGameplayStatics::PlaySoundAtLocation(this, AssetSubsystem->EquipmentAsset->ImpactSound_Body, GetActorLocation());
        }

        // 穿透力结算与销毁
        CurrentPenetrationPower -= DamagedActor->GetBodyResistance();
        if (CurrentPenetrationPower <= 0.f)
        {
            if (TracerEffectComponent) TracerEffectComponent->DestroyComponent();
            Destroy(); 
        }
    }
}

void AProjectileBullet::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// UE_LOG(LogTemp, Warning, TEXT("OnEndOverlap"));
}
