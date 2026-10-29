#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "EquipmentAsset.generated.h"

class UMaterialInterface;
class UTextureRenderTarget2D;
class UMaterialParameterCollection;
class UGameplayAbility;
class UGameplayEffect;
class UAnimMontage;
class USkeletalMesh;
class UTexture2D;
class UCurveFloat;
class UCurveVector;
class UNiagaraSystem;
class UMetaSoundSource;
class AEquipment;
class AShell;

// 通用装备数据资产
UCLASS()
class MUTATEARENA_API UEquipmentCommonAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "EquipSound")
	TObjectPtr<UMetaSoundSource> EquipSound;

	UPROPERTY(EditAnywhere, Category = "DryFireSound")
	TObjectPtr<UMetaSoundSource> DryFireSound;

	UPROPERTY(EditAnywhere, Category = "ImpactSound")
	TObjectPtr<UMetaSoundSource> ImpactSound_Body;
	UPROPERTY(EditAnywhere, Category = "ImpactSound")
	TObjectPtr<UMetaSoundSource> ImpactSound_Concrete;
	UPROPERTY(EditAnywhere, Category = "ImpactSound")
	TObjectPtr<UMetaSoundSource> ImpactSound_Dirt;
	UPROPERTY(EditAnywhere, Category = "ImpactSound")
	TObjectPtr<UMetaSoundSource> ImpactSound_Metal;
	UPROPERTY(EditAnywhere, Category = "ImpactSound")
	TObjectPtr<UMetaSoundSource> ImpactSound_Wood;

	UPROPERTY(EditAnywhere, Category = "HitWall")
	TObjectPtr<UMaterialInterface> WallMarkDecal;
	UPROPERTY(EditAnywhere, Category = "HitWall")
	TObjectPtr<UMetaSoundSource> HitWall_Concrete;
	UPROPERTY(EditAnywhere, Category = "HitWall")
	TObjectPtr<UMetaSoundSource> HitWall_Dirt;
	UPROPERTY(EditAnywhere, Category = "HitWall")
	TObjectPtr<UMetaSoundSource> HitWall_Metal;
	UPROPERTY(EditAnywhere, Category = "HitWall")
	TObjectPtr<UMetaSoundSource> HitWall_Wood;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(DataAsset::EquipmentCommon, GetFName());
	}
};

// 装备数据片段基类
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class MUTATEARENA_API UEquipmentFragment : public UObject
{
	GENERATED_BODY()
};

UCLASS(BlueprintType)
class MUTATEARENA_API UFragWeapon : public UEquipmentFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	int32 MagCapacity = 0;
	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 0;
	UPROPERTY(EditAnywhere)
	float FireRate = 0.f;
	UPROPERTY(EditAnywhere)
	bool bIsAutomatic = true;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UCurveFloat> DamageCurve;
	UPROPERTY(EditAnywhere)
	float Impulse = 0.f;
	UPROPERTY(EditAnywhere)
	int32 PelletNum = 1;
	UPROPERTY(EditAnywhere)
	float PenetrationPower = 0.f;

	UPROPERTY(EditAnywhere)
	float AimingFOVMul = 0.9f;
	UPROPERTY(EditAnywhere)
	float ScopeFOV = 90.f;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UCurveVector> RecoilCurve;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UCurveFloat> RecoilCurveRandVert;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UCurveFloat> RecoilCurveRandHor;

	UPROPERTY(EditAnywhere)
	float RecoilIncTime = 0.f;
	UPROPERTY(EditAnywhere)
	float RecoilMaxDecTime = 0.f;
	UPROPERTY(EditAnywhere)
	float RecoilMinDecTime = 0.f;
	UPROPERTY(EditAnywhere)
	float RecoilVertRef_DecTime = 0.f;
	UPROPERTY(EditAnywhere)
	float RecoilVertRef_Crosshair = 0.f;
	UPROPERTY(EditAnywhere)
	float CrosshairBaseSpread = 0.f;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectileBullet> ProjectileClass;
	UPROPERTY(EditAnywhere)
	TSubclassOf<AShell> ShellClass;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> MuzzleFlashEffect;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UMetaSoundSource> FireSound;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMetaSoundSource> MechSound;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> ReloadMontage_C;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> ReloadMontage_E;
};

UCLASS(BlueprintType)
class MUTATEARENA_API UFragMelee : public UEquipmentFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	float LightAttackDamage = 0.f;
	UPROPERTY(EditAnywhere)
	float HeavyAttackDamage = 0.f;

	UPROPERTY(EditAnywhere)
	TArray<FName> TraceSockets;
	UPROPERTY(EditAnywhere)
	float TraceRadius = 10.f;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> LightAttackMontage_C;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> LightAttackMontage_E;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> HeavyAttackMontage_C;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> HeavyAttackMontage_E;
};

UCLASS(BlueprintType)
class MUTATEARENA_API UFragThrowing : public UEquipmentFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMetaSoundSource> HitSound;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> ThrowMontage_C;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> ThrowMontage_E;
};

UCLASS(BlueprintType, Const)
class MUTATEARENA_API UEquipmentAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UEquipmentAsset> ParentAsset;

	UPROPERTY(EditAnywhere)
	EEquipmentName EquipmentName = EEquipmentName::None;

	UPROPERTY(EditAnywhere)
	FText ShowName;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UTexture2D> ShowImg;

	// 使用 protected 约束这些属性只能通过 Getter 访问父级的（皮肤不能自定义这些属性）
protected:
	UPROPERTY(EditAnywhere, meta = (EditCondition = "ParentAsset == nullptr"))
	EEquipmentType EquipmentType = EEquipmentType::None;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "ParentAsset == nullptr"))
	EEquipmentSlot EquipmentSlot = EEquipmentSlot::None;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "ParentAsset == nullptr"))
	TSoftClassPtr<AEquipment> EquipmentClass;

public:
	UPROPERTY(EditAnywhere)
	TArray<TSoftObjectPtr<UMaterialInterface>> OverrideMaterials;
	
protected:
	UPROPERTY(EditAnywhere, meta = (EditCondition = "ParentAsset == nullptr"))
	TArray<TSoftClassPtr<UGameplayAbility>> GrantedAbilities;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "ParentAsset == nullptr"))
	TObjectPtr<UAnimMontage> SwapInMontage_C;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "ParentAsset == nullptr"))
	TObjectPtr<UAnimMontage> SwapInMontage_E;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "ParentAsset == nullptr"))
	float WalkSpeedMul = 1.f;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "ParentAsset == nullptr"))
	float AimingWalkSpeedMul = 1.f;

public:
	UPROPERTY(EditAnywhere)
	bool bIsSupply = false;

	/**
	 * Must be the same as the OfferId in dev portal,
	 * Used to determine the button type in the UShop::OnQueryOffersComplete.
	 */
	UPROPERTY(EditAnywhere, meta = (EditCondition = "ParentAsset != nullptr"))
	FString OfferId;

	/**
	 * Must be same as the audience item ID in dev portal,
	 * Used to determine whether player's account owns the product.
	 */
	UPROPERTY(EditAnywhere, meta = (EditCondition = "ParentAsset != nullptr"))
	FString AudienceItemId;

	UPROPERTY(EditAnywhere)
	TArray<TObjectPtr<UEquipmentFragment>> Fragments;

protected:
	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UEquipmentFragment>> FragmentCache;

public:
	virtual void PostLoad() override
	{
		Super::PostLoad();

		FragmentCache.Empty();

		// 1. 先继承父武器的 Fragment 缓存
		if (ParentAsset)
		{
			ParentAsset->ConditionalPostLoad();
			FragmentCache = ParentAsset->FragmentCache;
		}

		// 2. 再用自身的 Fragment 覆盖
		for (UEquipmentFragment* Frag : Fragments)
		{
			if (Frag)
			{
				FragmentCache.Add(Frag->GetClass(), Frag);
			}
		}
	}

	template <typename ResultClass>
	const ResultClass* FindFragmentByClass() const
	{
		if (const TObjectPtr<UEquipmentFragment>* Found = FragmentCache.Find(ResultClass::StaticClass()))
		{
			return Cast<ResultClass>(*Found);
		}

		for (const UEquipmentFragment* Fragment : Fragments)
		{
			if (const ResultClass* CastedFragment = Cast<ResultClass>(Fragment))
			{
				return CastedFragment;
			}
		}

		if (ParentAsset)
		{
			return ParentAsset->FindFragmentByClass<ResultClass>();
		}

		return nullptr;
	}

	// 统一数据访问接口（屏蔽基础武器与皮肤的差异）
	
	bool IsSkin() const { return ParentAsset != nullptr; }

	EEquipmentName GetEquipmentParentName() const
	{
		return ParentAsset ? ParentAsset->EquipmentName : EEquipmentName::None;
	}
	
	EEquipmentName GetBaseEquipmentName() const
	{
		return ParentAsset ? ParentAsset->GetBaseEquipmentName() : EquipmentName;
	}
	
	EEquipmentType GetEquipmentType() const
	{
		return ParentAsset ? ParentAsset->GetEquipmentType() : EquipmentType;
	}

	EEquipmentSlot GetEquipmentSlot() const
	{
		return ParentAsset ? ParentAsset->GetEquipmentSlot() : EquipmentSlot;
	}

	TSoftClassPtr<AEquipment> GetEquipmentClass() const
	{
		return ParentAsset ? ParentAsset->GetEquipmentClass() : EquipmentClass;
	}

	const TArray<TSoftClassPtr<UGameplayAbility>>& GetGrantedAbilities() const
	{
		return ParentAsset ? ParentAsset->GetGrantedAbilities() : GrantedAbilities;
	}

	UAnimMontage* GetSwapInMontage_C() const
	{
		return ParentAsset ? ParentAsset->GetSwapInMontage_C() : SwapInMontage_C.Get();
	}

	UAnimMontage* GetSwapInMontage_E() const
	{
		return ParentAsset ? ParentAsset->GetSwapInMontage_E() : SwapInMontage_E.Get();
	}

	float GetWalkSpeedMul() const
	{
		return ParentAsset ? ParentAsset->GetWalkSpeedMul() : WalkSpeedMul;
	}

	float GetAimingWalkSpeedMul() const
	{
		return ParentAsset ? ParentAsset->GetAimingWalkSpeedMul() : AimingWalkSpeedMul;
	}

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(DataAsset::Equipment, GetFName());
	}
};
