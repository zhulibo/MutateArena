#include "ExampleComponent.h"

UExampleComponent::UExampleComponent()
{
	// PrimaryComponentTick.bCanEverTick = true;
}

void UExampleComponent::BeginPlay()
{
	Super::BeginPlay();

	// UE_LOG(LogTemp, Warning, TEXT("UExampleComponent::BeginPlay"));
}

void UExampleComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);
}
