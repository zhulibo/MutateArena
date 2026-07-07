#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_PlaySound2DLocal.generated.h"

class UMetaSoundSource;

UCLASS()
class UAN_PlaySound2DLocal : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAN_PlaySound2DLocal();

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	// 要播放的 MetaSound 资产
	UPROPERTY(EditAnywhere)
	UMetaSoundSource* MetaSound;

	UPROPERTY(EditAnywhere)
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere)
	float PitchMultiplier = 1.0f;
};
