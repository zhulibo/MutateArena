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
	
	DNA1 = DefaultConfig->DNA1;
	DNA2 = DefaultConfig->DNA2;
}
