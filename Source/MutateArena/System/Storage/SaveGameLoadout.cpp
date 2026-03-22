#include "SaveGameLoadout.h"

#include "DefaultConfig.h"

USaveGameLoadout::USaveGameLoadout()
{
	const UDefaultConfig* DefaultConfig = GetDefault<UDefaultConfig>();

	if (DefaultConfig == nullptr) return;

	Loadouts = DefaultConfig->Loadouts;
	LoadoutIndex = DefaultConfig->LoadoutIndex;

	HumanCharacterName = DefaultConfig->HumanCharacterName;
	MutantCharacterName = DefaultConfig->MutantCharacterName;
	
	HumanDNA1 = DefaultConfig->HumanDNA1;
	HumanDNA2 = DefaultConfig->HumanDNA2;
}
