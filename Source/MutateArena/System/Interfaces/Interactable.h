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
	// 交互目标是否允许交互
	virtual bool CanInteract() { return false; }
	// 通知交互目标交互成功
	virtual void OnInteract(class ABaseCharacter* BaseCharacter) {}
	// 在服务端通知交互目标交互成功
	virtual void OnInteract_Server() {}
	// 交互目标通知交互者已经与突变体交互成功（在交互者本地运行）
	virtual void OnInteractMutantSuccess(class AMutantCharacter* MutantCharacter) {}

};
