#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SmokeBlocker.generated.h"

UCLASS()
class MUTATEARENA_API ASmokeBlocker : public AActor
{
	GENERATED_BODY()

public:
	ASmokeBlocker();

protected:
	// 自带一个遮挡视线并用于灭火的球体组件
	UPROPERTY(VisibleAnywhere)
	class USphereComponent* SmokeBlockerSphere;
	
};