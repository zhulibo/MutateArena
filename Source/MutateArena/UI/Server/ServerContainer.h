#pragma once

#include "CoreMinimal.h"
#include "MutateArena/UI/LayoutBase.h"
#include "ServerContainer.generated.h"

UCLASS()
class MUTATEARENA_API UServerContainer : public ULayoutBase
{
	GENERATED_BODY()

public:
	// 用于添加大厅的层
	UPROPERTY(meta = (BindWidget))
	class UCommonActivatableWidgetStack* ServerStack;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UServer> ServerClass;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

};
