#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_SendGameplayEvent.generated.h"

UCLASS()
class MUTATEARENA_API UAN_SendGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FGameplayTag EventTag;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
	
};
