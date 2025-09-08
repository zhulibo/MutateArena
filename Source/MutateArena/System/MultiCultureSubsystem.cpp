#include "MultiCultureSubsystem.h"

void UMultiCultureSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 统一进行翻译，供FText::FindTextInLiveTable_Advanced查找使用。

	// 需与 FHumanCharacterMain 中的ShowName一致
#define LOCTEXT_NAMESPACE "Human"
	auto Solider = LOCTEXT("Solider", "Solider");
	auto Doctor = LOCTEXT("Doctor", "Doctor");
#undef LOCTEXT_NAMESPACE

	// 需与 FMutantCharacterMain 中的ShowName一致
#define LOCTEXT_NAMESPACE "Mutant"
	auto Tank = LOCTEXT("Tank", "Tank");
	auto Ghost = LOCTEXT("Ghost", "Ghost");
	auto Smoker = LOCTEXT("Smoker", "Smoker");
	auto Cutter = LOCTEXT("Cutter", "Cutter");
#undef LOCTEXT_NAMESPACE

	// 需与 FEquipmentMain 中的ShowName一致
#define LOCTEXT_NAMESPACE "Equipment"
	auto AK47 = LOCTEXT("AK47", "AK47");
	auto AK47Desert = LOCTEXT("AK47 Desert", "AK47 Desert");
	auto M870 = LOCTEXT("M870", "M870");
	auto M60 = LOCTEXT("M60", "M60");
	auto PKM = LOCTEXT("PKM", "PKM");

	auto Glock17 = LOCTEXT("Glock17", "Glock17");
	auto DesertEagle = LOCTEXT("Desert Eagle", "Desert Eagle");

	auto Kukri = LOCTEXT("Kukri", "Kukri");
	auto MilitaryShovel = LOCTEXT("Military Shovel", "Military Shovel");
	auto FireAxe = LOCTEXT("Fire Axe", "Fire Axe");
	
	auto Grenade = LOCTEXT("Grenade", "Grenade");
	auto Flashbang = LOCTEXT("Flashbang", "Flashbang");
	auto Smoke = LOCTEXT("Smoke", "Smoke");
	auto FireBottle = LOCTEXT("Fire Bottle", "Fire Bottle");

	auto AK47Cyclone = LOCTEXT("AK47 Cyclone", "AK47 Cyclone");
	auto MG42 = LOCTEXT("MG42", "MG42");
	auto GM6Lynx = LOCTEXT("GM6 Lynx", "GM6 Lynx");
#undef LOCTEXT_NAMESPACE

	// 需与 EEquipmentType 的值一致
#define LOCTEXT_NAMESPACE "EquipmentType"
	auto Primary = LOCTEXT("Primary", "Primary");
	auto Secondary = LOCTEXT("Secondary", "Secondary");
	auto Melee = LOCTEXT("Melee", "Melee");
	auto Throwing = LOCTEXT("Throwing", "Throwing");
#undef LOCTEXT_NAMESPACE
	
	// 需与 SprayPaint.Name 的值一致
#define LOCTEXT_NAMESPACE "SprayPaint"
	auto Brainburger = LOCTEXT("Brainburger", "Brainburger");
	auto Kun = LOCTEXT("2.5", "2.5");
	auto CorpseSuppressingTalisman = LOCTEXT("CorpseSuppressingTalisman", "Corpse Suppressing Talisman");
#undef LOCTEXT_NAMESPACE

}
