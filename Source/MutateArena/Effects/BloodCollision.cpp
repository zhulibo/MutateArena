#include "BloodCollision.h"
#include "NiagaraSystem.h"
#include "MutateArena/Characters/Data/CharacterAsset.h"
#include "MutateArena/System/AssetSubsystem.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"

void UBloodCollision::ReceiveParticleData_Implementation(const TArray<FBasicParticleData>& Data, UNiagaraSystem* NiagaraSystem, const FVector& SimulationPositionOffset)
{
	for (const FBasicParticleData& ParticleData : Data)
	{
		// TODO 只让碰撞事件触发一次
		// if (PositionIDs.Contains(ParticleData.Position)) continue;
		// PositionIDs.Add(ParticleData.Position);
		int64 TempX = FMath::RoundToInt(ParticleData.Position.X);
		if (PositionXs.Contains(TempX)) continue;
		PositionXs.Add(TempX);

		FVector WorldPos = ParticleData.Position + SimulationPositionOffset;
		FHitResult OutHit;
		FVector Start = WorldPos - FVector(0,0,1);
		FVector End   = WorldPos + FVector(0,0,1);
		FCollisionQueryParams QueryParams; // BUG 不传QueryParams碰撞不生效
		GetWorld()->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, ECollisionChannel::ECC_Visibility, FCollisionShape::MakeSphere(1.f), QueryParams);
		if (OutHit.bBlockingHit)
		{
			if (AssetSubsystem == nullptr) AssetSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UAssetSubsystem>();
			if (AssetSubsystem && AssetSubsystem->CharacterAsset)
			{
				if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(AssetSubsystem->CharacterAsset->MI_BloodDecal, this))
				{
					FLinearColor Color = FLinearColor(ParticleData.Velocity.X, ParticleData.Velocity.Y, ParticleData.Velocity.Z, ParticleData.Size);
					MID->SetVectorParameterValue(TEXT("Color"), Color.ToFColor(true));
					MID->SetScalarParameterValue(FName("TextureIndex"), FMath::RandRange(0, 5));

					auto DecalComponent = UGameplayStatics::SpawnDecalAttached(
						MID,
						FVector(5.f, 20.f, 20.f),
						OutHit.GetComponent(),
						OutHit.BoneName,
						OutHit.ImpactPoint,
						OutHit.ImpactNormal.Rotation(),
						EAttachLocation::KeepWorldPosition,
						10.f
					);

					// if (OutHit.BoneName.ToString() != FString(TEXT("NONE")))
					// {
					// 	UE_LOG(LogTemp, Warning, TEXT("OutHit.BoneName %s"), *OutHit.BoneName.ToString());
					// }
					
					DecalComponent->SetFadeOut(8.f, 2.f, false);
					DecalComponent->SetFadeScreenSize(0.004f);
				}
			}
		}
	}
}
