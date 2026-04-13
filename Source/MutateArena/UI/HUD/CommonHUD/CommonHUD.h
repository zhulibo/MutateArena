#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "CommonHUD.generated.h"

UENUM(BlueprintType)
enum class ECombatIconType : uint8
{
	Cause1000Damage,
	Cause1000Rage,
	BeImmune,
};

enum class EHUDState : uint8;

// TODO 动态添加 Widget 参考 lyra UUIExtensionSubsystem UUIExtensionPointWidget
UCLASS()
class MUTATEARENA_API UCommonHUD : public UCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	
	UPROPERTY()
	class ABasePlayerState* LocalPlayerState;
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;

	UPROPERTY(meta = (BindWidget))
	class UCommonTextBlock* Announcement;
	UFUNCTION()
	void OnAnnouncementChange(const FText& Text, float DisplayTime);
	UFUNCTION()
	void ClearAnnouncement();
	FTimerHandle AnnouncementTimerHandle;
	
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* KillStreak;
	void OnKillStreakChange(int32 Num);
	FTimerHandle HiddenKillStreakTimerHandle;
	void HiddenKillStreak();
	
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

	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* DamageLogContainer;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UDamageLogLine> DamageLogLineClass;
	void OnCauseDamage(float Num);
	FTimerHandle DamageLogTimerHandle;
	
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* HostingBox;
	void OnAFKHosting(bool bIsHosting);
	
	UPROPERTY(meta = (BindWidget))
	class UHighAlert* HighAlert;

	UPROPERTY(meta = (BindWidget))
	class UCommonLazyImage* CombatIcon;
	void OnCombatIconChange(ECombatIconType CombatIconType);
	FTimerHandle CombatIconTimerHandle;
	void ClearCombatIcon();

	UPROPERTY(meta = (BindWidget))
	class UMinimap* Minimap;
	
};
