#include "MultiCultureSubsystem.h"

void UMultiCultureSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 统一进行翻译，供FText::FindTextInLiveTable_Advanced查找使用。

	// 需与 EEquipmentType 的值一致
#define LOCTEXT_NAMESPACE "EquipmentType"
	auto Primary = LOCTEXT("Primary", "Primary");
	auto Secondary = LOCTEXT("Secondary", "Secondary");
	auto Melee = LOCTEXT("Melee", "Melee");
	auto Throwing = LOCTEXT("Throwing", "Throwing");
#undef LOCTEXT_NAMESPACE

}
