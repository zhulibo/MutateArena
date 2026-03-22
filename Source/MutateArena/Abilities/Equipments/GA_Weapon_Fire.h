#pragma once

#include "CoreMinimal.h"
#include "GA_Equipment.h"
#include "GA_Weapon_Fire.generated.h"

USTRUCT(BlueprintType)
struct FGameplayAbilityTargetData_FireInfo : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	UPROPERTY()
	FVector HitTarget = FVector::ZeroVector;

	UPROPERTY()
	float RecoilVert= 0.f;

	UPROPERTY()
	float RecoilHor= 0.f;

	UPROPERTY()
	float SpreadX= 0.f;

	UPROPERTY()
	float SpreadY= 0.f;

	// 返回静态结构体信息
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FGameplayAbilityTargetData_FireInfo::StaticStruct();
	}

	// 网络序列化（决定了数据怎么在网线中打包传输）
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << HitTarget;
		Ar << RecoilVert;
		Ar << RecoilHor;
		Ar << SpreadX;
		Ar << SpreadY;
		bOutSuccess = true;
		return true;
	}
};

// 向 UE 底层注册这个结构体支持网络序列化
template<>
struct TStructOpsTypeTraits<FGameplayAbilityTargetData_FireInfo> : public TStructOpsTypeTraitsBase2<FGameplayAbilityTargetData_FireInfo>
{
	enum { WithNetSerializer = true };
};

UCLASS(Abstract)
class MUTATEARENA_API UGA_Weapon_Fire : public UGA_Equipment
{
	GENERATED_BODY()

public:
	UGA_Weapon_Fire();

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION()
	void OnFireEventReceived();
	// 用来接收解包数据的函数
	UFUNCTION()
	void OnTargetDataReceived(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ActivationTag);
	
	UFUNCTION()
	void StartFireLoop();

	UFUNCTION()
	void OnFireIntervalFinished();
	
};
