#pragma once

#include "CoreMinimal.h"
#include "CharacterType.h"
#include "Engine/DataAsset.h"
#include "MutateArena/MutateArena.h"
#include "DNAAsset2.generated.h"

enum class EDNA : uint8;

UCLASS(BlueprintType)
class MUTATEARENA_API UDNAAsset2 : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	EDNA DNA = EDNA::None;

	UPROPERTY(EditAnywhere)
	FText DisplayName;
	
	UPROPERTY(EditAnywhere)
	FText Desc;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UTexture2D> Icon;
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(ASSET_DNA, GetFName());
	}
	
};
