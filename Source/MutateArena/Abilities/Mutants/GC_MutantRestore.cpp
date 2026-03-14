#include "GC_MutantRestore.h"

#include "MutateArena/Characters/MutantCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "MetaSoundSource.h"

AGC_MutantRestore::AGC_MutantRestore()
{
	PrimaryActorTick.bCanEverTick = false;
	
	bAutoDestroyOnRemove = true;
}

bool AGC_MutantRestore::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	AMutantCharacter* MutantCharacter = Cast<AMutantCharacter>(MyTarget);
	if (MutantCharacter && MutantCharacter->IsLocallyControlled())
	{
		UGameplayStatics::SpawnSound2D(this, RestoreHealthSound);
	}
	
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}

bool AGC_MutantRestore::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	return Super::OnRemove_Implementation(MyTarget, Parameters);
}
