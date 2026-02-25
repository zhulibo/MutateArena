#include "GameplayCue_MutantRestore.h"

#include "MutateArena/Characters/MutantCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "MetaSoundSource.h"

AGameplayCue_MutantRestore::AGameplayCue_MutantRestore()
{
	bAutoDestroyOnRemove = true;
}

bool AGameplayCue_MutantRestore::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	UE_LOG(LogTemp, Warning, TEXT("OnExecute_Implementation"));
	AMutantCharacter* MutantCharacter = Cast<AMutantCharacter>(MyTarget);
	if (MutantCharacter && MutantCharacter->IsLocallyControlled())
	{
		UGameplayStatics::SpawnSound2D(this, RestoreHealthSound);
	}
	
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}

bool AGameplayCue_MutantRestore::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	return Super::OnRemove_Implementation(MyTarget, Parameters);
}
