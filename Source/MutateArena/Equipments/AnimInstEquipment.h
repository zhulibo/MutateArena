#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AnimInstEquipment.generated.h"

UCLASS(Abstract)
class MUTATEARENA_API UAnimInstEquipment : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAnimInstEquipment();

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
protected:
	UPROPERTY()
	class AEquipment* Equipment;

	UPROPERTY(BlueprintReadOnly)
	bool bIsAiming =  false;

};
