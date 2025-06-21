#include "Scoreboard.h"
#include "CommonHierarchicalScrollBox.h"
#include "CommonTextBlock.h"
#include "ScoreBoardLineButton.h"
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
			GameTitle->SetText(FText::FromString(ULibraryCommon::ObfuscatePlayerName(EOSSubsystem->GetLobbyServerName(), this)));
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

		for (int32 i = 0; i < PlayerStates.Num(); ++i)
		{
			if (UScoreBoardLineButton* ScoreBoardLineButton = CreateWidget<UScoreBoardLineButton>(this, ScoreBoardLineButtonClass))
			{
				FString PlayerName = PlayerStates[i]->GetPlayerName();
				ScoreBoardLineButton->Player->SetText(FText::FromString(ULibraryCommon::ObfuscatePlayerName(PlayerName, this)));
				ScoreBoardLineButton->Damage->SetText(FText::FromString(FString::FromInt(PlayerStates[i]->Damage)));
				ScoreBoardContainer->AddChild(ScoreBoardLineButton);
				if (PlayerStates[i]->Team == ETeam::Team2)
				{
					ScoreBoardLineButton->SetColorAndOpacity(C_GREEN);
				}
			}
		}
	}

	// 帧率
	float DeltaTime = FApp::GetDeltaTime();
	int32 FPSValue = DeltaTime > 0.0f ? FMath::RoundToInt(1.0f / DeltaTime) : 0;
	FPS->SetText(FText::FromString(FString::Printf(TEXT("%d"), FPSValue)));

	// 服务器名字
	
}
