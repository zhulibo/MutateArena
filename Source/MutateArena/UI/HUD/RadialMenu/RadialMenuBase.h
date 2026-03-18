#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "RadialMenuBase.generated.h"

USTRUCT(BlueprintType)
struct FRadialMenuItemData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FText ItemText;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UTexture2D> ItemTexture;
};

UCLASS()
class MUTATEARENA_API URadialMenuBase : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void BuildMenu(const TArray<FRadialMenuItemData>& ItemsData);

	// 缓存当前的色块总数
	int32 SegmentCount = 0;
	int32 GetSegmentCount() const { return SegmentCount; }

	UFUNCTION()
	void UpdatePointerInput(double X, double Y);

	UPROPERTY()
	int32 SelectedItemIndex = -1;
	UFUNCTION()
	void SetSelectedItem(int32 Index);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	class UImage* BgImage;
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicBgMaterial;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class URadialMenuItem> ItemWidgetClass;

	UPROPERTY()
	float Radius = 230.f;

	UPROPERTY(meta = (BindWidget))
	class UCanvasPanel* ContainerCanvas;

	UPROPERTY()
	TArray<URadialMenuItem*> SpawnedItems;

	// 指针
	UPROPERTY(meta = (BindWidget))
	UImage* PointerImage;
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicPointerMat;
	UPROPERTY()
	float MaxPointerRadius = 150.0f;
	UPROPERTY()
	float DeadzoneThreshold = 0.6f;
	// 迟滞缓冲值（当已经选中某项时，死区阈值降低多少，防止在边界闪烁） 比如：推到 0.6 才能进去，但退回到 0.55 才会出来
	UPROPERTY()
	float DeadzoneHysteresis = 0.05f;
	// 虚拟游标，用来“记住”鼠标拖动后的位置，维持指向
	FVector2D VirtualCursor = FVector2D::ZeroVector;
	UPROPERTY()
	float PointerSensitivity = .1f;

public:
	void ResetPointerInput();
	void SetPointerColor(FLinearColor NewColor);
	void SetSegmentColor(FLinearColor NewColor);
	
};
