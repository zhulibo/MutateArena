#include "HumanDNASelectScreen.h"
#include "HumanDNASelectButton.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "MutateArena/Characters/Data/HumanDNAAsset.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"

void UHumanDNASelectScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UStorageSubsystem* StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (!StorageSubsystem) return;
	
	for (uint8 i = 0; i < static_cast<uint8>(EHumanDNA::None); ++i)
	{
		EHumanDNA CurrentType = static_cast<EHumanDNA>(i);
        
		if (UHumanDNAAsset* DNAData = StorageSubsystem->GetHumanDNAAssetByType(CurrentType))
		{
			if (UHumanDNASelectButton* NewButton = CreateWidget<UHumanDNASelectButton>(this, HumanDNASelectButtonClass))
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

UWidget* UHumanDNASelectScreen::NativeGetDesiredFocusTarget() const
{
	if (DNAContainer && DNAContainer->GetChildrenCount() > 0)
	{
		int32 MiddleIndex = DNAContainer->GetChildrenCount() / 2;
		return DNAContainer->GetChildAt(MiddleIndex);
	}
	
	return Super::NativeGetDesiredFocusTarget();
}

void UHumanDNASelectScreen::Setup(FHumanDNASelectComplete InCallback)
{
	Callback = InCallback;
}

void UHumanDNASelectScreen::OnDNAButtonClicked(UHumanDNAAsset* SelectedDNA)
{
	DeactivateWidget();

	Callback.ExecuteIfBound(SelectedDNA);
}
