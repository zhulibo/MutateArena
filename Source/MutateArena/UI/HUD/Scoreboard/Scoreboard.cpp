#include "Scoreboard.h"
#include "CommonHierarchicalScrollBox.h"
#include "CommonTextBlock.h"
#include "ScoreBoardLineButton.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "MutateArena/Utils/LibraryCommon.h"

void UScoreboard::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (ABaseController* BaseController = Cast<ABaseController>(GetOwningPlayer()))
	{
		BaseController->ShowScoreboard.AddUObject(this, &ThisClass::ShowScoreboard);
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
			Server->SetText(FText::FromString(ULibraryCommon::ObfuscatePlayerName(EOSSubsystem->GetLobbyServerName(), this)));
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
		ScoreBoardContainer->ClearChildren();

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
		
		for (int32 i = 0; i < PlayerStates.Num(); ++i)
		{
			if (UScoreBoardLineButton* ScoreBoardLineButton = CreateWidget<UScoreBoardLineButton>(this, ScoreBoardLineButtonClass))
			{
				FString PlayerName = PlayerStates[i]->GetPlayerName();
				ScoreBoardLineButton->Player->SetText(FText::FromString(ULibraryCommon::ObfuscatePlayerName(PlayerName, this)));
				
				ScoreBoardLineButton->Damage->SetText(FText::FromString(FString::FromInt(PlayerStates[i]->Damage)));

				if (BaseGameState->ActorHasTag(TAG_GAME_STATE_MELEE) || BaseGameState->ActorHasTag(TAG_GAME_STATE_TDM))
				{
					ScoreBoardLineButton->Death->SetText(FText::FromString(FString::FromInt(PlayerStates[i]->Death)));
				}
				
				if (BaseGameState->ActorHasTag(TAG_GAME_STATE_MUTATION))
				{
					ScoreBoardLineButton->Survive->SetText(FText::FromString(FString::FromInt(PlayerStates[i]->Survive)));
					ScoreBoardLineButton->Infect->SetText(FText::FromString(FString::FromInt(PlayerStates[i]->Infect)));
				}

				// TODO 换成图标
				ScoreBoardLineButton->Platform->SetText(FText::FromString(UGameplayStatics::GetPlatformName()));

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
		
				ScoreBoardLineButton->Ping->SetText(FText::FromString(FString::FromInt(PlayerStates[i]->GetPingInMilliseconds())));
				
				ScoreBoardContainer->AddChild(ScoreBoardLineButton);

				if (LocalBasePlayerState)
				{
					ScoreBoardLineButton->SetColorAndOpacity(PlayerStates[i]->Team == LocalBasePlayerState->Team ? C_BLUE : C_RED);
				}
			}
		}
	}

	// 帧率
	float DeltaTime = FApp::GetDeltaTime();
	int32 FPSValue = DeltaTime > 0.0f ? FMath::RoundToInt(1.0f / DeltaTime) : 0;
	FPS->SetText(FText::FromString(FString::Printf(TEXT("%d"), FPSValue)));
}
