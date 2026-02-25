#include "GameplayCue_MutantSmokerSkill.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/Mutants/MutantSmoker.h"
#include "Components/SphereComponent.h"

AGameplayCue_MutantSmokerSkill::AGameplayCue_MutantSmokerSkill()
{
	bAutoDestroyOnRemove = true;
}

bool AGameplayCue_MutantSmokerSkill::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (AMutantSmoker* MutantSmoker = Cast<AMutantSmoker>(MyTarget))
	{
		SmokeEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			SmokerEffect,
			MutantSmoker->GetActorLocation(),
			MutantSmoker->GetActorRotation()
		);

		float Time = 10.f;
		if (SmokeEffectComponent)
		{
			SmokeEffectComponent->SetVariableFloat(TEXT("Time"), Time);
		}

		// 创建一个Actor，阻挡ECC_Visibility
		if (AActor* SmokeActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), GetActorLocation(), GetActorRotation()))
		{
			SmokeActor->SetLifeSpan(Time);
			if (USphereComponent* SphereComponent = NewObject<USphereComponent>(SmokeActor, TEXT("SphereComponent")))
			{
				SphereComponent->RegisterComponent();
				SmokeActor->SetRootComponent(SphereComponent);
				SphereComponent->SetSphereRadius(400.f);
				SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				SphereComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

				// SphereComponent根组件影响力SmokeActor位置，重新设置下
				SmokeActor->SetActorLocation(GetActorLocation());
				SmokeActor->Tags.Add(TAG_SMOKE_ACTOR);
			}
		}
	}

	return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool AGameplayCue_MutantSmokerSkill::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	// 也许会改为围绕在角色身边的烟雾，技能结束时需要结束烟雾。
	// if (MutantSmoker == nullptr) MutantSmoker = Cast<AMutantSmoker>(MyTarget);
	// if (MutantSmoker && SmokeEffectComponent)
	// {
	// 	SmokeEffectComponent->DestroyComponent();
	// }

	return Super::OnRemove_Implementation(MyTarget, Parameters);
}
