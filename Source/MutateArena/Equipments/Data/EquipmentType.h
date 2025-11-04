#pragma once

#include "Engine/DataTable.h"
#include "EquipmentType.generated.h"

#define TRACE_LENGTH 100000.f

UENUM(BlueprintType)
enum class EEquipmentName : uint8 // 装备名字
{
	// 主
	AK47,
	AK47_Desert,
	AK47_Gold,
	AK47_Quenching,
	
	M870,
	
	M60,
	
	PKM,
	
	GM6Lynx,

	// 副
	Glock17,
	
	DesertEagle,
	DesertEagle_Quenching,
	
	// 近战
	Kukri,
	Kukri_Damascus,
	Kukri_Quenching,
	
	MilitaryShovel,
	
	FireAxe,
	
	// 投掷
	Grenade,
	Flashbang,
	Smoke,
	FireBottle,
	
	// 补给箱
	AK47_Cyclone,
	MG42,

	None
};

UENUM(BlueprintType)
enum class EEquipmentCate : uint8 // 装备种类（武器自身属性）
{
	Generic,
	Shotgun,
	Melee,
	Throwing,

	None
};

UENUM(BlueprintType)
enum class EEquipmentType : uint8 // 装备类型（装备的位置）
{
	Primary,
	Secondary,
	Melee,
	Throwing,
	// Tool,
	// Tonic,

	None
};

UENUM(BlueprintType)
enum class EEquipmentState : uint8
{
	Equipped,
	Dropped,
	Thrown
};

USTRUCT(BlueprintType)
struct FEquipmentMain : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FString ShowName;

	UPROPERTY(EditAnywhere)
	EEquipmentName EquipmentParentName = EEquipmentName::None;

	UPROPERTY(EditAnywhere)
	EEquipmentName EquipmentName = EEquipmentName::None;

	UPROPERTY(EditAnywhere)
	EEquipmentCate EquipmentCate = EEquipmentCate::None;

	UPROPERTY(EditAnywhere)
	EEquipmentType EquipmentType = EEquipmentType::None;

	UPROPERTY(EditAnywhere)
	TSoftClassPtr<class AEquipment> EquipmentClass;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<USkeletalMesh> DefaultSkeletalMesh;

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

};

USTRUCT(BlueprintType)
struct FWeaponData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EEquipmentName EquipmentName = EEquipmentName::None;

	UPROPERTY(EditAnywhere)
	int32 MaxCarriedAmmo = 0;
	UPROPERTY(EditAnywhere)
	int32 MagCapacity = 0;

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
	float AimingFOVMul = 0.9;
	UPROPERTY(EditAnywhere)
	float WalkSpeedMul = 1.f;
	UPROPERTY(EditAnywhere)
	float AimingWalkSpeedMul = 1.f;
	UPROPERTY(EditAnywhere)
	bool bIsPIP = false;
	UPROPERTY(EditAnywhere)
	float ScopeFOV = 90.f;

};

USTRUCT(BlueprintType)
struct FMeleeData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EEquipmentName EquipmentName = EEquipmentName::None;

	UPROPERTY(EditAnywhere)
	float LightAttackDamage = 0.f;
	UPROPERTY(EditAnywhere)
	float HeavyAttackDamage = 0.f;

	UPROPERTY(EditAnywhere)
	float WalkSpeedMul = 1.f;

};

USTRUCT(BlueprintType)
struct FEquipmentRecoil : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EEquipmentName EquipmentName = EEquipmentName::None;

	// Y 垂直后坐力 X 水平后坐力 Z 散布
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UCurveVector> RecoilCurve;
	// 随机
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

};
