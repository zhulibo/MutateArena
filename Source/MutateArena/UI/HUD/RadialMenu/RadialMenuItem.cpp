#include "RadialMenuItem.h"

#include "CommonLazyImage.h"
#include "CommonTextBlock.h"
#include "RadialMenuBase.h"
#include "MutateArena/MutateArena.h"

void URadialMenuItem::InitData(const FRadialMenuItemData& Data)
{
	ItemName->SetText(Data.ItemText);
	
	// 检查软指针是否有效（是否配置了贴图资源）
	if (!Data.ItemTexture.IsNull())
	{
		// 使用异步加载贴图的方法
		ItemImage->SetBrushFromLazyTexture(Data.ItemTexture, true); // 尺寸必须重置
		ItemImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		// 如果两个软指针都是空的，隐藏图片控件
		ItemImage->SetVisibility(ESlateVisibility::Collapsed); 
	}
    
	SetSelectionState(false);
}

void URadialMenuItem::SetSelectionState(bool bIsSelected)
{
	// FLinearColor TargetColor = bIsSelected ? C_WHITE : C_WHITE;
	// ItemName->SetColorAndOpacity(FSlateColor(TargetColor));
	// ItemImage->SetColorAndOpacity(TargetColor);
}
