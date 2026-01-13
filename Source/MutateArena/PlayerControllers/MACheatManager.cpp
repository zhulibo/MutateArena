#include "MACheatManager.h"

#include "MutateArena/PlayerControllers/BaseController.h"

void UMACheatManager::GiveEquipment(FString EquipmentName)
{
	if (ABaseController* BaseController = Cast<ABaseController>(GetOuterAPlayerController()))
	{
		BaseController->GiveEquipment(EquipmentName);
	}
}

void UMACheatManager::SwitchTeam(int32 TeamNum)
{
	UE_LOG(LogTemp, Warning, TEXT("SwitchTeam %d"), TeamNum);
}
