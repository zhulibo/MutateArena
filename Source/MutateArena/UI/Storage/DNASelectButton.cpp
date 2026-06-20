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
	
	if (DNAData->DNA == EDNA2::AcceleratedMetabolism 
		|| DNAData->DNA == EDNA2::ThermalRegulation
		|| DNAData->DNA == EDNA2::ToxicityImmunity
		|| DNAData->DNA == EDNA2::PainModulation
		|| DNAData->DNA == EDNA2::CoreMaintenance)
	{
		Desc->SetText(FText::FromString(TEXT("In Development")));
		SetIsEnabled(false);
	}
}
