#pragma once

#include "CoreMinimal.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "PlayerSubsystem.generated.h"

enum class ECoolLoginType : uint8;

UCLASS()
class MUTATEARENA_API UPlayerSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPlayerSubsystem();

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UPlayerSubsystem, STATGROUP_Tickables);
	}

protected:
	UPROPERTY()
	UEOSSubsystem* EOSSubsystem;

public:
	UPROPERTY()
	class UNotifyLayout* NotifyLayout;
	void AddNotifyLayout();
	void AddNotify(const FColor DisplayColor, const FText& Msg);
	// UMG会在关卡切换时销毁，待新关卡Viewport加载后再显示登录通知。
	void CheckIfNeedShowLoginNotify();
protected:
	bool bShowNotify_LoggedIn = false;
	bool bShowNotify_LoggedInReducedFunctionality = false;
	bool bShowNotify_NotLoggedIn = false;
	bool bShowNotify_UsingLocalProfile = false;
	void OnLoginStatusChanged(const FAuthLoginStatusChanged& AuthLoginStatusChanged);
	
	// 邀请
	void OnUILobbyJoinRequested(const FUILobbyJoinRequested& UILobbyJoinRequested);

public:
	bool IsDead = false;
	void SetIsDead();
	
};
