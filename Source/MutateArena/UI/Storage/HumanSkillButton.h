#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "HumanSkillButton.generated.h"

enum class EHumanSkill : uint8;

UCLASS()
class MUTATEARENA_API UHumanSkillButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UCommonLazyImage* ShowImg;

	UPROPERTY()
	EHumanSkill HumanSkill;

};
