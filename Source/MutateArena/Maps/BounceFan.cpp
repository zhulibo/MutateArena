#include "BounceFan.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/Character.h"
#include "NiagaraComponent.h"

ABounceFan::ABounceFan()
{
    PrimaryActorTick.bCanEverTick = false;
    
    bReplicates = true;

    DefaultRootComp = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRootComp"));
    RootComponent = DefaultRootComp;

    FanMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FanMesh"));
    FanMesh->SetupAttachment(RootComponent);

    LaunchDirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("LaunchDirectionArrow"));
    LaunchDirectionArrow->SetupAttachment(RootComponent);
    LaunchDirectionArrow->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
    LaunchDirectionArrow->SetRelativeLocation(FVector(0.f, 0.f, 10.f));

    NS_Wind = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NS_Wind"));
    NS_Wind->SetupAttachment(LaunchDirectionArrow);

    TriggerCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("TriggerCapsule"));
    TriggerCapsule->SetupAttachment(RootComponent);
}

void ABounceFan::BeginPlay()
{
    Super::BeginPlay();
    
    SetReplicateMovement(true);

    TriggerCapsule->OnComponentBeginOverlap.AddDynamic(this, &ABounceFan::OnOverlapBegin);
}

void ABounceFan::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor != this)
    {
        if (ACharacter* OverlappedCharacter = Cast<ACharacter>(OtherActor))
        {
            if (OverlappedCharacter->IsLocallyControlled() || HasAuthority())
            {
                FVector LaunchDirection = LaunchDirectionArrow->GetForwardVector();
                FVector FinalLaunchVelocity = LaunchDirection * LaunchSpeed;
                OverlappedCharacter->LaunchCharacter(FinalLaunchVelocity, bXYOverride, bZOverride);
            }
        }
    }
}
