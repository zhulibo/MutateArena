#include "ItemSpawnPoint.h"
#include "Components/ArrowComponent.h"

AItemSpawnPoint::AItemSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	ArrowComponent->SetupAttachment(RootComponent);
	ArrowComponent->ArrowSize = 2.0f;
	ArrowComponent->ArrowLength = 50.0f;
}

void AItemSpawnPoint::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	if (ArrowComponent)
	{
		if (ItemType == ESpawnItemType::Pickup)
		{
			ArrowComponent->ArrowColor = FColor::Yellow;
		}
		else if (ItemType == ESpawnItemType::HerbRage)
		{
			ArrowComponent->ArrowColor = FColor::Green;
		}
		else if (ItemType == ESpawnItemType::HerbRepelReceived)
		{
			ArrowComponent->ArrowColor = FColor::Blue;
		}
	}
}
