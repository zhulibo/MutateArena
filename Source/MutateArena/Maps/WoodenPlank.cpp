#include "WoodenPlank.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/DamageEvents.h"
#include "Field/FieldSystemObjects.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

AWoodenPlank::AWoodenPlank()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
	HitBox->SetupAttachment(DefaultSceneRoot);
	HitBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	HitBox->bReturnMaterialOnMove = true;
	
	GeometryComp = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("GeometryComp"));
	GeometryComp->SetupAttachment(DefaultSceneRoot);
    
	// 初始状态下关闭物理模拟
	GeometryComp->SetSimulatePhysics(false);
	GeometryComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWoodenPlank::BeginPlay()
{
	Super::BeginPlay();

	// 仅在服务器端初始化血量
	if (HasAuthority())
	{
		CurrentHealth = MaxHealth;
	}
}

// 注册网络同步变量
void AWoodenPlank::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AWoodenPlank, CurrentHealth);
}

float AWoodenPlank::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 1. 先调用父类逻辑，触发引擎默认的伤害事件（Blueprint Delegate），并获取实际伤害值
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 2. 使用 ActualDamage 进行判断
	if (!HasAuthority() || bIsBroken || ActualDamage <= 0.f)
	{
		return 0.f;
	}

	// 3. 扣除实际伤害
	CurrentHealth -= ActualDamage;

	if (CurrentHealth <= 0.f)
	{
		bIsBroken = true;

		FVector HitLoc = GetActorLocation();
		FVector HitDir = FVector::DownVector;

		// 判断是点伤害还是径向伤害
		if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
		{
			const FPointDamageEvent* PointDamageEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
			HitLoc = PointDamageEvent->HitInfo.ImpactPoint;
			HitDir = PointDamageEvent->ShotDirection;
		}
		else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
		{
			const FRadialDamageEvent* RadialDamageEvent = static_cast<const FRadialDamageEvent*>(&DamageEvent);
			HitLoc = RadialDamageEvent->Origin; 
			HitDir = (GetActorLocation() - HitLoc).GetSafeNormal(); 
		}

		Multicast_TriggerBreak(HitLoc, HitDir);
		SetLifeSpan(10.f); // 注意和 Chaos 里的移除时长匹配
	}

	return ActualDamage; // 返回实际造成的伤害
}

void AWoodenPlank::Multicast_TriggerBreak_Implementation(FVector HitLocation, FVector HitDirection)
{
	if (!GeometryComp) return;

	// 1. 关闭外部的 HitBox 碰撞
	HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 2. 激活 GC 组件的物理和碰撞
	GeometryComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GeometryComp->SetSimulatePhysics(true);
	
	// 防角色跨上抽搐 TODO 无效
	// GeometryComp->CanCharacterStepUpOn = ECB_No;
	// GeometryComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	// GeometryComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); 
	// GeometryComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	
	// 防止掉落的碎块影响 AI 的寻路网格生成
	// GeometryComp->SetCanEverAffectNavigation(false);
	
	// 创建一个球形衰减场 (Radial Falloff)
	if (URadialFalloff* RadialStrainField = NewObject<URadialFalloff>())
	{
		float StrainMagnitude = 500000.f; // 破坏力大小（必须大于资产中的 Damage Threshold）
		float FieldRadius = 100.f;        // 破坏场的作用半径不知道为什么设置太小破坏不了大木板
		
		RadialStrainField->SetRadialFalloff(
			StrainMagnitude, 
			0.f, 
			1.f, 
			0.f, 
			FieldRadius, 
			HitLocation, 
			EFieldFalloffType::Field_FallOff_None // 不衰减，范围内全额破坏
		);

		// 将破坏场应用到几何体集合组件上
		// Target 为 ExternalClusterStrain 表示这个场专门用来打破几何体碎块之间的连接
		GeometryComp->ApplyPhysicsField(true, EGeometryCollectionPhysicsTypeEnum::Chaos_ExternalClusterStrain, nullptr, RadialStrainField);
	}
	
	// 一键强制粉碎：打破当前所有激活的碎块簇连接，将其还原为最小的物理碎片
	// GeometryComp->CrumbleActiveClusters();

	// 3. 让刚刚被“场”打碎的碎块瞬间炸飞开来
	float ImpulseRadius = 50.f;
	float ImpulseStrength = 200.f;
	GeometryComp->AddRadialImpulse(HitLocation, ImpulseRadius, ImpulseStrength, ERadialImpulseFalloff::RIF_Constant, true);
}
