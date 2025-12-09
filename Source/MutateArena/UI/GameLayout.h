#pragma once

#include "CoreMinimal.h"
#include "LayoutBase.h"
#include "GameLayout.generated.h"

UENUM(BlueprintType)
enum class EHUDState : uint8
{
	Playing,
	Spectating,
	Inactive
};

enum class ECoolLoginType : uint8;

UCLASS()
class MUTATEARENA_API UGameLayout : public ULayoutBase
{
	GENERATED_BODY()

public:
	// 游戏层
	UPROPERTY(meta = (BindWidget))
	class UCommonActivatableWidgetStack* GameStack;
	// 菜单层
	UPROPERTY(meta = (BindWidget))
	UCommonActivatableWidgetStack* MenuStack;
	// 模态层
	UPROPERTY(meta = (BindWidget))
	UCommonActivatableWidgetStack* ModalStack;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UMutationContainer> MutationContainerClass;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UMeleeContainer> MeleeContainerClass;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UTeamDeadMatchContainer> TeamDeadMatchContainerClass;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UPauseMenu> PauseMenuClass;
	void ShowPauseMenu();

};
