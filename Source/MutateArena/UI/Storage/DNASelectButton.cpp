#include "DNASelectButton.h"

#include "CommonLazyImage.h"
#include "CommonTextBlock.h"
#include "MutateArena/Characters/Data/DNAAsset2.h"

void UDNASelectButton::Setup(UDNAAsset2* DNAData)
{
	if (!DNAData) return;

	DNA = DNAData->DNA;
	DisplayName->SetText(DNAData->DisplayName);
	Desc->SetText(DNAData->Desc);
	Icon->SetBrushFromLazyTexture(DNAData->Icon);
	
	if (DNAData->DNA == EDNA::AcceleratedMetabolism 
		|| DNAData->DNA == EDNA::ThermalRegulation
		|| DNAData->DNA == EDNA::ToxicityImmunity
		|| DNAData->DNA == EDNA::PainModulation
		|| DNAData->DNA == EDNA::CoreMaintenance)
	{
		Desc->SetText(FText::FromString(TEXT("In Development")));
		SetIsEnabled(false);
	}
}
