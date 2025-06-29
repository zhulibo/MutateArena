#include "MAGameInstance.h"

#include "EOSSubsystem.h"
#include "MutateArena/MutateArena.h"

void UMAGameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UMAGameInstance::OnNetworkFailure);
	}
}

void UMAGameInstance::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type Type, const FString& String)
{
	UE_LOG(LogTemp, Warning, TEXT("OnNetworkFailure Type %d String: %s -----------------------------------------------"), Type, *String);

	// 大概率是房主对局中直接退出房间
	if (Type == ENetworkFailure::ConnectionLost)
	{
		// 则将当前玩家设置为未准备状态
		if (EOSSubsystem == nullptr) EOSSubsystem = GetSubsystem<UEOSSubsystem>();
		if (EOSSubsystem)
		{
			EOSSubsystem->ModifyLobbyMemberAttr(TMap<FSchemaAttributeId, FSchemaVariant>{
				{ LOBBY_MEMBER_READY, false}
			});
		}
	}
}
