#include "TowerCrane.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ATowerCrane::ATowerCrane()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicatingMovement(false);

    RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));
    SetRootComponent(RootComp);

    BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
    BaseMesh->SetupAttachment(RootComp);

    TopMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopMesh"));
    TopMesh->SetupAttachment(RootComp);
}

void ATowerCrane::BeginPlay()
{
    Super::BeginPlay();

    BaseRotation = TopMesh->GetRelativeRotation();
    
    // 赋予初始目标旋转为默认值，保持一开始不动
    TargetRotation = BaseRotation;

    if (HasAuthority())
    {
        GetWorld()->GetTimerManager().SetTimer(
            RotationTimerHandle, 
            this, 
            &ATowerCrane::ToggleRotation, 
            RotationInterval, 
            true
        );
    }
}

void ATowerCrane::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ATowerCrane, bIsRotated);
}

void ATowerCrane::ToggleRotation()
{
    bIsRotated = !bIsRotated;
    UpdateTargetRotation();
}

void ATowerCrane::OnRep_bIsRotated()
{
    // 客户端端：收到状态改变时更新目标旋转
    UpdateTargetRotation();
}

void ATowerCrane::UpdateTargetRotation()
{
    TargetRotation = BaseRotation;
    
    // 如果是旋转状态，则加上角度偏移；否则保持 BaseRotation（即转回来）
    if (bIsRotated)
    {
        TargetRotation.Yaw += RotationAngle;
    }
}

void ATowerCrane::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    FRotator CurrentRotation = TopMesh->GetRelativeRotation();
    
    if (!CurrentRotation.Equals(TargetRotation, 0.01f))
    {
        FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation, TargetRotation, DeltaTime, RotationInterpSpeed);
        TopMesh->SetRelativeRotation(NewRotation);
    }
}
