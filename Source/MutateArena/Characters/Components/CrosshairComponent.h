#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CrosshairComponent.generated.h"

UCLASS()
class MUTATEARENA_API UCrosshairComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCrosshairComponent();
	friend class AHumanCharacter;
	
protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY()
	AHumanCharacter* HumanCharacter;
	UPROPERTY()
	class UUISubsystem* UISubsystem;

public:
	float VelocityFactor = 0.f;
	float JumpFactor = 0.f;
protected:
	float ShootFactor = 0.f;
	void SetHUDCrosshair(float DeltaSeconds);

	FVector2D WalkSpeedRange;

};
