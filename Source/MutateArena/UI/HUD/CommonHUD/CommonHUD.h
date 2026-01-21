#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "CommonHUD.generated.h"

UCLASS()
class MUTATEARENA_API UCommonHUD : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	class ABasePlayerState* LocalPlayerState;
	
	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Announcement;
	void OnAnnouncementChange(FText Text);

	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* KillStreak;
	void OnKillStreakChange(int32 Num);

	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* KillLogContainer;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UKillLogLine> KillLogLineClass;
	void OnAddKillLog(ABasePlayerState* AttackerState, const FText& CauserName, ABasePlayerState* DamagedState);

	UPROPERTY(meta = (BindWidget))
	class UCrosshair* Crosshair;

	UPROPERTY(meta = (BindWidget))
	class UInteract* Interact;

	UPROPERTY(meta = (BindWidget))
	class UTextChat* TextChat;

	UPROPERTY(meta = (BindWidget))
	class USpectator* Spectator;
	void OnHUDStateChange(EHUDState HUDState);

	UPROPERTY(Transient)
	TArray<class UDamageLogLine*> DamageLogPool;
	UDamageLogLine* GetPooledDamageLog();
	void ReturnToPool(UDamageLogLine* Widget);
	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* DamageLogContainer;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UDamageLogLine> DamageLogLineClass;
	void OnCauseDamage(float Num);
	FTimerHandle DamageLogTimerHandle;
	
};
