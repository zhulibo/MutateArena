#include "Scoreboard.h"
#include "CommonHierarchicalScrollBox.h"
#include "CommonLazyImage.h"
#include "CommonTextBlock.h"
#include "ScoreBoardLineButton.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/Data/DNAAsset2.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/System/DataAssetManager.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/Storage/SaveGameLoadout.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"
#include "MutateArena/Utils/LibraryCommon.h"

void UScoreboard::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->ShowScoreboard.AddUObject(this, &ThisClass::ShowScoreboard);
	}
}

void UScoreboard::ShowScoreboard(bool bIsShow)
{
	if (bIsShow)
	{
		// ETriggerEvent::Triggered时调用ShowScoreboard，会频繁触发
		if (GetVisibility() == ESlateVisibility::Visible) return;

		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ThisClass::RefreshScoreBoard, 0.5f, true, 0.f);

		if (EOSSubsystem == nullptr) EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
		if (EOSSubsystem)
		{
			Server->SetText(FText::FromString(ULibraryCommon::ObfuscateServerName(EOSSubsystem->GetLobbyServerName(), this)));
		}

		SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		SetVisibility(ESlateVisibility::Hidden);

		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	}
}

// TODO 不同模式计分板
void UScoreboard::RefreshScoreBoard()
{
	if (GetVisibility() != ESlateVisibility::Visible) return;
	
	if (BaseGameState == nullptr) BaseGameState = GetWorld()->GetGameState<ABaseGameState>();
	if (BaseGameState)
	{
		TArray<ABasePlayerState*> PlayerStates;
		PlayerStates.Append(BaseGameState->GetPlayerStates({}));

		// 删除空对象
		PlayerStates.RemoveAll([](const ABasePlayerState* BasePlayerState) {
			if (BasePlayerState == nullptr)
			{
				UE_LOG(LogTemp, Warning, TEXT("Found a null pointer in PlayerStates, removing it."));
			}
			return BasePlayerState == nullptr;
		});

		// 按照伤害排序
		Algo::Sort(PlayerStates, [](const ABasePlayerState* A, const ABasePlayerState* B) {
			return A->Damage > B->Damage;
		});
		
		ABasePlayerState* LocalBasePlayerState = nullptr;
		if (ABaseController* LocalBaseController = Cast<ABaseController>(GetWorld()->GetFirstPlayerController()))
		{
			LocalBasePlayerState = Cast<ABasePlayerState>(LocalBaseController->PlayerState);
		}
		
		if (StorageSubsystem == nullptr) StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
		
		int32 CurrentPlayerCount = PlayerStates.Num();
		int32 ExistingWidgetCount = ScoreBoardContainer->GetChildrenCount();

		// 1. 复用或创建 Widget 以匹配当前玩家数量
		for (int32 i = 0; i < CurrentPlayerCount; ++i)
		{
			UScoreBoardLineButton* ScoreBoardLineButton = nullptr;

			if (i < ExistingWidgetCount)
			{
				// 获取已存在的 Widget 并确保其可见
				ScoreBoardLineButton = Cast<UScoreBoardLineButton>(ScoreBoardContainer->GetChildAt(i));
				if (ScoreBoardLineButton)
				{
					ScoreBoardLineButton->SetVisibility(ESlateVisibility::Visible);
				}
			}
			else
			{
				// 现存 Widget 不足，创建新的 Widget 并添加到容器
				ScoreBoardLineButton = CreateWidget<UScoreBoardLineButton>(this, ScoreBoardLineButtonClass);
				if (ScoreBoardLineButton)
				{
					ScoreBoardContainer->AddChild(ScoreBoardLineButton);
				}
			}

			// 2. 更新 Widget 数据
			if (ScoreBoardLineButton)
			{
				FLinearColor TargetColor = C_WHITE;
				if (LocalBasePlayerState)
				{
					TargetColor = PlayerStates[i]->Team == LocalBasePlayerState->Team ? C_BLUE : C_RED;
				}
				
				// 玩家名
				FString PlayerName = PlayerStates[i]->GetPlayerName();
				ScoreBoardLineButton->Player->SetText(FText::FromString(ULibraryCommon::ObfuscateName(PlayerName, this)));
				ScoreBoardLineButton->Player->SetColorAndOpacity(TargetColor);
				
				// DNA
				if (StorageSubsystem && StorageSubsystem->CacheLoadout)
				{
					if (UDNAAsset2* DNAAsset1 = StorageSubsystem->GetDNAAssetByType(PlayerStates[i]->DNA1))
					{
						ScoreBoardLineButton->DNA1->SetBrushFromLazyTexture(DNAAsset1->Icon);
					}
					if (UDNAAsset2* DNAAsset2 = StorageSubsystem->GetDNAAssetByType(PlayerStates[i]->DNA2))
					{
						ScoreBoardLineButton->DNA2->SetBrushFromLazyTexture(DNAAsset2->Icon);
					}
				}
				
				ScoreBoardLineButton->Damage->SetText(FText::FromString(FString::FromInt(PlayerStates[i]->Damage)));
				ScoreBoardLineButton->Damage->SetColorAndOpacity(TargetColor);

				if (BaseGameState->ActorHasTag(TAG_GAME_STATE_MELEE) || BaseGameState->ActorHasTag(TAG_GAME_STATE_TDM))
				{
					ScoreBoardLineButton->Death->SetText(FText::FromString(FString::FromInt(PlayerStates[i]->Death)));
				}
				if (BaseGameState->ActorHasTag(TAG_GAME_STATE_MUTATION))
				{
					ScoreBoardLineButton->Survive->SetText(FText::FromString(FString::FromInt(PlayerStates[i]->Survive)));
					ScoreBoardLineButton->Infect->SetText(FText::FromString(FString::FromInt(PlayerStates[i]->Infect)));
				}
				ScoreBoardLineButton->Death->SetColorAndOpacity(TargetColor);
				ScoreBoardLineButton->Survive->SetColorAndOpacity(TargetColor);
				ScoreBoardLineButton->Infect->SetColorAndOpacity(TargetColor);

				// TODO 换成图标
				ScoreBoardLineButton->Platform->SetText(FText::FromString(UGameplayStatics::GetPlatformName()));
				ScoreBoardLineButton->Platform->SetColorAndOpacity(TargetColor);

				FString InputType = FString();
				switch (PlayerStates[i]->InputType)
				{
				case ECommonInputType::MouseAndKeyboard:
					InputType = "KBM";
					break;
				case ECommonInputType::Gamepad:
					InputType = "Controller";
					break;
				case ECommonInputType::Touch:
					InputType = "Touch";
					break;
				}
				ScoreBoardLineButton->Input->SetText(FText::FromString(InputType));
				ScoreBoardLineButton->Input->SetColorAndOpacity(TargetColor);
		
				ScoreBoardLineButton->Ping->SetText(FText::FromString(FString::FromInt(PlayerStates[i]->GetPingInMilliseconds())));
				ScoreBoardLineButton->Ping->SetColorAndOpacity(TargetColor);
			}
		}

		// 3. 隐藏多余的 Widget（例如当有玩家退出游戏时）
		for (int32 i = CurrentPlayerCount; i < ScoreBoardContainer->GetChildrenCount(); ++i)
		{
			if (UWidget* ExtraWidget = ScoreBoardContainer->GetChildAt(i))
			{
				ExtraWidget->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	// 帧率
	float DeltaTime = FApp::GetDeltaTime();
	int32 FPSValue = DeltaTime > 0.f ? FMath::RoundToInt(1.f / DeltaTime) : 0;
	FPS->SetText(FText::FromString(FString::Printf(TEXT("%d"), FPSValue)));
}
