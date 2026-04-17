#include "GC_MutantCorpseDriverSkill.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "MutateArena/Characters/Mutants/MutantCorpseDriver.h"

AGC_MutantCorpseDriverSkill::AGC_MutantCorpseDriverSkill()
{
	PrimaryActorTick.bCanEverTick = false;
	bAutoDestroyOnRemove = true;
}

bool AGC_MutantCorpseDriverSkill::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	AMutantCorpseDriver* MutantCorpseDriver = Cast<AMutantCorpseDriver>(MyTarget);
	if (MutantCorpseDriver == nullptr)
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
    
	if (MutantCorpseDriver->IsLocallyControlled())
	{
		if (SkillActivationSound)
		{
			UGameplayStatics::PlaySound2D(MutantCorpseDriver, SkillActivationSound);
		}
	}
	
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
