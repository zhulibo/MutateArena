#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "MinimapIcon.generated.h"

class UImage;
class UMaterialInstanceDynamic;

UCLASS()
class MUTATEARENA_API UMinimapIcon : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	UImage* IconImage;

	void UpdateIconAppearance(bool bIsSameFloor, const FLinearColor& InColor);

protected:
	virtual void NativeOnInitialized() override;
	
private:
	UPROPERTY()
	UMaterialInstanceDynamic* IconMID;
	
};
