#pragma once

#include "CoreMinimal.h"
#include "CharacterType.h"
#include "Engine/DataAsset.h"
#include "MutateArena/MutateArena.h"
#include "HumanDNAAsset.generated.h"

enum class EHumanDNA : uint8;

UCLASS(BlueprintType)
class MUTATEARENA_API UHumanDNAAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	EHumanDNA DNAType = EHumanDNA::None;

	UPROPERTY(EditAnywhere)
	FText DisplayName;
	
	UPROPERTY(EditAnywhere)
	FText Desc;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UTexture2D> Icon;
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(ASSET_HUMAN_DNA, GetFName());
	}
	
};
