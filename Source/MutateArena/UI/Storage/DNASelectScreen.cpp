#include "DNASelectScreen.h"
#include "DNASelectButton.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/GameInstance.h"
#include "MutateArena/Characters/Data/DNAAsset2.h"
#include "MutateArena/System/AssetSubsystem.h"

void UDNASelectScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UAssetSubsystem* AssetSubsystem = GetGameInstance()->GetSubsystem<UAssetSubsystem>())
	{
		for (uint8 i = 0; i < static_cast<uint8>(EDNA2::None); ++i)
		{
			EDNA2 CurrentType = static_cast<EDNA2>(i);
        
			if (UDNAAsset2* DNAData = AssetSubsystem->GetDNAAssetByType(CurrentType))
			{
				if (UDNASelectButton* NewButton = CreateWidget<UDNASelectButton>(this, DNASelectButtonClass))
				{
					NewButton->Setup(DNAData);
					NewButton->OnClicked().Clear();
					NewButton->OnClicked().AddUObject(this, &ThisClass::OnDNAButtonClicked, DNAData);
                
					if (UWrapBoxSlot* NewSlot = Cast<UWrapBoxSlot>(DNAContainer->AddChild(NewButton)))
					{
						NewSlot->SetPadding(FMargin(20, 20, 0, 0));
					}
				}
			}
		}
	}
}

UWidget* UDNASelectScreen::NativeGetDesiredFocusTarget() const
{
	if (DNAContainer && DNAContainer->GetChildrenCount() > 0)
	{
		int32 MiddleIndex = DNAContainer->GetChildrenCount() / 2;
		return DNAContainer->GetChildAt(MiddleIndex);
	}
	
	return Super::NativeGetDesiredFocusTarget();
}

void UDNASelectScreen::Setup(FDNASelectComplete InCallback)
{
	Callback = InCallback;
}

void UDNASelectScreen::OnDNAButtonClicked(UDNAAsset2* SelectedDNA)
{
	DeactivateWidget();

	Callback.ExecuteIfBound(SelectedDNA);
}
