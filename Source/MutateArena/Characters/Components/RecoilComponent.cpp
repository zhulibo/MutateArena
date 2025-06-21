#include "RecoilComponent.h"

#include "CombatComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "TimerManager.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/PlayerControllers/BaseController.h"

URecoilComponent::URecoilComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void URecoilComponent::BeginPlay()
{
	Super::BeginPlay();

	// if (HumanCharacter && HumanCharacter->IsLocallyControlled())
	// {
	// 	ABaseController* BaseController = Cast<ABaseController>(HumanCharacter->Controller);
	// 	if (BaseController)
	// 	{
	// 		InputPitchScale_DEPRECATED = BaseController->GetDeprecatedInputPitchScale();
	// 		InputYawScale_DEPRECATED = BaseController->GetDeprecatedInputYawScale();
	// 	}
	// }
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

	AWeapon* Weapon = HumanCharacter->CombatComponent->GetUsingWeapon();
	if (Weapon == nullptr) return;

	// 从总后坐力中减去上次开火未及时应用的后坐力
	RecoilVertTotal -= RecoilVertTarget - RecoilVertLastTick;
	RecoilHorTotal -= RecoilHorTarget - RecoilHorLastTick;

	// 开火需要增加的目标垂直后坐力
	RecoilVertTarget = FMath::RandRange(Weapon->RecoilMinVert, Weapon->RecoilMaxVert);

	// 开火需要增加的目标水平后坐力
	if (RecoilHorDirection == ERecoilHorDirection::Random)
	{
		RecoilHorTarget = FMath::RandRange(Weapon->RecoilMinHor, Weapon->RecoilMaxHor);
	}
	else if (RecoilHorDirection == ERecoilHorDirection::Right)
	{
		// 水平后坐力方向锁定时，垂直后坐力处于最大状态且不会再增加，增加水平后坐力至2倍
		RecoilHorTarget = FMath::RandRange(0.f, Weapon->RecoilMaxHor) * 2;
	}
	else if (RecoilHorDirection == ERecoilHorDirection::Left)
	{
		RecoilHorTarget = -FMath::RandRange(0.f, Weapon->RecoilMaxHor) * 2;
	}

	// 首发后座倍率
	if (HumanCharacter->CombatComponent->bIsFirstShot)
	{
		RecoilVertTarget *= Weapon->FirstShotRecoilMul;
		RecoilHorTarget *= Weapon->FirstShotRecoilMul;
	}

	// 累计总垂直后坐力
	RecoilVertTotal += RecoilVertTarget;
	// 限制总垂直后坐力
	if (RecoilVertTotal > Weapon->RecoilTotalVertLimit)
	{
		RecoilVertTarget -= RecoilVertTotal - Weapon->RecoilTotalVertLimit;
		RecoilVertTotal = Weapon->RecoilTotalVertLimit;

		// 垂直后坐力首次达到最大时，固定水平后坐力方向
		if (RecoilHorDirection == ERecoilHorDirection::Random)
		{
			RecoilHorDirection = FMath::Abs(Weapon->RecoilMaxHor) > FMath::Abs(Weapon->RecoilMinHor)
			? ERecoilHorDirection::Right : ERecoilHorDirection::Left;
		}
	}

	// 累计总水平后坐力
	RecoilHorTotal += RecoilHorTarget;
	// 限制总水平后坐力
	if (RecoilHorTotal > Weapon->RecoilTotalHorLimit) // 达到最右值
	{
		RecoilHorTarget -= RecoilHorTotal - Weapon->RecoilTotalHorLimit;
		RecoilHorTotal = Weapon->RecoilTotalHorLimit;
		RecoilHorDirection = ERecoilHorDirection::Left; // 转向左
		// UE_LOG(LogTemp, Warning, TEXT("turn left"));
	}
	else if (RecoilHorTotal < -Weapon->RecoilTotalHorLimit) // 达到最左值
	{
		RecoilHorTarget += -RecoilHorTotal - Weapon->RecoilTotalHorLimit;
		RecoilHorTotal = -Weapon->RecoilTotalHorLimit;
		RecoilHorDirection = ERecoilHorDirection::Right; // 转向右
		// UE_LOG(LogTemp, Warning, TEXT("turn right"));
	}

	// UE_LOG(LogTemp, Warning, TEXT("RecoilVertTarget %f RecoilHorTarget %f"), RecoilVertTarget, RecoilHorTarget);
	// UE_LOG(LogTemp, Warning, TEXT("RecoilTotalVert %f RecoilTotalHor %f"), RecoilVertTotal, RecoilHorTotal);

	// 重置应用开火后坐力耗时
	RecoilIncCostTime = 0.f;
	// 重置应用开火上一帧后坐力
	RecoilVertLastTick = 0.f;
	RecoilHorLastTick = 0.f;

	// 重置回复后坐力需要的时间
	RecoilDecTime = 0.f;
	// 重置回复后坐力已经消耗的时间
	RecoilDecCostTime = 0.f;
	// 重置回复后坐力上一帧后坐力
	RecoilVertLastTick2 = 0.f;
	RecoilHorLastTick2 = 0.f;
}

// 在RecoilIncTime时间内，分多帧应用开火后产生的后坐力
void URecoilComponent::PollApplyRecoil(float DeltaSeconds)
{
	if (RecoilVertTotal == 0.f && RecoilHorTotal == 0.f) return;

	if (HumanCharacter == nullptr || HumanCharacter->CombatComponent == nullptr) return;
	AWeapon* Weapon = HumanCharacter->CombatComponent->GetUsingWeapon();
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

		// 计算回复后坐力所需时间
		RecoilDecTime = RecoilVertTotal / Weapon->RecoilDecSpeed;
		// 备份需要恢复的总后坐力
		RecoilVertTotalRecover = RecoilVertTotal;
		RecoilHorTotalRecover = RecoilHorTotal;

		// UE_LOG(LogTemp, Warning, TEXT("RecoilDecTime %f RecoilVertTotal %f RecoilHorTotal %f"), RecoilDecTime, RecoilVertTotal, RecoilHorTotal);
	}

	// 当前帧需要达到的后坐力
	float Alpha = RecoilIncCostTime / Weapon->RecoilIncTime;
	float RecoilVertCurTick = FMath::InterpEaseOut(0.f, RecoilVertTarget, Alpha, 2.f);
	float RecoilHorCurTick = FMath::InterpEaseOut(0.f, RecoilHorTarget, Alpha, 2.f);

	// UE_LOG(LogTemp, Warning, TEXT("RecoilIncCostTime %f RecoilVertCurTick %f RecoilHorCurTick %f"), RecoilIncCostTime, RecoilVertCurTick, RecoilHorCurTick);

	// 应用掉当前帧需要增加的后坐力（view kick）
	HumanCharacter->AddControllerPitchInput((RecoilVertCurTick - RecoilVertLastTick) / InputPitchScale_DEPRECATED);
	HumanCharacter->AddControllerYawInput((RecoilHorCurTick - RecoilHorLastTick) / InputYawScale_DEPRECATED);

	// 记录上一帧后坐力
	RecoilVertLastTick = RecoilVertCurTick;
	RecoilHorLastTick = RecoilHorCurTick;
}

// 开火产生的后坐力应用完成后，恢复后坐力
void URecoilComponent::PollRecoverRecoil(float DeltaSeconds)
{
	if (RecoilDecCostTime >= RecoilDecTime) return;

	if (HumanCharacter == nullptr || HumanCharacter->CombatComponent == nullptr) return;
	AWeapon* Weapon = HumanCharacter->CombatComponent->GetUsingWeapon();
	if (Weapon == nullptr) return;

	// 累计回复后坐力耗时
	RecoilDecCostTime += DeltaSeconds;

	// 回复后坐力完毕
	if (RecoilDecCostTime >= RecoilDecTime)
	{
		// 约束耗时
		RecoilDecCostTime = RecoilDecTime;
	}

	// 当前帧需要回复的后坐力
	float Alpha = RecoilDecCostTime / RecoilDecTime;
	float RecoilVertCurTick = FMath::InterpEaseInOut(0.f, RecoilVertTotalRecover, Alpha, 2.f);
	float RecoilHorCurTick = FMath::InterpEaseInOut(0.f, RecoilHorTotalRecover, Alpha, 2.f);

	// UE_LOG(LogTemp, Warning, TEXT("RecoilDecCostTime %f RecoilVertCurTick %f RecoilHorCurTick %f"), RecoilDecCostTime, RecoilVertCurTick, RecoilHorCurTick);

	// 回复垂直后坐力
	float DiffRecoilVert = RecoilVertCurTick - RecoilVertLastTick2;
	HumanCharacter->AddControllerPitchInput(-DiffRecoilVert / InputPitchScale_DEPRECATED);
	RecoilVertTotal -= DiffRecoilVert;
	// 限制垂直后坐力
	if (RecoilVertTotal < 0.f) RecoilVertTotal = 0.f;

	// 回复水平后坐力
	float DiffRecoilHor = RecoilHorCurTick - RecoilHorLastTick2;
	HumanCharacter->AddControllerYawInput(-DiffRecoilHor / InputYawScale_DEPRECATED);

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
	RecoilVertLastTick2 = RecoilVertCurTick;
	RecoilHorLastTick2 = RecoilHorCurTick;
}

// 设置水平后坐力方向
void URecoilComponent::SetRecoilHorDirection(ERecoilHorDirection TempRecoilHorDirection)
{
	RecoilHorDirection = TempRecoilHorDirection;
}

float URecoilComponent::GetCurRecoilVert()
{
	if (HumanCharacter && HumanCharacter->CombatComponent && HumanCharacter->CombatComponent->bIsAiming)
	{
		return 0.f;
	}

	return RecoilVertTotal;
}

float URecoilComponent::GetCurRecoilHor()
{
	if (HumanCharacter && HumanCharacter->CombatComponent && HumanCharacter->CombatComponent->bIsAiming)
	{
		return 0.f;
	}

	return RecoilHorTotal;
}
