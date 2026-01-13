#include "Server.h"

#include "CommonHierarchicalScrollBox.h"
#include "CommonTextBlock.h"
#include "ServerContainer.h"
#include "ServerLineButton.h"
#include "MutateArena/PlayerControllers/MenuController.h"
#include "MutateArena/UI/Common/CommonButton.h"
#include "Components/EditableTextBox.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Lobby.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/GameModes/Data/GameModeType.h"
#include "MutateArena/UI/Common/CommonComboBox.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MutateArena/Utils/LibraryNotify.h"
#include "Engine/UserInterfaceSettings.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/UI/ProjectTags.h"

#define LOCTEXT_NAMESPACE "UServer"

void UServer::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ModeComboBox->AddOption(ALL);
	for (int32 i = 0; i < static_cast<int32>(ECoolGameMode::None); ++i)
	{
		FString EnumValue = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(static_cast<ECoolGameMode>(i)));
		ModeComboBox->AddOption(EnumValue);
	}
	ModeComboBox->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::OnModeComboBoxChanged);
	ModeComboBox->SetSelectedIndex(0);

	RefreshServerButton->OnClicked().AddUObject(this, &ThisClass::OnRefreshServerButtonClicked);
	ResetServerButton->OnClicked().AddUObject(this, &ThisClass::OnResetServerButtonClicked);
	CreateServerButton->OnClicked().AddUObject(this, &ThisClass::OnCreateServerButtonClicked);
	PagePrevButton->OnClicked().AddUObject(this, &ThisClass::OnPagePrevButtonClicked);
	PageNextButton->OnClicked().AddUObject(this, &ThisClass::OnPageNextButtonClicked);

	EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
	if (EOSSubsystem)
	{
		EOSSubsystem->OnCreateLobbyComplete.AddUObject(this, &ThisClass::OnCreateLobbyComplete);
		EOSSubsystem->OnFindLobbiesComplete.AddUObject(this, &ThisClass::OnFindLobbiesComplete);
		EOSSubsystem->OnJoinLobbyComplete.AddUObject(this, &ThisClass::OnJoinLobbyComplete);
		EOSSubsystem->OnLobbyJoined.AddUObject(this, &ThisClass::OnLobbyJoined);
	}
}

void UServer::NativeConstruct()
{
	Super::NativeConstruct();

	OnRefreshServerButtonClicked();

	bIsActioning = false;

	// for (int32 i = 0; i < 100; ++i)
	// {
	// 	if (UServerLineButton* ServerLineButton = CreateWidget<UServerLineButton>(this, ServerLineButtonClass))
	// 	{
	// 		ServerLineButtonContainer->AddChild(ServerLineButton);
	// 	}
	// }
}

void UServer::NativeOnActivated()
{
	Super::NativeOnActivated();

	// 游戏结束退回菜单时，直接进入Lobby
	if (EOSSubsystem && EOSSubsystem->GetJoinedLobbies())
	{
		GoToLobby();
	}
}

UWidget* UServer::NativeGetDesiredFocusTarget() const
{
	return RefreshServerButton;
}

// 创建大厅
void UServer::OnCreateServerButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("OnCreateServerButtonClicked ------------------------------------------"));
	if (bIsActioning) return;
	
	if (EOSSubsystem)
	{
		bIsActioning = true;
		
		CreateServerButton->ButtonText->SetText(LOCTEXT("Creating", "Creating"));
		CreateServerButton->SetIsEnabled(false);

		EOSSubsystem->CreateLobby();
	}
}

// 创建大厅完成事件
void UServer::OnCreateLobbyComplete(bool bWasSuccessful)
{
	bIsActioning = false;
	
	CreateServerButton->ButtonText->SetText(LOCTEXT("Create", "Create"));
	CreateServerButton->SetIsEnabled(true);

	if (bWasSuccessful)
	{
	}
	else
	{
		NOTIFY(this, C_RED, LOCTEXT("CreateServerFailed", "Create server failed"));
	}
}

void UServer::OnModeComboBoxChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	MapComboBox->ClearOptions();

	if (SelectedItem == ALL)
	{
		MapComboBox->AddOption(ALL);
	}
	else if (SelectedItem == MUTATION)
	{
		MapComboBox->AddOption(ALL);
		for (int32 i = 0; i < static_cast<int32>(EMutationMap::None); ++i)
		{
			FString EnumValue = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(static_cast<EMutationMap>(i)));
			MapComboBox->AddOption(EnumValue);
		}
	}
	else if (SelectedItem == MELEE)
	{
		MapComboBox->AddOption(ALL);
		for (int32 i = 0; i < static_cast<int32>(EMeleeMap::None); ++i)
		{
			FString EnumValue = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(static_cast<EMeleeMap>(i)));
			MapComboBox->AddOption(EnumValue);
		}
	}
	else if (SelectedItem == TEAM_DEAD_MATCH)
	{
		MapComboBox->AddOption(ALL);
		for (int32 i = 0; i < static_cast<int32>(ETeamDeadMatchMap::None); ++i)
		{
			FString EnumValue = ULibraryCommon::GetEnumValue(UEnum::GetValueAsString(static_cast<ETeamDeadMatchMap>(i)));
			MapComboBox->AddOption(EnumValue);
		}
	}

	MapComboBox->SetSelectedIndex(0);
}

// 查找大厅
void UServer::OnRefreshServerButtonClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("OnRefreshServerButtonClicked ------------------------------------------"));
	if (EOSSubsystem == nullptr) EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
	if (EOSSubsystem)
	{
		RefreshServerButton->ButtonText->SetText(LOCTEXT("Refreshing", "Refreshing"));
		RefreshServerButton->SetIsEnabled(false);

		EOSSubsystem->FindLobbies(
			ServerNameEditableTextBox->GetText().ToString(),
			ModeComboBox->GetSelectedOption(),
			MapComboBox->GetSelectedOption()
		);
	}
}

// 查找大厅完成事件
void UServer::OnFindLobbiesComplete(bool bWasSuccessful, const TArray<TSharedRef<const FLobby>>& Lobbies)
{
	RefreshServerButton->ButtonText->SetText(LOCTEXT("Refresh", "Refresh"));
	RefreshServerButton->SetIsEnabled(true);

	if (bWasSuccessful)
	{
		if (ServerLineButtonContainer == nullptr && ServerLineButtonClass == nullptr) return;

		ServerLineButtonContainer->ClearChildren();

		if (Lobbies.Num() == 0)
		{
			NOTIFY(this, C_WHITE, LOCTEXT("NoServerFound", "No server found"));
			return;
		}
		
		for (int32 i = 0; i < Lobbies.Num(); ++i)
		{
			if (UServerLineButton* ServerLineButton = CreateWidget<UServerLineButton>(this, ServerLineButtonClass))
			{
				ServerLineButton->Lobby = Lobbies[i];
				ServerLineButton->OnClicked().AddUObject(this, &ThisClass::OnServerLineButtonClicked, ServerLineButton);
				ServerLineButtonContainer->AddChild(ServerLineButton);
				
				FString ServerName = FString("-1");
				FString ModeName = FString("-1");
				FString MapName = FString("-1");
				if (Lobbies[i]->Attributes.Num() > 0)
				{
					ServerName = Lobbies[i]->Attributes.Find(LOBBY_SERVER_NAME)->GetString();
					ModeName = Lobbies[i]->Attributes.Find(LOBBY_MODE_NAME)->GetString();
					MapName = Lobbies[i]->Attributes.Find(LOBBY_MAP_NAME)->GetString();
				}

				ServerName = ULibraryCommon::ObfuscateName(ServerName, this);
				
				if (Lobbies[i]->Attributes.Num() > 0)
				{
					ServerLineButton->Server->SetText(FText::FromString(ServerName));
					ServerLineButton->Mode->SetText(FText::FromString(ModeName));
					ServerLineButton->Map->SetText(FText::FromString(MapName));
				}
				ServerLineButton->Player->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), Lobbies[i]->Members.Num(), Lobbies[i]->MaxMembers)));

				// 游戏进度
				int64 Status = EOSSubsystem->GetLobbyStatus();
				int64 MaxStatus = -1;
				if (ModeName == MUTATION)
				{
					MaxStatus = Lobbies[i]->Attributes.Find(LOBBY_MATCH_ROUND)->GetInt64();
				}
				else if (ModeName == TEAM_DEAD_MATCH || ModeName == MELEE)
				{
					MaxStatus = Lobbies[i]->Attributes.Find(LOBBY_MATCH_TIME)->GetInt64();
				}
				ServerLineButton->Status->SetText(FText::FromString(FString::Printf(TEXT("%lld/%lld"), Status, MaxStatus)));
				
				float ProgressRate = Status / MaxStatus;
				FColor StatusColor;
				if (ProgressRate < 0.f)
				{
					StatusColor = C_GREY;
				}
				else if (ProgressRate == 0.f)
				{
					StatusColor = C_WHITE;
				}
				else if (ProgressRate < 0.5)
				{
					StatusColor = C_GREEN;
				}
				else if (ProgressRate < 0.8)
				{
					StatusColor = C_YELLOW;
				}
				else
				{
					StatusColor = C_RED;
				}
				ServerLineButton->Status->SetColorAndOpacity(StatusColor);

				// TODO Ping
				int32 Ping = -1;
				ServerLineButton->Ping->SetText(FText::AsNumber(Ping));
				FColor PingColor;
				if (Ping < 0)
				{
					PingColor = C_GREY;
				}
				else if (Ping < 100)
				{
					PingColor = C_GREEN;
				}
				else if (Ping < 200)
				{
					PingColor = C_YELLOW;
				}
				else
				{
					PingColor = C_RED;
				}
				ServerLineButton->Ping->SetColorAndOpacity(PingColor);
			}
		}
	}
	else
	{
		NOTIFY(this, C_RED, LOCTEXT("FindServerFailed", "Find server failed"));
	}
}

// 重置查询条件
void UServer::OnResetServerButtonClicked()
{
	ServerNameEditableTextBox->SetText(FText());
	ModeComboBox->SetSelectedIndex(0);
}

void UServer::OnPagePrevButtonClicked()
{
	if (ServerLineButtonContainer && ServerLineButtonContainer->GetChildAt(0))
	{
		float PageHeight = ServerLineButtonContainer->GetCachedGeometry().GetAbsoluteSize().Y;
		float LineButtonHeight = ServerLineButtonContainer->GetChildAt(0) ? ServerLineButtonContainer->GetChildAt(0)->GetCachedGeometry().GetAbsoluteSize().Y : 0.f;
		float JumpOverLineButtonNum = 10.f;
		if (LineButtonHeight > 0.f && PageHeight > 0.f)
		{
			JumpOverLineButtonNum = PageHeight * 0.5f / LineButtonHeight;
		}
	
		const FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());
		const float DPI = GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())->GetDPIScaleBasedOnSize(FIntPoint(ViewportSize.X, ViewportSize.Y));
		int32 TargetLineButtonNum = FMath::CeilToInt(ServerLineButtonContainer->GetScrollOffset() * DPI / LineButtonHeight - JumpOverLineButtonNum + 1);
	
		UWidget* TargetLineButton = ServerLineButtonContainer->GetChildAt(TargetLineButtonNum - 1);
		if (TargetLineButton == nullptr)
		{
			TargetLineButtonNum = 1;
			TargetLineButton = ServerLineButtonContainer->GetChildAt(TargetLineButtonNum - 1);
		}
		if (ServerLineButtonContainer->GetChildAt(TargetLineButtonNum - 1) && TargetLineButton)
		{
			ServerLineButtonContainer->ScrollWidgetIntoView(TargetLineButton, true, EDescendantScrollDestination::Center);
			ServerLineButtonContainer->GetChildAt(TargetLineButtonNum - 1)->SetFocus();
		}
	}
}

void UServer::OnPageNextButtonClicked()
{
	if (ServerLineButtonContainer && ServerLineButtonContainer->GetChildAt(0))
	{
		float PageHeight = ServerLineButtonContainer->GetCachedGeometry().GetAbsoluteSize().Y;
		float LineButtonHeight = ServerLineButtonContainer->GetChildAt(0) ? ServerLineButtonContainer->GetChildAt(0)->GetCachedGeometry().GetAbsoluteSize().Y : 0.f;
		float JumpOverLineButtonNum = 10.f;
		if (LineButtonHeight > 0.f && PageHeight > 0.f)
		{
			JumpOverLineButtonNum = PageHeight * 1.5f / LineButtonHeight;
		}

		const FVector2D ViewportSize = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());
		const float DPI = GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())->GetDPIScaleBasedOnSize(FIntPoint(ViewportSize.X, ViewportSize.Y));
		int32 TargetLineButtonNum = FMath::FloorToInt(ServerLineButtonContainer->GetScrollOffset() * DPI / LineButtonHeight + JumpOverLineButtonNum);

		UWidget* TargetLineButton = ServerLineButtonContainer->GetChildAt(TargetLineButtonNum - 1);
		if (TargetLineButton == nullptr)
		{
			TargetLineButtonNum = ServerLineButtonContainer->GetChildrenCount();
			TargetLineButton = ServerLineButtonContainer->GetChildAt(TargetLineButtonNum - 1);
		}
		if (ServerLineButtonContainer->GetChildAt(TargetLineButtonNum - 1) && TargetLineButton)
		{
			ServerLineButtonContainer->ScrollWidgetIntoView(TargetLineButton, true, EDescendantScrollDestination::Center);
			ServerLineButtonContainer->GetChildAt(TargetLineButtonNum - 1)->SetFocus();
		}
	}
}

// 加入大厅
void UServer::OnServerLineButtonClicked(UServerLineButton* ServerLineButton)
{
	UE_LOG(LogTemp, Warning, TEXT("OnServerLineButtonClicked ------------------------------------------"));
	
	if (bIsActioning) return;

	if (EOSSubsystem && ServerLineButton->Lobby.IsValid())
	{
		bIsActioning = true;

		EOSSubsystem->JoinLobby(ServerLineButton->Lobby);
	}
}

// 加入大厅完成事件
void UServer::OnJoinLobbyComplete(bool bWasSuccessful)
{
	bIsActioning = false;

	if (bWasSuccessful)
	{
	}
	else
	{
		NOTIFY(this, C_RED, LOCTEXT("JoinServerFailed", "Join server failed"));
	}
}

void UServer::OnLobbyJoined(const FLobbyJoined& LobbyJoined)
{
	GoToLobby();
}

void UServer::GoToLobby()
{
	if (UISubsystem == nullptr) UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer());;
	if (UISubsystem)
	{
		if (auto Layer = UISubsystem->GetLayerStack(TAG_UI_LAYER_SERVER))
		{
			Layer->AddWidget(LobbyClass);
		}
	}
}

#undef LOCTEXT_NAMESPACE
