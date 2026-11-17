#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Equipments/Data/EquipmentType.h"
#include "EquipmentAsset.generated.h"

class UAnimInstHuman_Layer;
class UAnimInstHuman1P_Layer;
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
class UCameraShakeBase;

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

// 武器开火时单个机械构件的程序化往复运动配置
USTRUCT(BlueprintType)
struct FWeaponFirePartMotion
{
	GENERATED_BODY()

	// 对应机械骨骼通道
	UPROPERTY(EditAnywhere)
	EWeaponMechBone TargetBone = EWeaponMechBone::Bolt;

	// 最大局部位置偏移
	UPROPERTY(EditAnywhere)
	FVector MaxLocationOffset = FVector(0.f, 0.f, 0.f);

	// 最大局部旋转偏移
	UPROPERTY(EditAnywhere)
	FRotator MaxRotationOffset = FRotator::ZeroRotator;

	// 单次往复总耗时（秒）
	UPROPERTY(EditAnywhere)
	float CycleDuration = 0.0f;

	// 后退到位时间占比
	UPROPERTY(EditAnywhere)
	float KickTimeRatio = 0.0f;

	// 后死点驻留时间占比
	UPROPERTY(EditAnywhere)
	float HoldTimeRatio = 0.0f;
};

UCLASS(BlueprintType)
class MUTATEARENA_API UFragWeapon : public UEquipmentFragment
{
	GENERATED_BODY()

public:
	// 开火时机械构件程序化往复运动配置（按 TargetBone 映射至对应骨骼通道）
	UPROPERTY(EditAnywhere)
	TArray<FWeaponFirePartMotion> FirePartMotions;

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
	// 瞄准过渡耗时（同时驱动镜头 FOV 缩放与角色程序化瞄准姿态插值）
	UPROPERTY(EditAnywhere)
	float ADSTime = 0.3f;

	// 1P 瞄准时，Sight 插槽距离摄像机镜头的理想前向距离 (单位: cm | X轴正前方距离)
	UPROPERTY(EditAnywhere)
	float ADSEyeReliefDistance = 10.f;
	// 3P 瞄准时角色向右歪头贴腮的总旋转偏移 (单位: 度 | 标准坐标系: X=Roll向右歪头, Y=Pitch低头, Z=Yaw偏航)
	UPROPERTY(EditAnywhere)
	FRotator ADSHeadTiltRotation3P = FRotator::MakeFromEuler(FVector(14.f, -2.f, 4.f));
	// 单发开火位置抖动 (单位: cm | X=负值向后撞肩, Y=左右, Z=上下)
	UPROPERTY(EditAnywhere)
	FVector RecoilKickLocation = FVector(0.f, 0.f, 0.f);
	// 单发开火旋转抖动 (单位: 度 | X=枪身拧转, Y=枪口上跳, Z=枪口左右摆)
	UPROPERTY(EditAnywhere)
	FRotator RecoilKickRotation = FRotator::MakeFromEuler(FVector(0.f, 0.f, 0.f));
	// 连发最大位置偏移上限 (单位: cm | 防止连续扫射时枪模移出屏幕)
	UPROPERTY(EditAnywhere)
	FVector RecoilMaxLocation = FVector(999.f, 999.f, 999.f);
	// 连发最大旋转偏移上限 (单位: 度 | 防止连续扫射时枪口无限上扬)
	UPROPERTY(EditAnywhere)
	FRotator RecoilMaxRotation = FRotator::MakeFromEuler(FVector(90.f, 90.f, 90.f));
	// 开火爆发速度倍率 (越大枪身手后坐力后仰越快)
	UPROPERTY(EditAnywhere)
	float RecoilKickSpeedMul = 1.f;
	// 后坐力回正速度倍率 (越大枪身归位越快)
	UPROPERTY(EditAnywhere)
	float RecoilRecoverySpeedMul = 0.6f;
	// 后坐力回正阻尼倍率 (越大回正过冲越小)
	UPROPERTY(EditAnywhere)
	float RecoilDampingMul = 1.f;
	// 连发随机晃动倍率 (越大扫射时枪身左右游走和拧动越剧烈；0=完全按固定方向跳动)
	UPROPERTY(EditAnywhere)
	float RecoilNoiseScale = 1.f;
	// 机瞄后坐力缩放比例 (0.5 = 开镜时枪模抖动幅度减半)
	UPROPERTY(EditAnywhere)
	float ADSRecoilScale = 0.5f;

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
	TObjectPtr<UAnimMontage> ReloadMontage1P_C;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> ReloadMontage1P_E;
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
	TObjectPtr<UAnimMontage> LightAttackMontage1P_C;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> LightAttackMontage1P_E;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> LightAttackMontage_C;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> LightAttackMontage_E;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> HeavyAttackMontage1P_C;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> HeavyAttackMontage1P_E;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> HeavyAttackMontage_C;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> HeavyAttackMontage_E;
};

UCLASS(BlueprintType)
class MUTATEARENA_API UFragThrowable : public UEquipmentFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMetaSoundSource> HitSound;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UNiagaraSystem> ExplodeEffect;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMetaSoundSource> ExplodeSound;

	// 引信时间（秒）；碰撞引爆类投掷物（如燃烧瓶）无需使用
	UPROPERTY(EditAnywhere)
	float FuseTime = 3.f;
	UPROPERTY(EditAnywhere)
	float Bounciness = 0.3f;
	UPROPERTY(EditAnywhere)
	float Friction = 0.6f;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> ThrowMontage1P_C;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> ThrowMontage1P_E;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> ThrowMontage_C;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> ThrowMontage_E;
};

UCLASS(BlueprintType)
class MUTATEARENA_API UFragGrenade : public UEquipmentFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	float Damage = 500.f;
	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 100.f;
	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 400.f;
};

UCLASS(BlueprintType)
class MUTATEARENA_API UFragFlashbang : public UEquipmentFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	float Radius = 1200.f;
	// 闪光留存时间
	UPROPERTY(EditAnywhere)
	float MaxFlashTime = 6.f;
	// 残影留存时间
	UPROPERTY(EditAnywhere)
	float MaxCapTime = 10.f;
	// 起音较慢，在引爆前提前多少秒播放音效
	UPROPERTY(EditAnywhere)
	float SoundLeadTime = 0.5f;
};

UCLASS(BlueprintType)
class MUTATEARENA_API UFragSmoke : public UEquipmentFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	float Duration = 12.f;
};

UCLASS(BlueprintType)
class MUTATEARENA_API UFragStun : public UEquipmentFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	float Radius = 800.f;
	UPROPERTY(EditAnywhere)
	TSubclassOf<UCameraShakeBase> CameraShakeClass;
	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> StunEffectClass;
};

UCLASS(BlueprintType)
class MUTATEARENA_API UFragFireBottle : public UEquipmentFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	float FireRadius = 200.f;
	UPROPERTY(EditAnywhere)
	float Duration = 10.f;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMetaSoundSource> BurnSound;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMetaSoundSource> ExtinguishSound;
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

	// 使用 protected 约束这些属性只能通过 Getter 访问父级的（子级皮肤不能自定义这些属性）
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
	TSubclassOf<UAnimInstHuman1P_Layer> AnimInstHuman1P_Layer;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "ParentAsset == nullptr"))
	TSubclassOf<UAnimInstHuman_Layer> AnimInstHuman_Layer;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "ParentAsset == nullptr"))
	TObjectPtr<UAnimMontage> EquipMontage1P_C;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "ParentAsset == nullptr"))
	TObjectPtr<UAnimMontage> EquipMontage1P_E;

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

	EEquipmentName GetParentEquipmentName() const
	{
		return ParentAsset ? ParentAsset->GetParentEquipmentName() : EquipmentName;
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
	
	TSubclassOf<UAnimInstHuman1P_Layer> GetAnimInstHuman1P_Layer() const
	{
		return ParentAsset ? ParentAsset->GetAnimInstHuman1P_Layer() : AnimInstHuman1P_Layer;
	}
	
	TSubclassOf<UAnimInstHuman_Layer> GetAnimInstHuman_Layer() const
	{
		return ParentAsset ? ParentAsset->GetAnimInstHuman_Layer() : AnimInstHuman_Layer;
	}
	
	UAnimMontage* GetEquipMontage1P_C() const
	{
		return ParentAsset ? ParentAsset->GetEquipMontage1P_C() : EquipMontage1P_C.Get();
	}

	UAnimMontage* GetEquipMontage1P_E() const
	{
		return ParentAsset ? ParentAsset->GetEquipMontage1P_E() : EquipMontage1P_E.Get();
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
