#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AnimInstEquipment.generated.h"

enum class EEquipmentName : uint8;

UCLASS()
class MUTATEARENA_API UAnimInstEquipment : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAnimInstEquipment();

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY()
	class AEquipment* Equipment;

	UPROPERTY(BlueprintReadOnly)
	bool bIsAiming =  false;

};
