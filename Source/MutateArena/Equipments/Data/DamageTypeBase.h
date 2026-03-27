#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "DamageTypeBase.generated.h"

UENUM(BlueprintType)
enum class EDamageCauserType : uint8
{
	Equipment,
	Melee,
	// 感染并不应用伤害，只是为了给击杀日志传参
	MutantInfect,
	// 突变体造成的伤害
	MutantDamage,
	// 流血
	Bleed,
	// 跌落
	Fall,
};

UCLASS()
class MUTATEARENA_API UDamageTypeBase : public UDamageType
{
	GENERATED_BODY()

public:
	// Used to determine the type of damage, Only need cast to UDamageTypeBase to get the type
	// avoid cast to UDamageTypeEquipment, UDamageTypeMutant, etc.
	UPROPERTY(EditAnywhere)
	EDamageCauserType DamageType;

};
