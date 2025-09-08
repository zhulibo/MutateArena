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
	class ABaseController* BaseController;

	float VelocityFactor = 0.f;
	float JumpFactor = 0.f;
	float ShootFactor = 0.f;
	void SetHUDCrosshair(float DeltaSeconds);

	// FVector2D WalkSpeedRange;
	// FVector2D VelocityFactorRange = FVector2D(0.f, 0.4f);

};
