#include "DebugMenu.h"

#include "CommonTextBlock.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Maps/Environment/EnvironmentManager.h"
#include "MutateArena/UI/HUD/Mutation/MutationContainer.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/System/GameplayTags.h"
#include "MutateArena/UI/Common/CommonButton.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "MutateArena/Utils/LibraryNotify.h"

#define LOCTEXT_NAMESPACE "UDebugMenu"

void UDebugMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	OnActivated().AddWeakLambda(this, [this]() {
		bWantToBack = true;
	});

	OnDeactivated().AddUObject(this, &ThisClass::OnDeactivatedInternal);

	// Environment
	EnvButton_AcidRain->ButtonText->SetText(FText::FromString(TEXT("AcidRain")));
	EnvButton_AcidRain->OnClicked().AddUObject(this, &ThisClass::OnEnvButtonClicked, FGameplayTag(TAG_STATE_ENV_ACIDRAIN));
	
	EnvButton_BloodMoon->ButtonText->SetText(FText::FromString(TEXT("BloodMoon")));
	EnvButton_BloodMoon->OnClicked().AddUObject(this, &ThisClass::OnEnvButtonClicked, FGameplayTag(TAG_STATE_ENV_BLOODMOON));
	
	EnvButton_Fog->ButtonText->SetText(FText::FromString(TEXT("Fog")));
	EnvButton_Fog->OnClicked().AddUObject(this, &ThisClass::OnEnvButtonClicked, FGameplayTag(TAG_STATE_ENV_FOG));
	
	EnvButton_Snow->ButtonText->SetText(FText::FromString(TEXT("Snow")));
	EnvButton_Snow->OnClicked().AddUObject(this, &ThisClass::OnEnvButtonClicked, FGameplayTag(TAG_STATE_ENV_SNOW));
}

UWidget* UDebugMenu::NativeGetDesiredFocusTarget() const
{
	return EnvButton_BloodMoon;
}

void UDebugMenu::OnDeactivatedInternal()
{
	// 进入二级菜单时不需要FocusGame
	if (!bWantToBack)
	{
		return;
	}

	// 返回游戏时FocusGame
	if (BaseController == nullptr) BaseController = Cast<ABaseController>(GetOwningPlayer());
	if (BaseController)
	{
		BaseController->FocusGame();
	}
}

void UDebugMenu::OnEnvButtonClicked(FGameplayTag EnvironmentTag)
{
	if (GetOwningPlayer() == nullptr || !GetOwningPlayer()->HasAuthority())
	{
		NOTIFY(this, Colors::Yellow, FText::FromString(TEXT("Only can change environment on host!")));
		return;
	}
	
	if (ABaseGameState* BaseGameState = GetWorld()->GetGameState<ABaseGameState>())
	{
		if (AEnvironmentManager* EnvManager = BaseGameState->EnvironmentManager)
		{
			if (EnvManager->IsEnvironmentActive(EnvironmentTag))
			{
				EnvManager->DeactivateEnvironment(EnvironmentTag);
			}
			else
			{
				EnvManager->ActivateEnvironment(EnvironmentTag);
			}
		}
	}
	
	DeactivateWidget();
}

#undef LOCTEXT_NAMESPACE
