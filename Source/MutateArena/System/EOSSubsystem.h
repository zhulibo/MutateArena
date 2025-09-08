#pragma once

#include "CoreMinimal.h"
#include "Online/OnlineServices.h"
#include "Online/Auth.h"
#include "Online/UserInfo.h"
#include "Online/Lobbies.h"
#include "Online/UserFile.h"
#include "Online/TitleFile.h"
#include "Online/Commerce.h"
#include "Online/OnlineAsyncOpHandle.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EOSSubsystem.generated.h"

enum class ETeam : uint8;

UENUM(BlueprintType)
enum class ECoolLoginType : uint8
{
	ExchangeCode,
	Password,
	DevAuth,
};

using namespace UE::Online;

// Login
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoginComplete, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoginStatusChanged, const FAuthLoginStatusChanged& AuthLoginStatusChanged);

// Lobby
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCreateLobbyComplete, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnFindLobbiesComplete, bool bWasSuccessful, const TArray<TSharedRef<const FLobby>>& Lobbies);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnJoinLobbyComplete, bool bWasSuccessful);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyInvitationAdded, const FLobbyInvitationAdded& LobbyInvitationAdded);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUILobbyJoinRequested, const FUILobbyJoinRequested& UILobbyJoinRequested);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyJoined, const FLobbyJoined& LobbyJoined);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyMemberJoined, const FLobbyMemberJoined& LobbyMemberJoined);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyMemberLeft, const FLobbyMemberLeft& LobbyMemberLeft);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPromoteLobbyMemberComplete, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyLeaderChanged, const FLobbyLeaderChanged& LobbyLeaderChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnKickLobbyMemberComplete, bool bWasSuccessful);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnModifyLobbyAttrComplete, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyAttrChanged, const FLobbyAttributesChanged& LobbyAttrChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnModifyLobbyMemberAttrComplete, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyMemberAttrChanged, const FLobbyMemberAttributesChanged& LobbyMemberAttrChanged);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLobbyLeft, const FLobbyLeft& LobbyLeft);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLeaveLobbyComplete, bool bWasSuccessful);

// UserFile
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEnumerateUserFilesComplete, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnReadUserFileComplete, bool bWasSuccessful, const FUserFileContentsRef& FileContents);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnWriteUserFileComplete, bool bWasSuccessful);

// TitleFile
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEnumerateTitleFilesComplete, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnReadTitleFileComplete, bool bWasSuccessful, const FTitleFileContentsRef& FileContents);

// Commerce
DECLARE_MULTICAST_DELEGATE_OneParam(FOnQueryOffersComplete, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCheckoutComplete, bool bWasSuccessful, TOptional<FString> TransactionId);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPurchaseCompleted, const FCommerceOnPurchaseComplete& CommerceOnPurchaseComplete);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnQueryEntitlementsComplete, bool bWasSuccessful);

UCLASS()
class MUTATEARENA_API UEOSSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UEOSSubsystem();
	
	virtual void Deinitialize() override;

	TArray<FOnlineEventDelegateHandle> OnlineEventDelegateHandles;

	// Login
	void Login(FPlatformUserId TempPlatformUserId, ECoolLoginType LoginType, FString Id, FString Token);
	FOnLoginComplete OnLoginComplete;
	TSharedPtr<FAccountInfo> AccountInfo;
	void GetAccountInfo(FPlatformUserId Id);
	TSharedPtr<FUserInfo> UserInfo;
	void QueryUserInfo();
	void GetUserInfo();
	FOnLoginStatusChanged OnLoginStatusChanged;
	void BroadcastOnLoginStatusChanged(const FAuthLoginStatusChanged& AuthLoginStatusChanged);

	FAccountId GetAccountId();
	FString GetPlayerName();

	// Lobby
	TSharedPtr<const FLobby> CurLobby;
	FName LocalLobbyName = TEXT("LocalLobbyName");
	void CreateLobby();
	FOnCreateLobbyComplete OnCreateLobbyComplete;
	void FindLobbies(FString LobbyName, FString GameMode, FString MapName);
	FOnFindLobbiesComplete OnFindLobbiesComplete;
	void JoinLobby(TSharedPtr<const FLobby> Lobby);
	FOnJoinLobbyComplete OnJoinLobbyComplete;
	bool GetJoinedLobbies();
	UPROPERTY()
	FTimerHandle TickNumTimerHandle;
	void ChangeLobbyMemberTickNum();

	FOnLobbyInvitationAdded OnLobbyInvitationAdded;
	void BroadcastOnLobbyInvitationAdded(const FLobbyInvitationAdded& LobbyInvitationAdded);
	FOnUILobbyJoinRequested OnUILobbyJoinRequested;
	void BroadcastOnUILobbyJoinRequested(const FUILobbyJoinRequested& UILobbyJoinRequested);

	FOnLobbyJoined OnLobbyJoined;
	void BroadcastOnLobbyJoined(const FLobbyJoined& LobbyJoined);
	FOnLobbyMemberJoined OnLobbyMemberJoined;
	void BroadcastOnLobbyMemberJoined(const FLobbyMemberJoined& LobbyMemberJoined);
	FOnLobbyMemberLeft OnLobbyMemberLeft;
	void BroadcastOnLobbyMemberLeft(const FLobbyMemberLeft& LobbyMemberLeft);

	void PromoteLobbyMember(FAccountId TargetAccountId);
	FOnPromoteLobbyMemberComplete OnPromoteLobbyMemberComplete;
	FOnLobbyLeaderChanged OnLobbyLeaderChanged;
	void BroadcastOnLobbyLeaderChanged(const FLobbyLeaderChanged& LobbyLeaderChanged);
	void KickLobbyMember(FAccountId TargetAccountId);
	FOnKickLobbyMemberComplete OnKickLobbyMemberComplete;

	void ModifyLobbyAttr(TMap<FSchemaAttributeId, FSchemaVariant> UpdatedAttr = TMap<FSchemaAttributeId, FSchemaVariant>(),
		TSet<FSchemaAttributeId> RemovedAttr = TSet<FSchemaAttributeId>());
	FOnModifyLobbyAttrComplete OnModifyLobbyAttrComplete;
	FOnLobbyAttrChanged OnLobbyAttrChanged;
	void BroadcastOnLobbyAttrChanged(const FLobbyAttributesChanged& LobbyAttrChanged);
	void ModifyLobbyMemberAttr(TMap<FSchemaAttributeId, FSchemaVariant> UpdatedAttr = TMap<FSchemaAttributeId, FSchemaVariant>(),
		TSet<FSchemaAttributeId> RemovedAttr = TSet<FSchemaAttributeId>());
	FOnModifyLobbyMemberAttrComplete OnModifyLobbyMemberAttrComplete;
	FOnLobbyMemberAttrChanged OnLobbyMemberAttrChanged;
	void BroadcastOnLobbyMemberAttrChanged(const FLobbyMemberAttributesChanged& LobbyMemberAttrChanged);

	void GetResolvedConnectString(FString& Url);

	FOnLobbyLeft OnLobbyLeft;
	void BroadcastOnLobbyLeft(const FLobbyLeft& LobbyLeft);
	void LeaveLobby();
	FOnLeaveLobbyComplete OnLeaveLobbyComplete;

	FString GetLobbyServerName();
	FString GetLobbyModeName();
	FString GetLobbyMapName();
	// Status代表游戏当前回合数，未开始时是0
	int64 GetLobbyStatus();

	TSharedPtr<const FLobbyMember> GetMember(FAccountId AccountId);
	TSharedPtr<const FLobbyMember> GetMemberByPlayerName(FString PlayerName);

	ETeam GetMemberTeam(TSharedPtr<const FLobbyMember> Member);
	bool GetMemberReady(TSharedPtr<const FLobbyMember> Member);
	FString GetMemberName(TSharedPtr<const FLobbyMember> Member);
	FString GetMemberMsg(TSharedPtr<const FLobbyMember> Member);

	bool IsLobbyHost(TSharedPtr<const FLobbyMember> Member = nullptr);
	TSharedPtr<const FLobbyMember> GetLobbyHost();
	bool IsLocalMember(TSharedPtr<const FLobbyMember> Member);
	TSharedPtr<const FLobbyMember> GetLocalMember();

	// UserFile
	void EnumerateUserFiles();
	FOnEnumerateUserFilesComplete OnEnumerateUserFilesComplete;
	TArray<FString> GetEnumeratedUserFiles();
	FOnReadUserFileComplete OnReadUserFileComplete;
	void ReadUserFile(FString Filename);
	FOnWriteUserFileComplete OnWriteUserFileComplete;
	void WriteUserFile(FString Filename, FUserFileContents FileContents);

	// TitleFile
	void EnumerateTitleFiles();
	FOnEnumerateTitleFilesComplete OnEnumerateTitleFilesComplete;
	TArray<FString> GetEnumeratedTitleFiles();
	FOnReadTitleFileComplete OnReadTitleFileComplete;
	void ReadTitleFile(FString Filename);
	
	// Commerce
	void QueryOffers();
	FOnQueryOffersComplete OnQueryOffersComplete;
	TArray<FOffer> GetOffers();
	void Checkout(TArray<FPurchaseOffer> Offers);
	FOnCheckoutComplete OnCheckoutComplete;
	void BroadcastOnPurchaseCompleted(const FCommerceOnPurchaseComplete& CommerceOnPurchaseComplete);
	FOnPurchaseCompleted OnPurchaseCompleted;
	void QueryEntitlements();
	FOnQueryEntitlementsComplete OnQueryEntitlementsComplete;
	TArray<FEntitlement> Entitlements;
	void GetEntitlements();
	bool OwnEntitlement(const FString& Id);

protected:
	FPlatformUserId PlatformUserId;

	IOnlineServicesPtr OnlineServicesPtr;
	IAuthPtr AuthPtr;
	IUserInfoPtr UserInfoPtr;
	ILobbiesPtr LobbyPtr;
	IUserFilePtr UserFilePtr;
	ITitleFilePtr TitleFilePtr;
	ICommercePtr CommercePtr;
	
};
