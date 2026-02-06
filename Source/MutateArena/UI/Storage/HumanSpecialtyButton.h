#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "HumanSpecialtyButton.generated.h"

enum class EHumanSpecialty : uint8;

UCLASS()
class MUTATEARENA_API UHumanSpecialtyButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonLazyImage* ShowImg;

	UPROPERTY()
	EHumanSpecialty HumanSpecialty;

};
