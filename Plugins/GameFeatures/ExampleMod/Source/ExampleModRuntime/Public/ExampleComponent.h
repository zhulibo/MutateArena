#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExampleComponent.generated.h"

UCLASS(MinimalAPI, Blueprintable)
class UExampleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UExampleComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

};
