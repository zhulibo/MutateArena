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
	// 主
	auto AK47 = LOCTEXT("AK47", "AK47");
	auto AK47_Desert = LOCTEXT("AK47 Desert", "AK47 Desert");
	auto AK47_Gold = LOCTEXT("AK47 Gold", "AK47 Gold");
	auto AK47_Quenching = LOCTEXT("AK47 Quenching", "AK47 Quenching");
	
	auto M870 = LOCTEXT("M870", "M870");
	
	auto M60 = LOCTEXT("M60", "M60");
	
	auto PKM = LOCTEXT("PKM", "PKM");
	
	auto GM6Lynx = LOCTEXT("GM6 Lynx", "GM6 Lynx");

	// 副
	auto Glock17 = LOCTEXT("Glock17", "Glock17");
	
	auto DesertEagle = LOCTEXT("Desert Eagle", "Desert Eagle");
	auto DesertEagle_Quenching = LOCTEXT("Desert Eagle Quenching", "Desert Eagle Quenching");

	// 近战
	auto Kukri = LOCTEXT("Kukri", "Kukri");
	auto Kukri_Damascus = LOCTEXT("Kukri Damascus", "Kukri Damascus");
	auto Kukri_Quenching = LOCTEXT("Kukri Quenching", "Kukri Quenching");
	
	auto MilitaryShovel = LOCTEXT("Military Shovel", "Military Shovel");
	
	auto FireAxe = LOCTEXT("Fire Axe", "Fire Axe");

	// 投掷
	auto Grenade = LOCTEXT("Grenade", "Grenade");
	
	auto Flashbang = LOCTEXT("Flashbang", "Flashbang");
	
	auto Smoke = LOCTEXT("Smoke", "Smoke");
	
	auto FireBottle = LOCTEXT("Fire Bottle", "Fire Bottle");

	// 补给箱
	auto AK47Cyclone = LOCTEXT("AK47 Cyclone", "AK47 Cyclone");
	
	auto MG42 = LOCTEXT("MG42", "MG42");
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
	auto SealingTalisman = LOCTEXT("SealingTalisman", "Sealing Talisman");
	auto Kun = LOCTEXT("2.5", "2.5");
	auto Brainburger = LOCTEXT("Brainburger", "Brainburger");
	auto Biohazard = LOCTEXT("Biohazard", "Biohazard");
#undef LOCTEXT_NAMESPACE

}
