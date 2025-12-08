#include "KeyBindingLineButton.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Components/InputKeySelector.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/Utils/LibraryNotify.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#define LOCTEXT_NAMESPACE "UKeyBindingLineButton"

void UKeyBindingLineButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	KeySelector_KBM->OnKeySelected.AddUniqueDynamic(this, &ThisClass::OnKBMKeySelected);
	KeySelector_Controller->OnKeySelected.AddUniqueDynamic(this, &ThisClass::OnControllerKeySelected);
}

void UKeyBindingLineButton::NativeConstruct()
{
	Super::NativeConstruct();
}

void UKeyBindingLineButton::OnKBMKeySelected(FInputChord SelectedKey)
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *SelectedKey.Key.ToString());
	RebindKey(MappingName_KBM, SelectedKey.Key);
}

void UKeyBindingLineButton::OnControllerKeySelected(FInputChord SelectedKey)
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *SelectedKey.Key.ToString());
	RebindKey(MappingName_Controller, SelectedKey.Key);
}

void UKeyBindingLineButton::SetKeySilent(const FKey& Key, bool bIsGamepad)
{
	if (bIsGamepad)
	{
		KeySelector_Controller->OnKeySelected.RemoveDynamic(this, &ThisClass::OnControllerKeySelected);
		KeySelector_Controller->SetSelectedKey(Key);
		KeySelector_Controller->OnKeySelected.AddUniqueDynamic(this, &ThisClass::OnControllerKeySelected);
	}
	else
	{
		KeySelector_KBM->OnKeySelected.RemoveDynamic(this, &ThisClass::OnKBMKeySelected);
		KeySelector_KBM->SetSelectedKey(Key);
		KeySelector_KBM->OnKeySelected.AddUniqueDynamic(this, &ThisClass::OnKBMKeySelected);
	}
}

void UKeyBindingLineButton::RebindKey(FName MappingName, FKey NewKey)
{
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(GetOwningPlayer());
	if (BaseController == nullptr) return;
	
	if (EISubsystem == nullptr) EISubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(BaseController->GetLocalPlayer());
	if (EISubsystem == nullptr) return;

	if (UserSettings == nullptr) UserSettings = EISubsystem->GetUserSettings();
	if (UserSettings == nullptr) return;
	
	FMapPlayerKeyArgs Args;
	Args.MappingName = MappingName;
	Args.Slot = EPlayerMappableKeySlot::First;
	Args.NewKey = NewKey;

	FGameplayTagContainer FailureReason;
	UserSettings->MapPlayerKey(Args, FailureReason);
	
	if (FailureReason.IsEmpty())
	{
		UserSettings->ApplySettings();
		UserSettings->SaveSettings();
	}
	else
	{
		NOTIFY(this, C_RED, LOCTEXT("RebindFailed", "Rebind Failed"));
		
		UE_LOG(LogTemp, Warning, TEXT("RebindFailed: %s"), *FailureReason.ToString());
	}
}

#undef LOCTEXT_NAMESPACE
