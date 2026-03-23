#include "HumanDNASelectButton.h"

#include "CommonLazyImage.h"
#include "CommonTextBlock.h"
#include "MutateArena/Characters/Data/HumanDNAAsset.h"

void UHumanDNASelectButton::Setup(UHumanDNAAsset* DNAData)
{
	if (!DNAData) return;

	DNAType = DNAData->DNAType;
	DisplayName->SetText(DNAData->DisplayName);
	Desc->SetText(DNAData->Desc);
	Icon->SetBrushFromLazyTexture(DNAData->Icon);
	
	if (DNAData->DNAType == EHumanDNA::AcceleratedMetabolism 
		|| DNAData->DNAType == EHumanDNA::ThermalRegulation
		|| DNAData->DNAType == EHumanDNA::ToxicityImmunity
		|| DNAData->DNAType == EHumanDNA::PainModulation
		|| DNAData->DNAType == EHumanDNA::CoreMaintenance)
	{
		Desc->SetText(FText::FromString(TEXT("In Development")));
		SetIsEnabled(false);
	}
}
