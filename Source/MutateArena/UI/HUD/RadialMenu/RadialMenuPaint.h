#pragma once

#include "CoreMinimal.h"
#include "RadialMenuBase.h"
#include "RadialMenuPaint.generated.h"

UCLASS()
class MUTATEARENA_API URadialMenuPaint : public URadialMenuBase
{
	GENERATED_BODY()

public:
	
protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;
	
};
