#include "RecoilComponent.h"

#include "CombatComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "TimerManager.h"
#include "Curves/CurveVector.h"
#include "MutateArena/Equipments/Weapon.h"

URecoilComponent::URecoilComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URecoilComponent::BeginPlay()
{
	Super::BeginPlay();
	
	HumanCharacter = Cast<AHumanCharacter>(GetOwner());
}

void URecoilComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

	if (HumanCharacter && HumanCharacter->IsLocallyControlled())
	{
		// double Time1 = FPlatformTime::Seconds();

		PollApplyRecoil(DeltaSeconds);

		PollRecoverRecoil(DeltaSeconds);

		// double Time2 = FPlatformTime::Seconds();
		// UE_LOG(LogTemp, Warning, TEXT("Recoil cost time %f"), Time2 - Time1);
	}
}

// 开火后增加后坐力
void URecoilComponent::IncRecoil()
{
	if (HumanCharacter == nullptr || HumanCharacter->CombatComponent == nullptr) return;

	AWeapon* Weapon = HumanCharacter->CombatComponent->GetCurWeapon();
	if (Weapon == nullptr || Weapon->RecoilCurve == nullptr) return;

	// 从总后坐力中减去上次开火未及时应用的后坐力（开火后后坐力并非瞬时增加，而是分多帧应用）
	RecoilVertTotal -= RecoilVertCurFire - RecoilVertLastTick;
	RecoilHorTotal -= RecoilHorCurFire - RecoilHorLastTick;

	int32 CurShotCount = HumanCharacter->CombatComponent->CurShotCount;
	int32 RandRecoilCurveCount = CurShotCount % 10 + 1; // 曲线横轴只做了1-10

	FVector RecoilKick = Weapon->RecoilCurve->GetVectorValue(CurShotCount);
	float RecoilRandVert = 0.f;
	float RecoilRandHor = 0.f;
	if (Weapon->RecoilCurveRandVert) RecoilRandVert = Weapon->RecoilCurveRandVert->GetFloatValue(RandRecoilCurveCount);
	if (Weapon->RecoilCurveRandHor) RecoilRandHor = Weapon->RecoilCurveRandHor->GetFloatValue(RandRecoilCurveCount);
	// 开火需要增加的目标垂直后坐力
	RecoilVertCurFire = RecoilKick.Y + RecoilRandVert;
	// 开火需要增加的目标水平后坐力
	RecoilHorCurFire = RecoilKick.X + RecoilRandHor;

	// 累计总垂直后坐力
	RecoilVertTotal += RecoilVertCurFire;
	// 累计总水平后坐力
	RecoilHorTotal += RecoilHorCurFire;

	// 重置应用开火后坐力耗时
	RecoilIncCostTime = 0.f;
	// 重置应用开火上一帧后坐力
	RecoilVertLastTick = 0.f;
	RecoilHorLastTick = 0.f;

	// 重置恢复后坐力需要的时间
	RecoilDecTime = 0.f;
	// 重置恢复后坐力已经消耗的时间
	RecoilDecCostTime = 0.f;
	// 重置恢复后坐力上一帧后坐力
	RecoilVertRecoverLastTick = 0.f;
	RecoilHorRecoverLastTick = 0.f;
}

// 在RecoilIncTime时间内，分多帧应用开火后产生的后坐力
void URecoilComponent::PollApplyRecoil(float DeltaSeconds)
{
	if (RecoilVertTotal == 0.f && RecoilHorTotal == 0.f) return;

	if (HumanCharacter == nullptr || HumanCharacter->CombatComponent == nullptr) return;
	AWeapon* Weapon = HumanCharacter->CombatComponent->GetCurWeapon();
	if (Weapon == nullptr) return;

	if (RecoilIncCostTime >= Weapon->RecoilIncTime) return;

	// UE_LOG(LogTemp, Warning, TEXT("RecoilIncCostTime %f DeltaSeconds %f"), RecoilIncCostTime, DeltaSeconds);

	// 累计应用后坐力耗时
	RecoilIncCostTime += DeltaSeconds;

	// 应用开火后坐力完毕
	if (RecoilIncCostTime >= Weapon->RecoilIncTime)
	{
		// 约束耗时
		RecoilIncCostTime = Weapon->RecoilIncTime;

		// 计算恢复后坐力所需时间
		RecoilDecTime = FMath::Clamp(RecoilVertTotal / Weapon->RecoilVertRef_DecTime, Weapon->RecoilMinDecTime, Weapon->RecoilMaxDecTime);

		// 备份需要恢复的总后坐力
		RecoilVertRecoverTotal = RecoilVertTotal;
		RecoilHorRecoverTotal = RecoilHorTotal;

		// UE_LOG(LogTemp, Warning, TEXT("RecoilDecTime %f RecoilVertTotal %f RecoilHorTotal %f"), RecoilDecTime, RecoilVertTotal, RecoilHorTotal);
	}

	// 当前帧需要达到的后坐力
	float Alpha = RecoilIncCostTime / Weapon->RecoilIncTime;
	float RecoilVertCurTick = FMath::InterpEaseOut(0.f, RecoilVertCurFire, Alpha, 2.f);
	float RecoilHorCurTick = FMath::InterpEaseOut(0.f, RecoilHorCurFire, Alpha, 2.f);

	// UE_LOG(LogTemp, Warning, TEXT("RecoilIncCostTime %f RecoilVertCurTick %f RecoilHorCurTick %f"), RecoilIncCostTime, RecoilVertCurTick, RecoilHorCurTick);

	// 应用掉当前帧需要增加的后坐力（view kick）
	HumanCharacter->AddControllerPitchInput((RecoilVertCurTick - RecoilVertLastTick) * -1.f); // InputPitchScale_DEPRECATED == -2.5
	HumanCharacter->AddControllerYawInput(RecoilHorCurTick - RecoilHorLastTick); // InputYawScale_DEPRECATED == 2.5

	// 记录后坐力
	RecoilVertLastTick = RecoilVertCurTick;
	RecoilHorLastTick = RecoilHorCurTick;
}

// 开火产生的后坐力应用完成后，恢复后坐力
void URecoilComponent::PollRecoverRecoil(float DeltaSeconds)
{
	if (RecoilDecCostTime >= RecoilDecTime) return;

	if (HumanCharacter == nullptr || HumanCharacter->CombatComponent == nullptr) return;
	AWeapon* Weapon = HumanCharacter->CombatComponent->GetCurWeapon();
	if (Weapon == nullptr) return;

	// 累计恢复后坐力耗时
	RecoilDecCostTime += DeltaSeconds;

	// 恢复后坐力完毕
	if (RecoilDecCostTime >= RecoilDecTime)
	{
		// 约束耗时
		RecoilDecCostTime = RecoilDecTime;
	}

	// 当前帧需要恢复的后坐力
	float Alpha = RecoilDecCostTime / RecoilDecTime;
	float RecoilVertRecoverCurTick = FMath::InterpEaseInOut(0.f, RecoilVertRecoverTotal, Alpha, 2.f);
	float RecoilHorRecoverCurTick = FMath::InterpEaseInOut(0.f, RecoilHorRecoverTotal, Alpha, 2.f);

	// UE_LOG(LogTemp, Warning, TEXT("RecoilDecCostTime %f RecoilVertRecoverCurTick %f RecoilHorRecoverCurTick %f"), RecoilDecCostTime, RecoilVertRecoverCurTick, RecoilHorRecoverCurTick);

	// 恢复垂直后坐力
	float DiffRecoilVert = RecoilVertRecoverCurTick - RecoilVertRecoverLastTick;
	HumanCharacter->AddControllerPitchInput(-DiffRecoilVert * -1.f); // 恢复时准星是向下的
	RecoilVertTotal -= DiffRecoilVert;
	// 限制垂直后坐力
	if (RecoilVertTotal < 0.f) RecoilVertTotal = 0.f;

	// 恢复水平后坐力
	float DiffRecoilHor = RecoilHorRecoverCurTick - RecoilHorRecoverLastTick;
	HumanCharacter->AddControllerYawInput(-DiffRecoilHor);

	// 水平后坐力为正值（准心在右侧）
	if (RecoilHorTotal > 0.f)
	{
		RecoilHorTotal -= DiffRecoilHor;
		// 限制水平后坐力
		if (RecoilHorTotal < 0.f) RecoilHorTotal = 0.f;
	}
	// 水平后坐力为负值（准心在左侧）
	else if (RecoilHorTotal < 0.f)
	{
		RecoilHorTotal -= DiffRecoilHor;
		if (RecoilHorTotal > 0.f) RecoilHorTotal = 0.f;
	}

	// 记录上一帧后坐力
	RecoilVertRecoverLastTick = RecoilVertRecoverCurTick;
	RecoilHorRecoverLastTick = RecoilHorRecoverCurTick;
}

// 获取子弹偏移
float URecoilComponent::GetCurRecoilVert()
{
	if (HumanCharacter == nullptr || HumanCharacter->CombatComponent == nullptr)
	{
		return RecoilVertTotal;
	}
	
	return RecoilVertTotal * (1 - HumanCharacter->CombatComponent->AimingProgress);
}

float URecoilComponent::GetCurRecoilHor()
{
	if (HumanCharacter == nullptr || HumanCharacter->CombatComponent == nullptr)
	{
		return RecoilHorTotal;
	}
	
	return RecoilHorTotal * (1 - HumanCharacter->CombatComponent->AimingProgress);
}

// TODO 跳跃、移动速度影响精度
FVector2D URecoilComponent::GetCurSpreadVector()
{
	if (HumanCharacter && HumanCharacter->CombatComponent)
	{
		AWeapon* Weapon = HumanCharacter->CombatComponent->GetCurWeapon();
		if (Weapon && Weapon->RecoilCurve)
		{
			FVector RecoilKick = Weapon->RecoilCurve->GetVectorValue(HumanCharacter->CombatComponent->CurShotCount);
			
			return FMath::RandPointInCircle(RecoilKick.Z);
		}
	}

	return FVector2D::ZeroVector;
}
