#include "GC_MutantRestore.h"
#include "MutateArena/Characters/MutantCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "MetaSoundSource.h"

UGC_MutantRestore::UGC_MutantRestore()
{
}

bool UGC_MutantRestore::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (AMutantCharacter* MutantCharacter = Cast<AMutantCharacter>(MyTarget))
	{
		if (MutantCharacter->IsLocallyControlled())
		{
			if (RestoreHealthSound)
			{
				UGameplayStatics::SpawnSound2D(MutantCharacter, RestoreHealthSound);
			}
		}
	}
	
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
