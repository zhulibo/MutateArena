#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

class MUTATEARENA_API IInteractable
{
	GENERATED_BODY()

public:
	// 交互目标是否允许交互（传入交互者，以便目标判断能否被这个人交互）
	virtual bool CanInteract(class ABaseCharacter* Interactor) { return false; }
	
	// 由服务端执行的核心交互逻辑
	virtual void OnInteract_Server(class ABaseCharacter* Interactor) {}
	
};
