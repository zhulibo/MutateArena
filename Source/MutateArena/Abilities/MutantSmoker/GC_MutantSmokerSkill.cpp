#include "GC_MutantSmokerSkill.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "MetaSoundSource.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/Mutants/MutantSmoker.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MutateArena/System/Tags/ProjectTags.h"

AGC_MutantSmokerSkill::AGC_MutantSmokerSkill()
{
	PrimaryActorTick.bCanEverTick = false;
   
	bAutoDestroyOnRemove = true;
}

bool AGC_MutantSmokerSkill::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
    if (AMutantSmoker* MutantSmoker = Cast<AMutantSmoker>(MyTarget))
    {
       if (!SmokeEffectComponent && !SpawnedSmokeActor)
       {
          float TotalDuration = 10.f; 
          float TimeElapsed = 0.f;

          if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MyTarget))
          {
             FGameplayEffectQuery Query;
             Query.OwningTagQuery = FGameplayTagQuery::MakeQuery_MatchTag(TAG_STATE_MUTANT_SMOKER_SMOKING);

             TArray<float> Durations = ASC->GetActiveEffectsDuration(Query);
             TArray<float> TimeRemainings = ASC->GetActiveEffectsTimeRemaining(Query);

             if (Durations.Num() > 0 && TimeRemainings.Num() > 0)
             {
                TotalDuration = Durations[0];
                TimeElapsed = TotalDuration - TimeRemainings[0];
             }
          }
          
          SmokeEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
             GetWorld(),
             SmokerEffect,
             MutantSmoker->GetActorLocation(),
             MutantSmoker->GetActorRotation()
          );
          if (SmokeEffectComponent)
          {
             SmokeEffectComponent->SetVariableFloat(TEXT("Time"), TotalDuration);
             if (TimeElapsed > 0.1f)
             {
                SmokeEffectComponent->AdvanceSimulation(1, TimeElapsed);
             }
          }

          SpawnedSmokeActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), MutantSmoker->GetActorLocation(), MutantSmoker->GetActorRotation());
          if (SpawnedSmokeActor)
          {
             float RemainingTime = TotalDuration - TimeElapsed;
             SpawnedSmokeActor->SetLifeSpan(RemainingTime > 0.f ? RemainingTime : 0.1f);

             if (USphereComponent* SphereComponent = NewObject<USphereComponent>(SpawnedSmokeActor, TEXT("SphereComponent")))
             {
                SphereComponent->RegisterComponent();
                SpawnedSmokeActor->SetRootComponent(SphereComponent);
                SphereComponent->SetSphereRadius(400.f);
                SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                SphereComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
             }
             SpawnedSmokeActor->Tags.Add(TAG_SMOKE_ACTOR);
          }
       }
		
       if (MutantSmoker->IsLocallyControlled())
       {
          if (SkillActivationSound)
          {
             UGameplayStatics::PlaySound2D(MutantSmoker, SkillActivationSound);
          }
       }
    }

    return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool AGC_MutantSmokerSkill::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (SmokeEffectComponent)
	{
		SmokeEffectComponent->DestroyComponent();
	   SmokeEffectComponent = nullptr;
	}
   
   if (SpawnedSmokeActor)
   {
      SpawnedSmokeActor->Destroy();
      SpawnedSmokeActor = nullptr;
   }
   
	return Super::OnRemove_Implementation(MyTarget, Parameters);
}
