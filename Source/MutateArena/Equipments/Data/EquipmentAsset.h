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
    float WalkSpeedMul = 1.f;
    UPROPERTY(EditAnywhere)
    float AimingWalkSpeedMul = 1.f;
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
    float WalkSpeedMul = 1.f;
    
    UPROPERTY(EditAnywhere)
    TArray<FName> TraceSockets;
    UPROPERTY(EditAnywhere)
    float TraceRadius = 10.f;

    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UAnimMontage> LightAttackMontage_C;
    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UAnimMontage> LightAttackMontage_E;

    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UAnimMontage> HeavyAttackMontage_C;
    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UAnimMontage> HeavyAttackMontage_E;
};

UCLASS(BlueprintType)
class MUTATEARENA_API UFragThrowing : public UEquipmentFragment
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UMetaSoundSource> HitSound;

    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UAnimMontage> ThrowMontage_C;
    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UAnimMontage> ThrowMontage_E;
};

UCLASS(BlueprintType, Const)
class MUTATEARENA_API UEquipmentAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    FText ShowName;
    
    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UTexture2D> ShowImg;

    UPROPERTY(EditAnywhere)
    EEquipmentName EquipmentName = EEquipmentName::None;

    UPROPERTY(EditAnywhere)
    EEquipmentCate EquipmentCate = EEquipmentCate::None;

    UPROPERTY(EditAnywhere)
    EEquipmentType EquipmentType = EEquipmentType::None;

    UPROPERTY(EditAnywhere)
    TArray<TObjectPtr<UEquipmentFragment>> Fragments;

    UPROPERTY(EditAnywhere)
    TSoftClassPtr<AEquipment> EquipmentClass;

    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<USkeletalMesh> DefaultSkeletalMesh;
    
    UPROPERTY(EditAnywhere)
    TArray<TSoftClassPtr<UGameplayAbility>> GrantedAbilities;

    UPROPERTY(EditAnywhere)
    TArray<TSoftClassPtr<UGameplayEffect>> PassiveEffects;

    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UAnimMontage> SwapInMontage_C;
    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UAnimMontage> SwapInMontage_E;
    
private:
    UPROPERTY(Transient)
    TMap<TObjectPtr<UClass>, TObjectPtr<UEquipmentFragment>> FragmentCache;

public:
    virtual void PostLoad() override
    {
        Super::PostLoad();
        
        FragmentCache.Empty();
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
        return nullptr;
    }

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(DataAsset::Equipment, GetFName());
    }
};

UCLASS(BlueprintType)
class MUTATEARENA_API UFragWeaponModifier : public UEquipmentFragment
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
    bool bOverrideMagCapacity = false;
    UPROPERTY(EditAnywhere, meta = (EditCondition = "bOverrideMagCapacity"))
    int32 MagCapacity = 0;

    UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
    bool bOverrideMaxCarriedAmmo = false;
    UPROPERTY(EditAnywhere, meta = (EditCondition = "bOverrideMaxCarriedAmmo"))
    int32 MaxCarriedAmmo = 0;
};

UCLASS(BlueprintType, Const)
class MUTATEARENA_API UEquipmentSkinAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    EEquipmentName EquipmentParentName = EEquipmentName::None;

    UPROPERTY(EditAnywhere)
    EEquipmentName EquipmentName = EEquipmentName::None;

    UPROPERTY(EditAnywhere)
    bool bIsSupply = false;

    UPROPERTY(EditAnywhere)
    TObjectPtr<UFragWeaponModifier> WeaponModifier;

    UPROPERTY(EditAnywhere)
    FText SkinName;

    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UTexture2D> SkinIcon;

    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<USkeletalMesh> OverrideSkeletalMesh;

    UPROPERTY(EditAnywhere)
    TArray<TSoftObjectPtr<UMaterialInterface>> OverrideMaterials;

    UPROPERTY(EditAnywhere)
    TSoftClassPtr<class AEquipment> OverrideEquipmentClass;

    /**
     * Must be the same as the OfferId in dev portal,
     * Used to determine the button type in the UShop::OnQueryOffersComplete.
     */
    UPROPERTY(EditAnywhere)
    FString OfferId;

    /**
     * Must be same as the audience item ID in dev portal,
     * Used to determine whether player's account owns the product.
     */
    UPROPERTY(EditAnywhere)
    FString AudienceItemId;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(DataAsset::EquipmentSkin, GetFName());
    }
};
