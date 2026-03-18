#include "RadialMenuBase.h"

#include "RadialMenuItem.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"

void URadialMenuBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BgImage)
	{
		DynamicBgMaterial = BgImage->GetDynamicMaterial();
	}

	if (PointerImage)
	{
		DynamicPointerMat = PointerImage->GetDynamicMaterial();
	}
}

void URadialMenuBase::BuildMenu(const TArray<FRadialMenuItemData>& ItemsData)
{
	if (!ItemWidgetClass || !ContainerCanvas) return;

	// 清理旧数据
	ContainerCanvas->ClearChildren();
	SpawnedItems.Empty();

	// 1. 获取并缓存当前的动态总数
	SegmentCount = ItemsData.Num();
	if (SegmentCount == 0) return;

	// 2. 动态修改底板材质的“色块总数”参数！
	if (DynamicBgMaterial)
	{
		DynamicBgMaterial->SetScalarParameterValue(FName("SegmentCount"), static_cast<float>(SegmentCount));
	}

	// 算好每个切片占多少度 (例如 8个就是 45度)
	float AngleStep = 360.0f / SegmentCount;

	for (int32 i = 0; i < SegmentCount; ++i)
	{
		// 1. 实例化子 Widget
		URadialMenuItem* NewItem = CreateWidget<URadialMenuItem>(this, ItemWidgetClass);
		if (!NewItem) continue;

		// 2. 注入数据 (文字和图片)
		NewItem->InitData(ItemsData[i]);

		// 3. 添加到画布并获取 Slot
		if (UCanvasPanelSlot* CanvasSlot = ContainerCanvas->AddChildToCanvas(NewItem))
		{
			// --- 核心数学：计算圆周坐标 ---
			// UE里屏幕 Y 轴是向下的。为了让 0 号元素在正上方(12点钟)，起始角度必须减去 90 度
			float AngleDeg = (i * AngleStep) - 90.0f;
			float AngleRad = FMath::DegreesToRadians(AngleDeg);

			// 极坐标转直角坐标
			float X = Radius * FMath::Cos(AngleRad);
			float Y = Radius * FMath::Sin(AngleRad);

			// 4. 设置对齐和位置
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f)); // 确保锚点在子 Widget 自己的正中心
			CanvasSlot->SetAnchors(FAnchors(0.5f)); // 确保基于父容器(Canvas)的正中心定位
			CanvasSlot->SetPosition(FVector2D(X, Y)); // 设置算出来的圆周位置
			CanvasSlot->SetAutoSize(true); // 让文字和图片自适应大小
		}

		SpawnedItems.Add(NewItem);
	}
}

// 【新增】：清空记录（务必在打开轮盘时调用）
void URadialMenuBase::ResetPointerInput()
{
	VirtualCursor = FVector2D::ZeroVector;
	SetSelectedItem(-1);

	// 【修改】：清零材质长度参数，并隐藏图片
	if (DynamicPointerMat)
	{
		DynamicPointerMat->SetScalarParameterValue(FName("PointerLength"), 0.0f);
	}
}

void URadialMenuBase::UpdatePointerInput(double X, double Y)
{
	// 保持不变：-= 确保你的视觉指针“指哪打哪”
	VirtualCursor.X += X * PointerSensitivity;
	VirtualCursor.Y -= Y * PointerSensitivity;

	float Magnitude = VirtualCursor.Size();
	if (Magnitude > 1.0f)
	{
		VirtualCursor /= Magnitude;
		Magnitude = 1.0f;
	}

	// ================= 1. 视觉角度 =================
	// 传给材质的真实角度，让指针在屏幕上画得正确
	float VisualAngle = FMath::Atan2(VirtualCursor.Y, VirtualCursor.X) * 180.0f / PI;

	if (DynamicPointerMat)
	{
		DynamicPointerMat->SetScalarParameterValue(FName("PointerAngle"), VisualAngle);
		DynamicPointerMat->SetScalarParameterValue(FName("PointerLength"), Magnitude);
	}

	// ==============================================

	float CurrentDeadzone = (SelectedItemIndex == -1) ? DeadzoneThreshold : (DeadzoneThreshold - DeadzoneHysteresis);

	// ================= 2. 逻辑角度 =================
	if (Magnitude >= CurrentDeadzone && SegmentCount > 0)
	{
		// 【终极魔法】：为了迎合底层 UI 数组色块的方向，我们在这里单独给角度加上负号！
		float LogicAngle = -VisualAngle;

		float AngleStep = 360.0f / SegmentCount;
		float Offset = 90.0f + (AngleStep / 2.0f);

		// 注意这里改成了用 LogicAngle 去算
		float NormalizedAngle = FMath::Fmod(LogicAngle + Offset + 360.0f, 360.0f);
		int32 TargetIndex = FMath::FloorToInt(NormalizedAngle / AngleStep) % SegmentCount;

		SetSelectedItem(TargetIndex);
	}
	else
	{
		SetSelectedItem(-1);
	}
}

void URadialMenuBase::SetSelectedItem(int32 Index)
{
	if (Index == SelectedItemIndex) return;

	// 恢复旧选项
	if (SpawnedItems.IsValidIndex(SelectedItemIndex))
	{
		SpawnedItems[SelectedItemIndex]->SetSelectionState(false);
	}

	// 高亮新选项
	if (SpawnedItems.IsValidIndex(Index))
	{
		SpawnedItems[Index]->SetSelectionState(true);
	}

	if (DynamicBgMaterial)
	{
		DynamicBgMaterial->SetScalarParameterValue(FName("SelectedIndex"), static_cast<float>(Index));
	}

	SelectedItemIndex = Index;
}

void URadialMenuBase::SetPointerColor(FLinearColor NewColor)
{
	// 确保动态材质实例存在（防空指针保护）
	if (!DynamicPointerMat && PointerImage)
	{
		DynamicPointerMat = PointerImage->GetDynamicMaterial();
	}

	if (DynamicPointerMat)
	{
		DynamicPointerMat->SetVectorParameterValue(FName("PointerColor"), NewColor);
	}
}

void URadialMenuBase::SetSegmentColor(FLinearColor NewColor)
{
	if (DynamicBgMaterial)
	{
		NewColor.A = 0.3f;
		DynamicBgMaterial->SetVectorParameterValue(TEXT("SegmentColor"), NewColor);
	}
}
