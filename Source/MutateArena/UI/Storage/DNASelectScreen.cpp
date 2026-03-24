#include "DNASelectScreen.h"
#include "DNASelectButton.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "MutateArena/Characters/Data/DNAAsset2.h"
#include "MutateArena/System/Storage/StorageSubsystem.h"

void UDNASelectScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UStorageSubsystem* StorageSubsystem = GetGameInstance()->GetSubsystem<UStorageSubsystem>();
	if (!StorageSubsystem) return;
	
	for (uint8 i = 0; i < static_cast<uint8>(EDNA::None); ++i)
	{
		EDNA CurrentType = static_cast<EDNA>(i);
        
		if (UDNAAsset2* DNAData = StorageSubsystem->GetDNAAssetByType(CurrentType))
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
