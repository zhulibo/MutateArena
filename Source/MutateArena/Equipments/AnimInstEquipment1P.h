#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AnimInstEquipment1P.generated.h"

UCLASS()
class MUTATEARENA_API UAnimInstEquipment1P : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAnimInstEquipment1P();

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY()
	class AEquipment* Equipment;

	UPROPERTY(BlueprintReadOnly)
	bool bIsAiming = false;

};
