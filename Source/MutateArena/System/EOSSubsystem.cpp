#include "EOSSubsystem.h"

#include "MutateArena/MutateArena.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "MutateArena/Utils/LibraryCommon.h"
#include "MutateArena/Utils/LibraryNotify.h"

#define LOCTEXT_NAMESPACE "UEOSSubsystem"

UEOSSubsystem::UEOSSubsystem()
{
	OnlineServicesPtr = GetServices();
	if (OnlineServicesPtr)
	{
		AuthPtr = OnlineServicesPtr->GetAuthInterface();
		UserInfoPtr = OnlineServicesPtr->GetUserInfoInterface();
		LobbyPtr = OnlineServicesPtr->GetLobbiesInterface();
		UserFilePtr = OnlineServicesPtr->GetUserFileInterface();
		TitleFilePtr = OnlineServicesPtr->GetTitleFileInterface();
		CommercePtr = OnlineServicesPtr->GetCommerceInterface();

		if (AuthPtr)
		{
			OnlineEventDelegateHandles.Add(AuthPtr->OnLoginStatusChanged().Add(this, &ThisClass::BroadcastOnLoginStatusChanged));
		}
		if (LobbyPtr)
		{
			OnlineEventDelegateHandles.Add(LobbyPtr->OnLobbyJoined().Add(this, &ThisClass::BroadcastOnLobbyJoined));
			OnlineEventDelegateHandles.Add(LobbyPtr->OnLobbyMemberJoined().Add(this, &ThisClass::BroadcastOnLobbyMemberJoined));
			OnlineEventDelegateHandles.Add(LobbyPtr->OnLobbyMemberLeft().Add(this, &ThisClass::BroadcastOnLobbyMemberLeft));
			OnlineEventDelegateHandles.Add(LobbyPtr->OnLobbyLeaderChanged().Add(this, &ThisClass::BroadcastOnLobbyLeaderChanged));
			OnlineEventDelegateHandles.Add(LobbyPtr->OnLobbyInvitationAdded().Add(this, &ThisClass::BroadcastOnLobbyInvitationAdded));
			OnlineEventDelegateHandles.Add(LobbyPtr->OnUILobbyJoinRequested().Add(this, &ThisClass::BroadcastOnUILobbyJoinRequested));
			OnlineEventDelegateHandles.Add(LobbyPtr->OnLobbyAttributesChanged().Add(this, &ThisClass::BroadcastOnLobbyAttrChanged));
			OnlineEventDelegateHandles.Add(LobbyPtr->OnLobbyMemberAttributesChanged().Add(this, &ThisClass::BroadcastOnLobbyMemberAttrChanged));
			OnlineEventDelegateHandles.Add(LobbyPtr->OnLobbyLeft().Add(this, &ThisClass::BroadcastOnLobbyLeft));
		}
		if (CommercePtr)
		{
			OnlineEventDelegateHandles.Add(CommercePtr->OnPurchaseCompleted().Add(this, &ThisClass::BroadcastOnPurchaseCompleted));
		}
	}
}

void UEOSSubsystem::Deinitialize()
{
	for (auto& Handle : OnlineEventDelegateHandles)
	{
		Handle.Unbind();
	}

	Super::Deinitialize();
}

// 登录
void UEOSSubsystem::Login(FPlatformUserId TempPlatformUserId, ECoolLoginType LoginType, FString Id, FString Token)
{
	GetWorld()->GetTimerManager().SetTimer(TickNumTimerHandle, this, &ThisClass::ChangeLobbyMemberTickNum, 2.f, true);

	if (AuthPtr == nullptr) return;

	PlatformUserId = TempPlatformUserId;
	GetAccountInfo(PlatformUserId);

	// 已登录
	if (AccountInfo && AuthPtr->IsLoggedIn(AccountInfo->AccountId))
	{
		UE_LOG(LogTemp, Warning, TEXT("Login info exist"));
		GetUserInfo();

		OnLoginComplete.Broadcast(true);

		return;
	}

	FAuthLogin::Params Params;
	Params.PlatformUserId = PlatformUserId;
	switch (LoginType)
	{
	case ECoolLoginType::ExchangeCode:
		Params.CredentialsType = LoginCredentialsType::ExchangeCode;
		break;
	case ECoolLoginType::Password:
		Params.CredentialsType = LoginCredentialsType::Password;
		break;
	case ECoolLoginType::DevAuth:
		Params.CredentialsType = LoginCredentialsType::Developer;
		break;
	}
	Params.CredentialsId = Id;
	Params.CredentialsToken.Emplace<FString>(Token);
	
	AuthPtr->Login(MoveTemp(Params))
	.OnComplete([this](const TOnlineResult<FAuthLogin>& Result)
	{
		if(Result.IsOk())
		{
			AccountInfo = Result.GetOkValue().AccountInfo;

			QueryUserInfo();

			OnLoginComplete.Broadcast(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Login %s"), *Result.GetErrorValue().GetLogString());
			
			OnLoginComplete.Broadcast(false);
		}
	});
}

// 获取账号信息
void UEOSSubsystem::GetAccountInfo(FPlatformUserId Id)
{
	if (AuthPtr == nullptr) return;

	FAuthGetLocalOnlineUserByPlatformUserId::Params Params;
	Params.PlatformUserId = Id;
	TOnlineResult<FAuthGetLocalOnlineUserByPlatformUserId> Result = AuthPtr->GetLocalOnlineUserByPlatformUserId(MoveTemp(Params));
	if (Result.IsOk())
	{
		AccountInfo = Result.GetOkValue().AccountInfo;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GetLocalOnlineUserByPlatformUserId %s"), *Result.GetErrorValue().GetLogString());
	}
}

// 查询用户信息
void UEOSSubsystem::QueryUserInfo()
{
	if (UserInfoPtr == nullptr || AccountInfo == nullptr) return;

	FQueryUserInfo::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	Params.AccountIds.Emplace(AccountInfo->AccountId);

	UserInfoPtr->QueryUserInfo(MoveTemp(Params))
	.OnComplete([this](const TOnlineResult<FQueryUserInfo>& Result)
	{
		if (Result.IsOk())
		{
			GetUserInfo();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("QueryUserInfo %s"), *Result.GetErrorValue().GetLogString());
		}
	});
}

// 获取用户信息
void UEOSSubsystem::GetUserInfo()
{
	if (UserInfoPtr == nullptr || AccountInfo == nullptr) return;

	FGetUserInfo::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	Params.AccountId = AccountInfo->AccountId;

	TOnlineResult<FGetUserInfo> Result = UserInfoPtr->GetUserInfo(MoveTemp(Params));
	if (Result.IsOk())
	{
		UserInfo = Result.GetOkValue().UserInfo;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GetUserInfo %s"), *Result.GetErrorValue().GetLogString());
	}
}

void UEOSSubsystem::BroadcastOnLoginStatusChanged(const FAuthLoginStatusChanged& AuthLoginStatusChanged)
{
	OnLoginStatusChanged.Broadcast(AuthLoginStatusChanged);
}

FAccountId UEOSSubsystem::GetAccountId()
{
	if (AccountInfo.IsValid())
	{
		return AccountInfo->AccountId;
	}

	return FAccountId();
}

FString UEOSSubsystem::GetPlayerName()
{
	if (UserInfo == nullptr)
	{
		return FString(TEXT("-1"));
	}

	return UserInfo->DisplayName;
}

// 创建大厅
void UEOSSubsystem::CreateLobby()
{
	if (AuthPtr == nullptr || AccountInfo == nullptr || !AuthPtr->IsLoggedIn(AccountInfo->AccountId) || LobbyPtr == nullptr)
	{
		OnCreateLobbyComplete.Broadcast(false);
		return;
	}

	FCreateLobby::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	Params.LocalName = LocalLobbyName;
	Params.SchemaId = LOBBY_SCHEMA_ID;
	Params.bPresenceEnabled = true;
	Params.MaxMembers = 18;
	Params.JoinPolicy = ELobbyJoinPolicy::PublicAdvertised;
	Params.Attributes.Emplace(LOBBY_VERSION, ULibraryCommon::GetProjectVersion());
	Params.Attributes.Emplace(LOBBY_SERVER_NAME, FString(TEXT("Default Name")));
	Params.Attributes.Emplace(LOBBY_MODE_NAME, MUTATION);
	Params.Attributes.Emplace(LOBBY_MAP_NAME, FString(TEXT("Dust")));
	Params.Attributes.Emplace(LOBBY_MATCH_ROUND, static_cast<int64>(DEFAULT_MATCH_ROUND));
	Params.Attributes.Emplace(LOBBY_MATCH_TIME, static_cast<int64>(DEFAULT_MATCH_TIME));
	Params.Attributes.Emplace(LOBBY_STATUS, static_cast<int64>(0));
	Params.UserAttributes.Emplace(LOBBY_MEMBER_NAME, UserInfo->DisplayName);
	Params.UserAttributes.Emplace(LOBBY_MEMBER_TEAM, static_cast<int64>(1));
	Params.UserAttributes.Emplace(LOBBY_MEMBER_READY, false);
	Params.UserAttributes.Emplace(LOBBY_MEMBER_MSG, FString());
	Params.UserAttributes.Emplace(LOBBY_MEMBER_TICK_NUM, static_cast<int64>(0));

	LobbyPtr->CreateLobby(MoveTemp(Params))
	.OnComplete([this](const TOnlineResult<FCreateLobby>& Result)
	{
		if (Result.IsOk())
		{
			OnCreateLobbyComplete.Broadcast(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("CreateLobby %s"), *Result.GetErrorValue().GetLogString());
			OnCreateLobbyComplete.Broadcast(false);
		}
	});
}

// 查找大厅
void UEOSSubsystem::FindLobbies(FString LobbyName, FString GameMode, FString MapName)
{
	if (AuthPtr == nullptr || AccountInfo == nullptr || !AuthPtr->IsLoggedIn(AccountInfo->AccountId) || LobbyPtr == nullptr)
	{
		OnFindLobbiesComplete.Broadcast(false, TArray<TSharedRef<const FLobby>>());
		return;
	}

	FFindLobbies::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	Params.MaxResults = 100;

	Params.Filters.Emplace(FFindLobbySearchFilter{
		LOBBY_VERSION,
		ESchemaAttributeComparisonOp::Equals,
		ULibraryCommon::GetProjectVersion()
	});

	if (!LobbyName.IsEmpty())
	{
		Params.Filters.Emplace(FFindLobbySearchFilter{
			LOBBY_SERVER_NAME,
			ESchemaAttributeComparisonOp::Near,
			LobbyName
		});
	}
	if (GameMode != ALL)
	{
		Params.Filters.Emplace(FFindLobbySearchFilter{
			LOBBY_MODE_NAME,
			ESchemaAttributeComparisonOp::Equals,
			GameMode
		});
	}
	if (MapName != ALL)
	{
		Params.Filters.Emplace(FFindLobbySearchFilter{
			LOBBY_MAP_NAME,
			ESchemaAttributeComparisonOp::Equals,
			MapName
		});
	}
	// TODO 添加Status
	
	LobbyPtr->FindLobbies(MoveTemp(Params))
	.OnComplete([this](const TOnlineResult<FFindLobbies>& Result)
	{
		if (Result.IsOk())
		{
			OnFindLobbiesComplete.Broadcast(true, Result.GetOkValue().Lobbies);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("FindLobbies %s"), *Result.GetErrorValue().GetLogString());
			OnFindLobbiesComplete.Broadcast(false, TArray<TSharedRef<const FLobby>>());
		}
	});
}

// 加入大厅
void UEOSSubsystem::JoinLobby(TSharedPtr<const FLobby> Lobby)
{
	if (LobbyPtr == nullptr || AccountInfo == nullptr || Lobby == nullptr) return;

	// 判断版本是否一致
	FString LobbyVersion = FString();
	if (const FSchemaVariant* Attr = Lobby->Attributes.Find(LOBBY_VERSION))
	{
		LobbyVersion = Attr->GetString();
	}
	FString YourVersion = ULibraryCommon::GetProjectVersion();
	if (YourVersion != LobbyVersion)
	{
		NOTIFY(this, C_YELLOW, FText::Format(LOCTEXT("GameVersionInconsistent", "Game version inconsistent: LobbyVersion {0} YourVersion {1}"), FText::FromString(LobbyVersion), FText::FromString(YourVersion)));
		OnJoinLobbyComplete.Broadcast(false);
		return;
	}

	FJoinLobby::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	Params.LocalName = LocalLobbyName;
	Params.LobbyId = Lobby->LobbyId;
	Params.bPresenceEnabled = true;
	Params.UserAttributes.Emplace(LOBBY_MEMBER_TEAM, static_cast<int64>(FMath::RandRange(1, 2)));
	Params.UserAttributes.Emplace(LOBBY_MEMBER_NAME, UserInfo->DisplayName);
	Params.UserAttributes.Emplace(LOBBY_MEMBER_READY, false);
	Params.UserAttributes.Emplace(LOBBY_MEMBER_MSG, FString());
	Params.UserAttributes.Emplace(LOBBY_MEMBER_TICK_NUM, static_cast<int64>(0));

	LobbyPtr->JoinLobby(MoveTemp(Params))
	.OnComplete([this](const TOnlineResult<FJoinLobby>& Result)
	{
		if (Result.IsOk())
		{
			OnJoinLobbyComplete.Broadcast(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("JoinLobby %s"), *Result.GetErrorValue().GetLogString());
			OnJoinLobbyComplete.Broadcast(false);
		}
	});
}

// LobbyMember's Attributes有时为空，为尽量解决此bug，定时修改TickNum Attribute，以便同步所有Attributes到其他客户端
void UEOSSubsystem::ChangeLobbyMemberTickNum()
{
	int64 TickNum = FMath::RandRange(0, 999);
	ModifyLobbyMemberAttr(TMap<FSchemaAttributeId, FSchemaVariant>{
		{ LOBBY_MEMBER_TICK_NUM, TickNum}
	});
}

bool UEOSSubsystem::GetJoinedLobbies()
{
	if (LobbyPtr == nullptr || AccountInfo == nullptr) return false;

	FGetJoinedLobbies::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;

	TOnlineResult<FGetJoinedLobbies> Result = LobbyPtr->GetJoinedLobbies(MoveTemp(Params));
	if (Result.IsOk())
	{
		// 发现加入了多个大厅，离开多余大厅
		if (Result.GetOkValue().Lobbies.Num() > 1)
		{
			NOTIFY(this, C_YELLOW, LOCTEXT("FoundMultipleLobbies", "Found joined multiple lobbies, exiting the extra lobby for you"));

			for (int i = 0; i < Result.GetOkValue().Lobbies.Num(); ++i)
			{
				if (CurLobby->LobbyId == Result.GetOkValue().Lobbies[i]->LobbyId) continue;
				
				FLeaveLobby::Params Params2;
				Params2.LocalAccountId = AccountInfo->AccountId;
				Params2.LobbyId = Result.GetOkValue().Lobbies[i]->LobbyId;

				LobbyPtr->LeaveLobby(MoveTemp(Params2))
				.OnComplete([this, i](const TOnlineResult<FLeaveLobby>& Result)
				{
					if (Result.IsOk())
					{
						NOTIFY(this, C_GREEN, FText::Format(LOCTEXT("LeaveLobbySuccess", "Leave lobby of index {0} success"), FText::AsNumber(i)));
					}
					else
					{
						NOTIFY(this, C_YELLOW, FText::Format(LOCTEXT("LeaveLobbyFailed", "Leave lobby of index {0} failed"), FText::AsNumber(i)));
						
						UE_LOG(LogTemp, Error, TEXT("LeaveLobby %d %s"), i, *Result.GetErrorValue().GetLogString());
					}
				});
			}
		}
		
		if (Result.GetOkValue().Lobbies.Num() > 0)
		{
			return true;
		}

		return false;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GetJoinedLobbies: %s"), *Result.GetErrorValue().GetLogString());
		return false;
	}
}

void UEOSSubsystem::BroadcastOnLobbyInvitationAdded(const FLobbyInvitationAdded& LobbyInvitationAdded)
{
	OnLobbyInvitationAdded.Broadcast(LobbyInvitationAdded);
}

void UEOSSubsystem::BroadcastOnUILobbyJoinRequested(const FUILobbyJoinRequested& UILobbyJoinRequested)
{
	OnUILobbyJoinRequested.Broadcast(UILobbyJoinRequested);
}


// TODO BroadcastOnLobbyJoined触发两次
void UEOSSubsystem::BroadcastOnLobbyJoined(const FLobbyJoined& LobbyJoined)
{
	// TODO bug 此处LobbyMember's Attributes为空
	CurLobby = LobbyJoined.Lobby;
	OnLobbyJoined.Broadcast(LobbyJoined);
}

// TODO BroadcastOnLobbyMemberJoined触发两次
void UEOSSubsystem::BroadcastOnLobbyMemberJoined(const FLobbyMemberJoined& LobbyMemberJoined)
{
	// TODO bug 此处LobbyMember's Attributes为空
	CurLobby = LobbyMemberJoined.Lobby;
	OnLobbyMemberJoined.Broadcast(LobbyMemberJoined);
}

void UEOSSubsystem::BroadcastOnLobbyMemberLeft(const FLobbyMemberLeft& LobbyMemberLef)
{
	CurLobby = LobbyMemberLef.Lobby;
	OnLobbyMemberLeft.Broadcast(LobbyMemberLef);
}

// 房主提升另一名成员为房主
void UEOSSubsystem::PromoteLobbyMember(FAccountId TargetAccountId)
{
	if (LobbyPtr == nullptr || AccountInfo == nullptr || CurLobby == nullptr) return;

	FPromoteLobbyMember::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	Params.LobbyId = CurLobby->LobbyId;
	Params.TargetAccountId = TargetAccountId;

	LobbyPtr->PromoteLobbyMember(MoveTemp(Params))
	.OnComplete([this](const TOnlineResult<FPromoteLobbyMember>& Result)
	{
		if (Result.IsOk())
		{
			OnPromoteLobbyMemberComplete.Broadcast(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("PromoteLobbyMember %s"), *Result.GetErrorValue().GetLogString());
			OnPromoteLobbyMemberComplete.Broadcast(false);
		}
	});
}

void UEOSSubsystem::BroadcastOnLobbyLeaderChanged(const FLobbyLeaderChanged& LobbyLeaderChanged)
{
	CurLobby = LobbyLeaderChanged.Lobby;
	OnLobbyLeaderChanged.Broadcast(LobbyLeaderChanged);
}

void UEOSSubsystem::KickLobbyMember(FAccountId TargetAccountId)
{
	if (LobbyPtr == nullptr || AccountInfo == nullptr || CurLobby == nullptr) return;

	FKickLobbyMember::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	Params.LobbyId = CurLobby->LobbyId;
	Params.TargetAccountId = TargetAccountId;

	LobbyPtr->KickLobbyMember(MoveTemp(Params))
	.OnComplete([this](const TOnlineResult<FKickLobbyMember>& Result)
	{
		if (Result.IsOk())
		{
			OnKickLobbyMemberComplete.Broadcast(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("KickLobbyMember %s"), *Result.GetErrorValue().GetLogString());
			OnKickLobbyMemberComplete.Broadcast(false);
		}
	});
}

// 修改大厅属性
void UEOSSubsystem::ModifyLobbyAttr(TMap<FSchemaAttributeId, FSchemaVariant> UpdatedAttr, TSet<FSchemaAttributeId> RemovedAttr)
{
	if (LobbyPtr == nullptr || AccountInfo == nullptr || CurLobby == nullptr) return;

	FModifyLobbyAttributes::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	Params.LobbyId = CurLobby->LobbyId;
	Params.UpdatedAttributes = MoveTemp(UpdatedAttr);
	Params.RemovedAttributes = MoveTemp(RemovedAttr);

	LobbyPtr->ModifyLobbyAttributes(MoveTemp(Params))
	.OnComplete([this](const TOnlineResult<FModifyLobbyAttributes>& Result)
	{
		if (Result.IsOk())
		{
			OnModifyLobbyAttrComplete.Broadcast(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ModifyLobbyAttr %s"), *Result.GetErrorValue().GetLogString());
			OnModifyLobbyAttrComplete.Broadcast(false);
		}
	});
}

void UEOSSubsystem::BroadcastOnLobbyAttrChanged(const FLobbyAttributesChanged& LobbyAttrChanged)
{
	CurLobby = LobbyAttrChanged.Lobby;
	OnLobbyAttrChanged.Broadcast(LobbyAttrChanged);
}

// 修改大厅成员属性
void UEOSSubsystem::ModifyLobbyMemberAttr(TMap<FSchemaAttributeId, FSchemaVariant> UpdatedAttr, TSet<FSchemaAttributeId> RemovedAttr)
{
	if (LobbyPtr == nullptr || AccountInfo == nullptr || CurLobby == nullptr) return;

	FModifyLobbyMemberAttributes::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	Params.LobbyId = CurLobby->LobbyId;
	Params.UpdatedAttributes = MoveTemp(UpdatedAttr);
	Params.RemovedAttributes = MoveTemp(RemovedAttr);

	LobbyPtr->ModifyLobbyMemberAttributes(MoveTemp(Params))
	.OnComplete([this](const TOnlineResult<FModifyLobbyMemberAttributes>& Result)
	{
		if (Result.IsOk())
		{
			OnModifyLobbyMemberAttrComplete.Broadcast(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ModifyLobbyMemberAttr %s"), *Result.GetErrorValue().GetLogString());
			OnModifyLobbyMemberAttrComplete.Broadcast(false);
		}
	});
}

void UEOSSubsystem::BroadcastOnLobbyMemberAttrChanged(const FLobbyMemberAttributesChanged& LobbyMemberAttrChanged)
{
	CurLobby = LobbyMemberAttrChanged.Lobby;
	OnLobbyMemberAttrChanged.Broadcast(LobbyMemberAttrChanged);
}

void UEOSSubsystem::GetResolvedConnectString(FString& Url)
{
	if (OnlineServicesPtr == nullptr || AccountInfo == nullptr || CurLobby == nullptr) return;

	FGetResolvedConnectString::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	Params.LobbyId = CurLobby->LobbyId;
	Params.PortType = NAME_GamePort;

	TOnlineResult<FGetResolvedConnectString> Result = OnlineServicesPtr->GetResolvedConnectString(MoveTemp(Params));
	if (Result.IsOk())
	{
		Url = Result.GetOkValue().ResolvedConnectString;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GetResolvedConnectString %s"), *Result.GetErrorValue().GetLogString());
	}
}

void UEOSSubsystem::BroadcastOnLobbyLeft(const FLobbyLeft& LobbyLeft)
{
	CurLobby = nullptr;
	OnLobbyLeft.Broadcast(LobbyLeft);
}

// 离开大厅
void UEOSSubsystem::LeaveLobby()
{
	if (LobbyPtr == nullptr || AccountInfo == nullptr || CurLobby == nullptr) return;

	FLeaveLobby::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	Params.LobbyId = CurLobby->LobbyId;

	LobbyPtr->LeaveLobby(MoveTemp(Params))
	.OnComplete([this](const TOnlineResult<FLeaveLobby>& Result)
	{
		if (Result.IsOk())
		{
			CurLobby = nullptr;
			OnLeaveLobbyComplete.Broadcast(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("LeaveLobby %s"), *Result.GetErrorValue().GetLogString());
			OnLeaveLobbyComplete.Broadcast(false);
		}
	});
}

FString UEOSSubsystem::GetLobbyServerName()
{
	if (CurLobby)
	{
		if (const FSchemaVariant* Attr = CurLobby->Attributes.Find(LOBBY_SERVER_NAME))
		{
			return Attr->GetString();
		}
	}
	
	return FString();
}

FString UEOSSubsystem::GetLobbyModeName()
{
	if (CurLobby)
	{
		if (const FSchemaVariant* Attr = CurLobby->Attributes.Find(LOBBY_MODE_NAME))
		{
			return Attr->GetString();
		}
	}
	
	return FString();
}

FString UEOSSubsystem::GetLobbyMapName()
{
	if (CurLobby)
	{
		if (const FSchemaVariant* Attr = CurLobby->Attributes.Find(LOBBY_MAP_NAME))
		{
			return Attr->GetString();
		}
	}
	
	return FString();
}

int64 UEOSSubsystem::GetLobbyMatchRound()
{
	if (CurLobby)
	{
		if (const FSchemaVariant* Attr = CurLobby->Attributes.Find(LOBBY_MATCH_ROUND))
		{
			return Attr->GetInt64();
		}
	}
	
	return 0;
}

int64 UEOSSubsystem::GetLobbyMatchTime()
{
	if (CurLobby)
	{
		if (const FSchemaVariant* Attr = CurLobby->Attributes.Find(LOBBY_MATCH_TIME))
		{
			return Attr->GetInt64();
		}
	}
	
	return 0;
}

int64 UEOSSubsystem::GetLobbyStatus()
{
	if (CurLobby)
	{
		if (const FSchemaVariant* Attr = CurLobby->Attributes.Find(LOBBY_STATUS))
		{
			return Attr->GetInt64();
		}
	}
	
	return 0;
}

ETeam UEOSSubsystem::GetMemberTeam(TSharedPtr<const FLobbyMember> Member)
{
	if (Member)
	{
		if (const FSchemaVariant* Attr = Member->Attributes.Find(LOBBY_MEMBER_TEAM))
		{
			return Attr->GetInt64() == 1 ? ETeam::Team1 : ETeam::Team2;
		}
	}
	
	return ETeam::NoTeam;
}

bool UEOSSubsystem::GetMemberReady(TSharedPtr<const FLobbyMember> Member)
{
	if (Member)
	{
		if (const FSchemaVariant* Attr = Member->Attributes.Find(LOBBY_MEMBER_READY))
		{
			return Attr->GetBoolean();
		}
	}
	
	return false;
}

FString UEOSSubsystem::GetMemberName(TSharedPtr<const FLobbyMember> Member)
{
	if (Member)
	{
		if (const FSchemaVariant* Attr = Member->Attributes.Find(LOBBY_MEMBER_NAME))
		{
			return Attr->GetString();
		}
	}
	
	return FString(TEXT("-1"));
}

TSharedPtr<const FLobbyMember> UEOSSubsystem::GetMember(FAccountId AccountId)
{
	if (CurLobby)
	{
		for (const auto& Member : CurLobby->Members)
		{
			if (Member.Value->AccountId == AccountId)
			{
				return Member.Value;
			}
		}
	}
	
	return nullptr;
}

TSharedPtr<const FLobbyMember> UEOSSubsystem::GetMemberByPlayerName(FString PlayerName)
{
	if (CurLobby)
	{
		for (const auto& Member : CurLobby->Members)
		{
			if (const FSchemaVariant* Attr = Member.Value->Attributes.Find(LOBBY_MEMBER_NAME))
			{
				if (Attr->GetString() == PlayerName)
				{
					return Member.Value;
				}
			}
		}
	}

	return nullptr;
}

FString UEOSSubsystem::GetMemberMsg(TSharedPtr<const FLobbyMember> Member)
{
	if (Member)
	{
		if (const FSchemaVariant* Attr = Member->Attributes.Find(LOBBY_MEMBER_MSG))
		{
			return Attr->GetString();
		}
	}
	
	return FString();
}

bool UEOSSubsystem::IsLobbyHost(TSharedPtr<const FLobbyMember> Member)
{
	if (CurLobby)
	{
		// 未传Member，判断本地是否为主机
		if (Member == nullptr)
		{
			if (AccountInfo)
			{
				return AccountInfo->AccountId == CurLobby->OwnerAccountId;
			}
		}
		else
		{
			return Member->AccountId == CurLobby->OwnerAccountId;
		}
	}
	
	return false;
}

TSharedPtr<const FLobbyMember> UEOSSubsystem::GetLobbyHost()
{
	if (CurLobby)
	{
		for (auto& Member : CurLobby->Members)
		{
			if (IsLobbyHost(Member.Value))
			{
				return Member.Value;
			}
		}
	}
	
	return nullptr;
}

bool UEOSSubsystem::IsLocalMember(TSharedPtr<const FLobbyMember> Member)
{
	if (AccountInfo)
	{
		return Member->AccountId == AccountInfo->AccountId;
	}
	
	return false;
}

TSharedPtr<const FLobbyMember> UEOSSubsystem::GetLocalMember()
{
	if (CurLobby)
	{
		for (auto& Member : CurLobby->Members)
		{
			if (IsLocalMember(Member.Value))
			{
				return Member.Value;
			}
		}
	}

	return nullptr;
}

// 获取远程用户文件
void UEOSSubsystem::EnumerateUserFiles()
{
	if (AuthPtr == nullptr || AccountInfo == nullptr || !AuthPtr->IsLoggedIn(AccountInfo->AccountId)) return;
	if (UserFilePtr == nullptr) return;

	FUserFileEnumerateFiles::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;

	UserFilePtr->EnumerateFiles(MoveTemp(Params))
	.OnComplete([this](const TOnlineResult<FUserFileEnumerateFiles>& Result)
	{
		if (Result.IsOk())
		{
			OnEnumerateUserFilesComplete.Broadcast(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("EnumerateUserFiles %s"), *Result.GetErrorValue().GetLogString());
			OnEnumerateUserFilesComplete.Broadcast(false);
		}
	});
}

// 获取用户文件名
TArray<FString> UEOSSubsystem::GetEnumeratedUserFiles()
{
	TArray<FString> Filenames;
	if (UserFilePtr == nullptr || AccountInfo == nullptr) return Filenames;

	FUserFileGetEnumeratedFiles::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;

	TOnlineResult<FUserFileGetEnumeratedFiles> Result = UserFilePtr->GetEnumeratedFiles(MoveTemp(Params));
	if (Result.IsOk())
	{
		Filenames = Result.GetOkValue().Filenames;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GetEnumeratedUserFiles %s"), *Result.GetErrorValue().GetLogString());
	}
	
	return Filenames;
}

// 读取用户文件
void UEOSSubsystem::ReadUserFile(FString Filename)
{
	if (UserFilePtr == nullptr || AccountInfo == nullptr) return;

	FUserFileReadFile::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	Params.Filename = Filename;

	UserFilePtr->ReadFile(MoveTemp(Params))
	.OnComplete([this, Filename](const TOnlineResult<FUserFileReadFile>& Result)
	{
		if (Result.IsOk())
		{
			OnReadUserFileComplete.Broadcast(true, Result.GetOkValue().FileContents, Filename);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ReadUserFile %s %s"), *Filename, *Result.GetErrorValue().GetLogString());
			OnReadUserFileComplete.Broadcast(false, FUserFileContentsRef(), Filename);
		}
	});
}

// 创建或覆盖用户文件
void UEOSSubsystem::WriteUserFile(FString Filename, FUserFileContents FileContents)
{
	if (UserFilePtr == nullptr || AccountInfo == nullptr) return;

	FUserFileWriteFile::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	Params.Filename = Filename;
	Params.FileContents = MoveTemp(FileContents);

	UserFilePtr->WriteFile(MoveTemp(Params))
	.OnComplete([this](const TOnlineResult<FUserFileWriteFile>& Result)
	{
		if (Result.IsOk())
		{
			OnWriteUserFileComplete.Broadcast(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("WriteUserFile %s"), *Result.GetErrorValue().GetLogString());
			OnWriteUserFileComplete.Broadcast(false);
		}
	});
}

// 获取远程标题文件
void UEOSSubsystem::EnumerateTitleFiles()
{
	if (AuthPtr == nullptr || AccountInfo == nullptr || !AuthPtr->IsLoggedIn(AccountInfo->AccountId)) return;
	if (TitleFilePtr == nullptr) return;

	FTitleFileEnumerateFiles::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;

	TitleFilePtr->EnumerateFiles(MoveTemp(Params))
	.OnComplete([this](const TOnlineResult<FTitleFileEnumerateFiles>& Result)
	{
		if (Result.IsOk())
		{
			OnEnumerateTitleFilesComplete.Broadcast(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("EnumerateTitleFiles %s"), *Result.GetErrorValue().GetLogString());
			OnEnumerateTitleFilesComplete.Broadcast(false);
		}
	});
}

// 获取标题文件名
TArray<FString> UEOSSubsystem::GetEnumeratedTitleFiles()
{
	TArray<FString> Filenames;
	if (TitleFilePtr == nullptr || AccountInfo == nullptr) return Filenames;

	FTitleFileGetEnumeratedFiles::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;

	TOnlineResult<FTitleFileGetEnumeratedFiles> Result = TitleFilePtr->GetEnumeratedFiles(MoveTemp(Params));
	if (Result.IsOk())
	{
		Filenames = Result.GetOkValue().Filenames;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GetEnumeratedTitleFiles %s"), *Result.GetErrorValue().GetLogString());
	}
	
	return Filenames;
}

// 读取标题文件
void UEOSSubsystem::ReadTitleFile(FString Filename)
{
	if (TitleFilePtr == nullptr || AccountInfo == nullptr) return;

	FTitleFileReadFile::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	Params.Filename = Filename;

	TitleFilePtr->ReadFile(MoveTemp(Params))
	.OnComplete([this, Filename](const TOnlineResult<FTitleFileReadFile>& Result)
	{
		if (Result.IsOk())
		{
			OnReadTitleFileComplete.Broadcast(true, Result.GetOkValue().FileContents, Filename);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ReadTitleFile %s %s"), *Filename, *Result.GetErrorValue().GetLogString());
			OnReadTitleFileComplete.Broadcast(false, MakeShared<TArray<uint8>>(), Filename);
		}
	});
}

// 获取远程商品列表
void UEOSSubsystem::QueryOffers()
{
	if (AuthPtr == nullptr || AccountInfo == nullptr || !AuthPtr->IsLoggedIn(AccountInfo->AccountId)) return;
	if (CommercePtr == nullptr) return;
	
	FCommerceQueryOffers::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	
	CommercePtr->QueryOffers(MoveTemp(Params))
	.OnComplete([this](const TOnlineResult<FCommerceQueryOffers>& Result)
	{
		if (Result.IsOk())
		{
			OnQueryOffersComplete.Broadcast(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("QueryOffers %s"), *Result.GetErrorValue().GetLogString());
			OnQueryOffersComplete.Broadcast(false);
		}
	});
}

// 获取商品列表
TArray<FOffer> UEOSSubsystem::GetOffers()
{
	TArray<FOffer> Offers;

	if (CommercePtr == nullptr || AccountInfo == nullptr) return Offers;

	FCommerceGetOffers::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;

	TOnlineResult<FCommerceGetOffers> Result = CommercePtr->GetOffers(MoveTemp(Params));
	if (Result.IsOk())
	{
		Offers = Result.GetOkValue().Offers;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GetOffers %s"), *Result.GetErrorValue().GetLogString());
	}

	return Offers;
}

// 购买商品
void UEOSSubsystem::Checkout(TArray<FPurchaseOffer> Offers)
{
	if (CommercePtr == nullptr || AccountInfo == nullptr) return;

	FCommerceCheckout::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	Params.Offers = Offers;

	CommercePtr->Checkout(MoveTemp(Params))
	.OnComplete([this](const TOnlineResult<FCommerceCheckout>& Result)
	{
		if (Result.IsOk())
		{
			OnCheckoutComplete.Broadcast(true, Result.GetOkValue().TransactionId);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Checkout %s"), *Result.GetErrorValue().GetLogString());
			OnCheckoutComplete.Broadcast(false, FString());
		}
	});
}

void UEOSSubsystem::BroadcastOnPurchaseCompleted(const FCommerceOnPurchaseComplete& CommerceOnPurchaseComplete)
{
	OnPurchaseCompleted.Broadcast(CommerceOnPurchaseComplete);
}

// 获取远程已购商品
void UEOSSubsystem::QueryEntitlements()
{
	if (AuthPtr == nullptr || AccountInfo == nullptr || CommercePtr == nullptr) return;

	if (!AuthPtr->IsLoggedIn(AccountInfo->AccountId)) return;

	FCommerceQueryEntitlements::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;
	Params.bIncludeRedeemed = true;

	CommercePtr->QueryEntitlements(MoveTemp(Params))
	.OnComplete([this](const TOnlineResult<FCommerceQueryEntitlements>& Result)
	{
		if (Result.IsOk())
		{
			OnQueryEntitlementsComplete.Broadcast(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("QueryEntitlements %s"), *Result.GetErrorValue().GetLogString());
			OnQueryEntitlementsComplete.Broadcast(false);
		}
	});
}

// 获取已购商品
void UEOSSubsystem::GetEntitlements()
{
	if (CommercePtr == nullptr || AccountInfo == nullptr) return;

	FCommerceGetEntitlements::Params Params;
	Params.LocalAccountId = AccountInfo->AccountId;

	TOnlineResult<FCommerceGetEntitlements> Result = CommercePtr->GetEntitlements(MoveTemp(Params));
	if (Result.IsOk())
	{
		// 已购商品中的ProductId等于门户商品的AudienceItemId
		Entitlements = Result.GetOkValue().Entitlements;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GetEntitlements %s"), *Result.GetErrorValue().GetLogString());
	}
}

// 是否拥有该商品
bool UEOSSubsystem::OwnEntitlement(const FString& Id)
{
	for (int i = 0; i < Entitlements.Num(); ++i)
	{
		if (Entitlements[i].ProductId == Id)
		{
			return true;
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
