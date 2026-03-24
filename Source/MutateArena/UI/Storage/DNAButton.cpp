#include "DNAButton.h"

#include "CommonLazyImage.h"
#include "CommonTextBlock.h"
#include "MutateArena/Characters/Data/DNAAsset2.h"

void UDNAButton::UpdateDNAInfo(UDNAAsset2* DNAData)
{
	if (DNAData)
	{
		DNA = DNAData->DNA;

		if (DisplayName)
		{
			DisplayName->SetText(DNAData->DisplayName);
		}

		if (Icon && DNAData->Icon)
		{
			Icon->SetBrushFromLazyTexture(DNAData->Icon);
		}
	}
}
