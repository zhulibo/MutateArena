#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerController.h"
#include "BaseController.generated.h"

enum class EMsgType : uint8;

UCLASS()
class MUTATEARENA_API ABaseController : public AModularPlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnUnPossess() override;
	virtual void SetPawn (APawn* InPawn) override;
	virtual void OnRep_Pawn() override;
	virtual void SetViewTarget(class AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams) override;
	
	UPROPERTY()
	class ABaseMode* BaseMode;
	UPROPERTY()
	class ABaseGameState* BaseGameState;
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;
	// Diff between client and server WORLD TIME, not network delay.
	float ServerClientDeltaTime = 0.f;
	void HandleServerClientDeltaTime();
	UFUNCTION(Server, Reliable)
	void RequestServerTime(float ClientTime);
	UFUNCTION(Client, Reliable)
	void ReturnServerTime(float ClientTime, float ServerTime);
	bool bServerClientDeltaTimeHasInit = false;
	float GetServerTime();

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UGameLayout> GameLayoutClass;
	void AddGameLayout();

public:
	void FocusGame();
	void FocusUI();

	// 观战
	UFUNCTION()
	void SetPlayerSpectate();
	UFUNCTION()
	void SetPlayerPlay();
	UFUNCTION(Client, Reliable)
	void ClientHUDStateChanged(EHUDState HUDState);

protected:
	void SetHUDWarmupCountdown(int32 CountdownTime);

	virtual void HandleMatchHasStarted();
	virtual void HandleMatchHasEnded();
	virtual void HandleLeavingMap();

public:
	virtual void SetHUDHealth(float Health) {}

	virtual void SetHUDAmmo(int32 Ammo);
	virtual void SetHUDCarriedAmmo(int32 CarriedAmmo);
	virtual void InitHUD() {}
protected:
	virtual void SetHUDTime() {}

public:
	UFUNCTION(Server, Reliable)
	void ServerSendMsg(EMsgType MsgType, ETeam Team, const FString& PlayerName, const FString& Msg = FString());
	
	UFUNCTION()
	void GiveEquipment(FString EquipmentName);
	
};
