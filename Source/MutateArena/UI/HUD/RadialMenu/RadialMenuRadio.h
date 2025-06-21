#pragma once

#include "CoreMinimal.h"
#include "RadialMenuBase.h"
#include "RadialMenuRadio.generated.h"

UCLASS()
class MUTATEARENA_API URadialMenuRadio : public URadialMenuBase
{
	GENERATED_BODY()

public:

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;

};
