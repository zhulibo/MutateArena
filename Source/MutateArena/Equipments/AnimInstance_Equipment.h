#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AnimInstance_Equipment.generated.h"

enum class EEquipmentName : uint8;

UCLASS()
class MUTATEARENA_API UAnimInstance_Equipment : public UAnimInstance
{
	GENERATED_BODY()

public:
	UAnimInstance_Equipment();

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY()
	class AEquipment* Equipment;

	UPROPERTY(BlueprintReadOnly)
	bool bIsAiming;

};
