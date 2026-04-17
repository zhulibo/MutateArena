#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "CorpseCharacter.generated.h"

UCLASS()
class MUTATEARENA_API ACorpseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACorpseCharacter();

	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditAnywhere)
	UAIPerceptionStimuliSourceComponent* StimuliSourceComp;
	
};
