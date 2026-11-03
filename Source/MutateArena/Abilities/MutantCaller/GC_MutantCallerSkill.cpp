#include "GC_MutantCallerSkill.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "MutateArena/Characters/Mutants/MutantCaller.h"

AGC_MutantCallerSkill::AGC_MutantCallerSkill()
{
	PrimaryActorTick.bCanEverTick = false;
	bAutoDestroyOnRemove = true;
}

bool AGC_MutantCallerSkill::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	AMutantCaller* MutantCaller = Cast<AMutantCaller>(MyTarget);
	if (MutantCaller == nullptr)
	{
		return Super::OnExecute_Implementation(MyTarget, Parameters);
	}

	FVector FXLocation = Parameters.Location;
	if (FXLocation.IsZero() && MyTarget)
	{
		FXLocation = MyTarget->GetActorLocation();
	}

	if (SummonEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this, 
			SummonEffect, 
			FXLocation, 
			MyTarget ? MyTarget->GetActorRotation() : FRotator::ZeroRotator
		);
	}

	if (SummonSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SummonSound, FXLocation);
	}
    
	if (MutantCaller->IsLocallyControlled())
	{
		if (SkillActivationSound)
		{
			UGameplayStatics::PlaySound2D(MutantCaller, SkillActivationSound);
		}
	}
	
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
