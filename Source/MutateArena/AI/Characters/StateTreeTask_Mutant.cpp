#include "StateTreeTask_Mutant.h"

#include "InputActionValue.h"
#include "StateTreeExecutionContext.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/MutantCharacter.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"

// 追逐目标
EStateTreeRunStatus FStateTreeTask_MutantChase::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	if (InstanceData.TargetActor == nullptr) return EStateTreeRunStatus::Failed;

	AHumanCharacter* TargetHumanCharacter = Cast<AHumanCharacter>(InstanceData.TargetActor);
	AMutantCharacter* MyMutantCharacter = Cast<AMutantCharacter>(Context.GetOwner());
	
	if (TargetHumanCharacter == nullptr || TargetHumanCharacter->bIsDead
		|| MyMutantCharacter == nullptr || MyMutantCharacter->bIsDead) return EStateTreeRunStatus::Failed;

	ABasePlayerState* TargetBasePlayerState = TargetHumanCharacter->GetPlayerState<ABasePlayerState>();
	ABasePlayerState* MyBasePlayerState = MyMutantCharacter->GetPlayerState<ABasePlayerState>();
	if (TargetBasePlayerState && MyBasePlayerState)
	{
		if (TargetBasePlayerState->Team == MyBasePlayerState->Team)
		{
			return EStateTreeRunStatus::Failed;
		}
	}
	
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeTask_MutantChase::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	if (InstanceData.TargetActor == nullptr) return EStateTreeRunStatus::Failed;
	
	AHumanCharacter* TargetHumanCharacter = Cast<AHumanCharacter>(InstanceData.TargetActor);
	AMutantCharacter* MyMutantCharacter = Cast<AMutantCharacter>(Context.GetOwner());
	
	if (TargetHumanCharacter == nullptr || TargetHumanCharacter->bIsDead
		|| MyMutantCharacter == nullptr || MyMutantCharacter->bIsDead) return EStateTreeRunStatus::Failed;
	
	ABasePlayerState* TargetBasePlayerState = TargetHumanCharacter->GetPlayerState<ABasePlayerState>();
	ABasePlayerState* MyBasePlayerState = MyMutantCharacter->GetPlayerState<ABasePlayerState>();
	if (TargetBasePlayerState && MyBasePlayerState)
	{
		if (TargetBasePlayerState->Team == MyBasePlayerState->Team)
		{
			return EStateTreeRunStatus::Failed;
		}
	}
	
	// [3. 获取位置]
	FVector MyLoc = MyMutantCharacter->GetActorLocation();
	FVector TargetLoc = InstanceData.TargetActor->GetActorLocation();

	// 计算距离的平方 (DistSquared)。
	// 计算机算平方根 (Sqrt) 很慢，所以游戏开发中通常比较“距离的平方”和“半径的平方”，效果一样但性能更好。
	float DistSq = FVector::DistSquared(MyLoc, TargetLoc);

	// [4. 判断是否到达终点]
	// 如果当前距离 < 设定的停止距离 (AcceptanceRadius)，说明走到了。
	// 返回 Succeeded，StateTree 就会切到下一个节点 (比如开始攻击)。
	if (DistSq < FMath::Square(InstanceData.AcceptanceRadius))
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// [5. 获取导航系统]
	// 导航系统 (NavSystem) 是负责计算 "怎么绕过墙壁走到目标" 的系统。
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(MyMutantCharacter->GetWorld());
	if (!NavSys)
	{
		// 如果地图里没有放 NavMeshBoundsVolume (没有导航网格)，就只能沿直线走过去 (可能会撞墙)。
		FVector Direction = (TargetLoc - MyLoc).GetSafeNormal();
		MyMutantCharacter->AddMovementInput(Direction, 1.0f);
		return EStateTreeRunStatus::Running;
	}

	// [6. 核心：寻路 (Pathfinding)]
	// 这一句的意思是：“请立刻算出从 MyLoc 到 TargetActor 的路径”。
	// 注意：Synchronously 表示“同步”，意味着会卡住主线程计算完才继续。
	// 在每帧 Tick 里调这个其实性能消耗较大，但对于单机托管或者少量 AFK 玩家来说没问题。
	UNavigationPath* NavPath = NavSys->FindPathToActorSynchronously(MyMutantCharacter->GetWorld(), MyLoc, InstanceData.TargetActor);

	// [7. 沿着路径移动]
	// PathPoints 是路径上的一系列点。
	// Point[0] 是你当前站的位置。
	// Point[1] 是你要迈出的下一步的位置。
	if (NavPath && NavPath->PathPoints.Num() > 1)
	{
		FVector NextPoint = NavPath->PathPoints[1]; // 获取下一个路点

		// 计算方向向量：(目的地 - 当前地).标准化
		FVector Direction = (NextPoint - MyLoc).GetSafeNormal();

		// [模拟按键]：相当于玩家按下了 W 键，向该方向移动
		MyMutantCharacter->AddMovementInput(Direction, 1.0f);

		// [8. 模拟鼠标转向]
		// 对于 PlayerController，光让角色移动是不够的，必须旋转 Controller (摄像机/准星)
		if (APlayerController* PC = Cast<APlayerController>(MyMutantCharacter->GetController()))
		{
			FRotator CurrentRot = PC->GetControlRotation();
			FRotator MoveDirRot = Direction.Rotation(); // 移动方向产生的旋转
			
			FRotator LookAtTargetRot = UKismetMathLibrary::FindLookAtRotation(MyLoc + FVector(0, 0, 60), TargetLoc + FVector(0, 0, 60));

			// 组合出最终的完美目标旋转：Yaw(左右)看移动方向，Pitch(上下)看敌人
			FRotator DesiredRot = FRotator(LookAtTargetRot.Pitch, MoveDirRot.Yaw, 0.f);

			// 统一使用 RInterpTo 处理整个 Rotator，它会自动算出最短的旋转路径，完美避开砸地问题
			FRotator NewRot = FMath::RInterpTo(CurrentRot, DesiredRot, DeltaTime, InstanceData.RotationInterpSpeed);

			PC->SetControlRotation(NewRot);
		}
	}
	else
	{
		// [9. 容错处理]
		// 如果寻路失败了 (比如敌人跳到了导航网格外面)，就死马当活马医，直接沿直线走。
		FVector Direction = (TargetLoc - MyLoc).GetSafeNormal();
		MyMutantCharacter->AddMovementInput(Direction, 1.0f);
	}

	// [10. 继续运行]
	// 因为还没到达终点 (DistSq check 在上面)，所以返回 Running，下一帧继续执行这个函数。
	return EStateTreeRunStatus::Running;
}

// 攻击目标
EStateTreeRunStatus FStateTreeTask_MutantAttack::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	if (InstanceData.TargetActor == nullptr) return EStateTreeRunStatus::Failed;
	
	AHumanCharacter* TargetHumanCharacter = Cast<AHumanCharacter>(InstanceData.TargetActor);
	AMutantCharacter* MyMutantCharacter = Cast<AMutantCharacter>(Context.GetOwner());
	
	if (TargetHumanCharacter == nullptr || TargetHumanCharacter->bIsDead
		|| MyMutantCharacter == nullptr || MyMutantCharacter->bIsDead) return EStateTreeRunStatus::Failed;
	
	ABasePlayerState* TargetBasePlayerState = TargetHumanCharacter->GetPlayerState<ABasePlayerState>();
	ABasePlayerState* MyBasePlayerState = MyMutantCharacter->GetPlayerState<ABasePlayerState>();
	if (TargetBasePlayerState && MyBasePlayerState)
	{
		if (TargetBasePlayerState->Team == MyBasePlayerState->Team)
		{
			return EStateTreeRunStatus::Failed;
		}
	}
	
	MyMutantCharacter->HeavyAttackButtonPressed(FInputActionValue());
	
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeTask_MutantAttack::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	if (InstanceData.TargetActor == nullptr) return EStateTreeRunStatus::Failed;
	
	AHumanCharacter* TargetHumanCharacter = Cast<AHumanCharacter>(InstanceData.TargetActor);
	AMutantCharacter* MyMutantCharacter = Cast<AMutantCharacter>(Context.GetOwner());
	
	if (TargetHumanCharacter == nullptr || TargetHumanCharacter->bIsDead
		|| MyMutantCharacter == nullptr || MyMutantCharacter->bIsDead) return EStateTreeRunStatus::Failed;
	
	ABasePlayerState* TargetBasePlayerState = TargetHumanCharacter->GetPlayerState<ABasePlayerState>();
	ABasePlayerState* MyBasePlayerState = MyMutantCharacter->GetPlayerState<ABasePlayerState>();
	if (TargetBasePlayerState && MyBasePlayerState)
	{
		if (TargetBasePlayerState->Team == MyBasePlayerState->Team)
		{
			return EStateTreeRunStatus::Failed;
		}
	}

	// 如果有目标，进行瞄准
	if (APlayerController* PC = Cast<APlayerController>(MyMutantCharacter->GetController()))
	{
		FVector MyLoc = MyMutantCharacter->GetActorLocation();
		FVector TargetLoc = InstanceData.TargetActor->GetActorLocation();

		FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(MyLoc + FVector(0, 0, 60), TargetLoc + FVector(0, 0, 60));

		FRotator CurrentRot = PC->GetControlRotation();
		FRotator NewRot = FMath::RInterpTo(CurrentRot, LookAtRot, DeltaTime, InstanceData.RotationInterpSpeed);

		PC->SetControlRotation(NewRot);
	}

	// 当动画蒙太奇结束（通过AnimNotify将状态重置为Ready）时，任务成功
	if (MyMutantCharacter->MutantState == EMutantState::Ready)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeTask_MutantAttack::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (AMutantCharacter* MyMutantCharacter = Cast<AMutantCharacter>(Context.GetOwner()))
	{
		MyMutantCharacter->HeavyAttackButtonReleased(FInputActionValue());
	}
}

// 随机走动
EStateTreeRunStatus FStateTreeTask_MutantWander::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	AMutantCharacter* MyMutantCharacter = Cast<AMutantCharacter>(Context.GetOwner());
	
	if (MyMutantCharacter == nullptr || MyMutantCharacter->bIsDead) 
	{
		return EStateTreeRunStatus::Failed;
	}

	// 获取导航系统，在角色周围寻找一个随机可达点
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(MyMutantCharacter->GetWorld()))
	{
		FNavLocation RandomNavLocation;
		// 使用 GetRandomReachablePointInRadius 获取半径内的随机点
		if (NavSys->GetRandomReachablePointInRadius(MyMutantCharacter->GetActorLocation(), InstanceData.WanderRadius, RandomNavLocation))
		{
			InstanceData.TargetLocation = RandomNavLocation.Location;
			return EStateTreeRunStatus::Running;
		}
	}

	// 如果找不到导航点，直接失败
	return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FStateTreeTask_MutantWander::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// UE_LOG(LogTemp, Warning, TEXT("随机走动 Tick"));
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	AMutantCharacter* MyMutantCharacter = Cast<AMutantCharacter>(Context.GetOwner());
	
	if (MyMutantCharacter == nullptr || MyMutantCharacter->bIsDead) return EStateTreeRunStatus::Failed;

	FVector MyLoc = MyMutantCharacter->GetActorLocation();

	// 检查是否到达了目标点
	float DistSq = FVector::DistSquared(MyLoc, InstanceData.TargetLocation);
	if (DistSq < FMath::Square(InstanceData.AcceptanceRadius))
	{
		// 到达目标点，任务成功完成
		return EStateTreeRunStatus::Succeeded;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(MyMutantCharacter->GetWorld());
	if (!NavSys)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 寻路并移动 (复用你在 Chase 中的逻辑，但目标换成了固定的随机坐标)
	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(MyMutantCharacter->GetWorld(), MyLoc, InstanceData.TargetLocation);

	if (NavPath && NavPath->PathPoints.Num() > 1)
	{
		FVector NextPoint = NavPath->PathPoints[1];
		FVector Direction = (NextPoint - MyLoc).GetSafeNormal();

		MyMutantCharacter->AddMovementInput(Direction, 0.5f); // 0.5f 让走动速度比追逐(1.0f)稍微慢一点，看起来更像闲逛

		if (APlayerController* PC = Cast<APlayerController>(MyMutantCharacter->GetController()))
		{
			FRotator CurrentRot = PC->GetControlRotation();
			FRotator MoveDirRot = Direction.Rotation(); 
			
			// 闲逛时，只要看向移动方向即可
			FRotator DesiredRot = FRotator(0.f, MoveDirRot.Yaw, 0.f);
			FRotator NewRot = FMath::RInterpTo(CurrentRot, DesiredRot, DeltaTime, InstanceData.RotationInterpSpeed);

			PC->SetControlRotation(NewRot);
		}
	}
	else
	{
		// 如果路径突然无效了(比如中途被动态障碍物堵死)，可以返回 Succeeded 让他提前结束走动进入发呆状态
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}
