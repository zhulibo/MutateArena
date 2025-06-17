#include "MultiCultureSubsystem.h"

void UMultiCultureSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 统一进行翻译，供FText::FindTextInLiveTable_Advanced查找使用。

	// 需与 FHumanCharacterMain 中的ShowName一致
#define LOCTEXT_NAMESPACE "Human"
	LOCTEXT("Solider", "Solider");
	LOCTEXT("Doctor", "Doctor");
	LOCTEXT("Test", "Test");
#undef LOCTEXT_NAMESPACE

	// 需与 FMutantCharacterMain 中的ShowName一致
#define LOCTEXT_NAMESPACE "Mutant"
	LOCTEXT("Tank", "Tank");
	LOCTEXT("Ghost", "Ghost");
	LOCTEXT("Smoker", "Smoker");
	LOCTEXT("Cutter", "Cutter");
#undef LOCTEXT_NAMESPACE

	// 需与 FEquipmentMain 中的ShowName一致
#define LOCTEXT_NAMESPACE "Equipment"
	LOCTEXT("AK47", "AK47");
	LOCTEXT("AK47 Desert", "AK47 Desert");
	LOCTEXT("M870", "M870");
	LOCTEXT("M60", "M60");
	LOCTEXT("PKM", "PKM");

	LOCTEXT("Glock17", "Glock17");
	LOCTEXT("Desert Eagle", "Desert Eagle");

	LOCTEXT("Kukri", "Kukri");
	LOCTEXT("Military Shovel", "Military Shovel");
	LOCTEXT("Fire Axe", "Fire Axe");
	
	LOCTEXT("Grenade", "Grenade");
	LOCTEXT("Flashbang", "Flashbang");
	LOCTEXT("Smoke", "Smoke");
	LOCTEXT("Fire Bottle", "Fire Bottle");

	LOCTEXT("AK47 Cyclone", "AK47 Cyclone");
	LOCTEXT("MG42", "MG42");
	LOCTEXT("GM6 Lynx", "GM6 Lynx");
#undef LOCTEXT_NAMESPACE

	// 需与 EEquipmentType 的值一致
#define LOCTEXT_NAMESPACE "EquipmentType"
	LOCTEXT("Primary", "Primary");
	LOCTEXT("Secondary", "Secondary");
	LOCTEXT("Melee", "Melee");
	LOCTEXT("Throwing", "Throwing");
#undef LOCTEXT_NAMESPACE

}
