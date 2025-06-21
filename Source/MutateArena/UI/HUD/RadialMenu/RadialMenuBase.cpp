#include "RadialMenuBase.h"

#include "CommonTextBlock.h"
#include "MutateArena/MutateArena.h"

void URadialMenuBase::SetSelectedItem(int32 Index)
{
	if (Index == SelectedItemIndex) return;

	switch (Index)
	{
	case 0:
		Item1->SetColorAndOpacity(C_GREEN);
		break;
	case 1:
		Item2->SetColorAndOpacity(C_GREEN);
		break;
	case 2:
		Item3->SetColorAndOpacity(C_GREEN);
		break;
	case 3:
		Item4->SetColorAndOpacity(C_GREEN);
		break;
	case 4:
		Item5->SetColorAndOpacity(C_GREEN);
		break;
	case 5:
		Item6->SetColorAndOpacity(C_GREEN);
		break;
	case 6:
		Item7->SetColorAndOpacity(C_GREEN);
		break;
	case 7:
		Item8->SetColorAndOpacity(C_GREEN);
		break;
	}

	switch (SelectedItemIndex)
	{
	case 0:
		Item1->SetColorAndOpacity(C_WHITE);
		break;
	case 1:
		Item2->SetColorAndOpacity(C_WHITE);
		break;
	case 2:
		Item3->SetColorAndOpacity(C_WHITE);
		break;
	case 3:
		Item4->SetColorAndOpacity(C_WHITE);
		break;
	case 4:
		Item5->SetColorAndOpacity(C_WHITE);
		break;
	case 5:
		Item6->SetColorAndOpacity(C_WHITE);
		break;
	case 6:
		Item7->SetColorAndOpacity(C_WHITE);
		break;
	case 7:
		Item8->SetColorAndOpacity(C_WHITE);
		break;
	}
	
	LastSelectedItemIndex = SelectedItemIndex;
	SelectedItemIndex = Index;
}
