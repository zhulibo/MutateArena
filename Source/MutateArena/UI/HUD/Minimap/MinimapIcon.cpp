#include "MinimapIcon.h"
#include "Components/Image.h"

void UMinimapIcon::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IconImage && IconImage->GetDynamicMaterial())
	{
		IconMID = IconImage->GetDynamicMaterial();
	}
}

void UMinimapIcon::UpdateIconAppearance(bool bIsSameFloor, const FLinearColor& InColor)
{
	if (!IconMID)
	{
		if (IconImage && IconImage->GetDynamicMaterial())
		{
			IconMID = IconImage->GetDynamicMaterial();
		}
	}

	if (IconMID)
	{
		// （1.0 实心, 0.0 空心）
		IconMID->SetScalarParameterValue(FName("IsSameFloor"), bIsSameFloor ? 1.0f : 0.0f);
		
		IconMID->SetVectorParameterValue(FName("Color"), InColor);
	}
}
