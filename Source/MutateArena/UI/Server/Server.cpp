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
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/GameModes/Data/GameModeType.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MutateArena/Utils/LibraryNotify.h"
#include "Engine/UserInterfaceSettings.h"
#include "Internationalization/StringTable.h"
#include "MutateArena/Assets/Data/CommonAsset.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/GameplayTags.h"
#include "MutateArena/UI/Common/ComboBoxItem.h"
#include "MutateArena/UI/Common/CommonComboBox2.h"

#define LOCTEXT_NAMESPACE "UServer"

void UServer::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ModeComboBox->OnGenerateItemWidget.BindDynamic(this, &ThisClass::GenerateComboBoxWidget);
	ModeComboBox->OnGenerateContentWidget.BindDynamic(this, &ThisClass::GenerateComboBoxWidget);
	
	MapComboBox->OnGenerateItemWidget.BindDynamic(this, &ThisClass::GenerateComboBoxWidget);
	MapComboBox->OnGenerateContentWidget.BindDynamic(this, &ThisClass::GenerateComboBoxWidget);

	ModeComboBox->AddOption(GameModes::All);
	const UEnum* ModeEnum = StaticEnum<EGameMode2>();
	for (int32 i = 0; i < static_cast<int32>(EGameMode2::None); ++i)
	{
		FName EnumKey = FName(*ModeEnum->GetNameStringByIndex(i));
		ModeComboBox->AddOption(EnumKey);
	}
	
	ModeComboBox->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::OnModeComboBoxChanged);
	ModeComboBox->SetSelectedOption(GameModes::All);
	// NativeOnInitialized 中 ModeComboBox->SetSelectedOption 无法触发 OnModeComboBoxChanged 主动调一下
	OnModeComboBoxChanged(GameModes::All, ESelectInfo::Direct);
	
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
		NOTIFY(this, Colors::Red, LOCTEXT("CreateServerFailed", "Create server failed"));
	}
}

void UServer::OnModeComboBoxChanged(FName SelectedItem, ESelectInfo::Type SelectionType)
{
	MapComboBox->ClearOptions();
	MapComboBox->AddOption(GameModes::All);

	if (SelectedItem == GameModes::Mutation) 
	{
		const UEnum* MapEnum = StaticEnum<EMutationMap>();
		for (int32 i = 0; i < static_cast<int32>(EMutationMap::None); ++i)
		{
			MapComboBox->AddOption(FName(MapEnum->GetNameStringByIndex(i)));
		}
	}
	else if (SelectedItem == GameModes::Melee)
	{
		const UEnum* MapEnum = StaticEnum<EMeleeMap>();
		for (int32 i = 0; i < static_cast<int32>(EMeleeMap::None); ++i)
		{
			MapComboBox->AddOption(FName(MapEnum->GetNameStringByIndex(i)));
		}
	}
	else if (SelectedItem == GameModes::TeamDeadMatch)
	{
		const UEnum* MapEnum = StaticEnum<ETeamDeadMatchMap>();
		for (int32 i = 0; i < static_cast<int32>(ETeamDeadMatchMap::None); ++i)
		{
			MapComboBox->AddOption(FName(MapEnum->GetNameStringByIndex(i)));
		}
	}

	MapComboBox->SetSelectedOption(GameModes::All);
}

UWidget* UServer::GenerateComboBoxWidget(FName ItemName)
{
	UAssetSubsystem* AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>();
	if (AssetSubsystem == nullptr && AssetSubsystem->CommonAsset == nullptr) return nullptr;
	
	UComboBoxItem* ItemWidget = CreateWidget<UComboBoxItem>(this, AssetSubsystem->CommonAsset->ComboBoxItemClass);
	if (!ItemWidget) return nullptr;
	
	FText DisplayText;

	if (ItemName == GameModes::All)
	{
		DisplayText = LOCTEXT("All", "ALL");
	}
	else
	{
		if (AssetSubsystem->CommonAsset->ST_Common)
		{
			FText TableText = FText::FromStringTable(AssetSubsystem->CommonAsset->ST_Common->GetStringTableId(), ItemName.ToString());
			if (!TableText.IsEmpty())
			{
				DisplayText = TableText;
			}
			else
			{
				DisplayText = FText::FromName(ItemName);
			}
		}
		else
		{
			DisplayText = FText::FromName(ItemName);
		}
	}

	ItemWidget->ItemText->SetText(DisplayText);

	return ItemWidget;
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
			NOTIFY(this, Colors::White, LOCTEXT("NoServerFound", "No server found"));
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
					ServerName = Lobbies[i]->Attributes.Find(LobbyAttrs::ServerName)->GetString();
					ModeName = Lobbies[i]->Attributes.Find(LobbyAttrs::ModeName)->GetString();
					MapName = Lobbies[i]->Attributes.Find(LobbyAttrs::MatchRound)->GetString();
				}

				ServerName = ULibraryCommon::ObfuscateServerName(ServerName, this);
				
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
				if (ModeName == GameModes::Mutation)
				{
					MaxStatus = Lobbies[i]->Attributes.Find(LobbyAttrs::MatchRound)->GetInt64();
				}
				else if (ModeName == GameModes::TeamDeadMatch || ModeName == GameModes::Melee)
				{
					MaxStatus = Lobbies[i]->Attributes.Find(LobbyAttrs::MatchTime)->GetInt64();
				}
				ServerLineButton->Status->SetText(FText::FromString(FString::Printf(TEXT("%lld/%lld"), Status, MaxStatus)));
				
				float ProgressRate = Status / MaxStatus;
				FColor StatusColor;
				if (ProgressRate < 0.f)
				{
					StatusColor = Colors::Grey;
				}
				else if (ProgressRate == 0.f)
				{
					StatusColor = Colors::White;
				}
				else if (ProgressRate < 0.5)
				{
					StatusColor = Colors::Green;
				}
				else if (ProgressRate < 0.8)
				{
					StatusColor = Colors::Yellow;
				}
				else
				{
					StatusColor = Colors::Red;
				}
				ServerLineButton->Status->SetColorAndOpacity(StatusColor);

				// TODO Ping
				int32 Ping = -1;
				ServerLineButton->Ping->SetText(FText::AsNumber(Ping));
				FColor PingColor;
				if (Ping < 0)
				{
					PingColor = Colors::Grey;
				}
				else if (Ping < 100)
				{
					PingColor = Colors::Green;
				}
				else if (Ping < 200)
				{
					PingColor = Colors::Yellow;
				}
				else
				{
					PingColor = Colors::Red;
				}
				ServerLineButton->Ping->SetColorAndOpacity(PingColor);
			}
		}
	}
	else
	{
		NOTIFY(this, Colors::Red, LOCTEXT("FindServerFailed", "Find server failed"));
	}
}

// 重置查询条件
void UServer::OnResetServerButtonClicked()
{
	ServerNameEditableTextBox->SetText(FText());
	ModeComboBox->SetSelectedOption(GameModes::All);
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
		NOTIFY(this, Colors::Red, LOCTEXT("JoinServerFailed", "Join server failed"));
	}
}

void UServer::OnLobbyJoined(const FLobbyJoined& LobbyJoined)
{
	GoToLobby();
}

void UServer::GoToLobby()
{
	if (UISubsystem == nullptr) UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer());
	if (UISubsystem)
	{
		if (auto Layer = UISubsystem->GetLayerStack(TAG_UI_LAYER_SERVER))
		{
			Layer->AddWidget(LobbyClass);
		}
	}
}

#undef LOCTEXT_NAMESPACE
