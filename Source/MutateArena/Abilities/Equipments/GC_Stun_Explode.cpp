#include "MutateArena/Abilities/Equipments/GC_Stun_Explode.h"

#include "MetaSoundSource.h"
#include "MutateArena/Characters/BaseCharacter.h" 
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

AGC_Stun_Explode::AGC_Stun_Explode()
{
	PrimaryActorTick.bCanEverTick = false;
	
	bAutoDestroyOnRemove = true;
}

bool AGC_Stun_Explode::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (ABaseCharacter* TargetChar = Cast<ABaseCharacter>(MyTarget))
	{
		if (TargetChar->IsLocallyControlled()) 
		{
			// 1. 处理视觉后处理
			if (TargetChar->Camera)
			{
				if (StunPostProcessMaterial && !StunMID)
				{
					StunMID = UMaterialInstanceDynamic::Create(StunPostProcessMaterial, this);
				}
				
				if (StunMID)
				{
					TargetChar->Camera->AddOrUpdateBlendable(StunMID, 1.0f);
					StunMID->SetScalarParameterValue(FName("HitTime"), GetWorld()->GetTimeSeconds());
				}
			}
			
			if (!EarRingingAudioComponent)
			{
				EarRingingAudioComponent = UGameplayStatics::SpawnSound2D(this, EarRingingSound);
			}
		}
	}

	return Super::OnActive_Implementation(MyTarget, Parameters);
}

bool AGC_Stun_Explode::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	// 当玩家已经在眩晕状态中再次被炸时，OnActive 不会再触发，只会触发OnExecute
	if (StunMID)
	{
		StunMID->SetScalarParameterValue(FName("HitTime"), GetWorld()->GetTimeSeconds());
	}
	
	// 再次被闪，从头触发耳鸣声
	if (IsValid(EarRingingAudioComponent))
	{
		if (EarRingingAudioComponent->IsPlaying())
		{
			EarRingingAudioComponent->Play();
		}
	}
	else
	{
		EarRingingAudioComponent = UGameplayStatics::SpawnSound2D(this, EarRingingSound);
	}
	
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}

bool AGC_Stun_Explode::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (ABaseCharacter* TargetChar = Cast<ABaseCharacter>(MyTarget))
	{
		if (TargetChar->IsLocallyControlled())
		{
			if (TargetChar->Camera && StunMID)
			{
				TargetChar->Camera->PostProcessSettings.RemoveBlendable(StunMID);
			}

			if (EarRingingAudioComponent)
			{
				EarRingingAudioComponent = nullptr; 
			}
		}
	}

	return Super::OnRemove_Implementation(MyTarget, Parameters);
}
