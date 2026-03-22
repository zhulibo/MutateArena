#include "HumanDNAButton.h"

#include "CommonLazyImage.h"
#include "CommonTextBlock.h"
#include "MutateArena/Characters/Data/HumanDNAAsset.h"

void UHumanDNAButton::UpdateDNAInfo(UHumanDNAAsset* DNAData)
{
	if (DNAData)
	{
		DNAType = DNAData->DNAType;

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
