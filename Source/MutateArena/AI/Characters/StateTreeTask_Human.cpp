#include "StateTreeTask_Human.h"

#include "InputActionValue.h"
#include "StateTreeExecutionContext.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Characters/Components/CombatComponent.h"
#include "MutateArena/Characters/Components/CombatStateType.h"
#include "MutateArena/Equipments/Weapon.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"

// 追逐目标
EStateTreeRunStatus FStateTreeTask_HumanChase::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	if (InstanceData.TargetActor == nullptr) return EStateTreeRunStatus::Failed;
	
	ABaseCharacter* TargetBaseCharacter = Cast<ABaseCharacter>(InstanceData.TargetActor);
	AHumanCharacter* MyHumanCharacter = Cast<AHumanCharacter>(Context.GetOwner());
	
	if (TargetBaseCharacter == nullptr || TargetBaseCharacter->bIsDead || MyHumanCharacter == nullptr) return EStateTreeRunStatus::Failed;

	ABasePlayerState* TargetBasePlayerState = TargetBaseCharacter->GetPlayerState<ABasePlayerState>();
	ABasePlayerState* MyBasePlayerState = MyHumanCharacter->GetPlayerState<ABasePlayerState>();
	if (TargetBasePlayerState && MyBasePlayerState)
	{
		if (TargetBasePlayerState->Team == MyBasePlayerState->Team)
		{
			return EStateTreeRunStatus::Failed;
		}
	}
	
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeTask_HumanChase::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	if (InstanceData.TargetActor == nullptr) return EStateTreeRunStatus::Failed;
	
	ABaseCharacter* TargetBaseCharacter = Cast<ABaseCharacter>(InstanceData.TargetActor);
	AHumanCharacter* MyHumanCharacter = Cast<AHumanCharacter>(Context.GetOwner());
	
	if (TargetBaseCharacter == nullptr || TargetBaseCharacter->bIsDead || MyHumanCharacter == nullptr) return EStateTreeRunStatus::Failed;

	ABasePlayerState* TargetBasePlayerState = TargetBaseCharacter->GetPlayerState<ABasePlayerState>();
	ABasePlayerState* MyBasePlayerState = MyHumanCharacter->GetPlayerState<ABasePlayerState>();
	if (TargetBasePlayerState && MyBasePlayerState)
	{
		if (TargetBasePlayerState->Team == MyBasePlayerState->Team)
		{
			return EStateTreeRunStatus::Failed;
		}
	}

	// [3. 获取位置]
	FVector MyLoc = MyHumanCharacter->GetActorLocation();
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
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(MyHumanCharacter->GetWorld());
	if (!NavSys)
	{
		// 如果地图里没有放 NavMeshBoundsVolume (没有导航网格)，就只能沿直线走过去 (可能会撞墙)。
		FVector Direction = (TargetLoc - MyLoc).GetSafeNormal();
		MyHumanCharacter->AddMovementInput(Direction, 1.0f);
		return EStateTreeRunStatus::Running;
	}

	// [6. 核心：寻路 (Pathfinding)]
	// 这一句的意思是：“请立刻算出从 MyLoc 到 TargetActor 的路径”。
	// 注意：Synchronously 表示“同步”，意味着会卡住主线程计算完才继续。
	// 在每帧 Tick 里调这个其实性能消耗较大，但对于单机托管或者少量 AFK 玩家来说没问题。
	UNavigationPath* NavPath = NavSys->FindPathToActorSynchronously(MyHumanCharacter->GetWorld(), MyLoc, InstanceData.TargetActor);

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
		MyHumanCharacter->AddMovementInput(Direction, 1.0f);

		// [8. 模拟鼠标转向]
		// 对于 PlayerController，光让角色移动是不够的，必须旋转 Controller (摄像机/准星)
		if (APlayerController* PC = Cast<APlayerController>(MyHumanCharacter->GetController()))
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
		MyHumanCharacter->AddMovementInput(Direction, 1.0f);
	}

	// [10. 继续运行]
	// 因为还没到达终点 (DistSq check 在上面)，所以返回 Running，下一帧继续执行这个函数。
	return EStateTreeRunStatus::Running;
}

void FStateTreeTask_HumanChase::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
}

// 攻击目标
EStateTreeRunStatus FStateTreeTask_HumanFire::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	if (InstanceData.TargetActor == nullptr) return EStateTreeRunStatus::Failed;
	
	ABaseCharacter* TargetBaseCharacter = Cast<ABaseCharacter>(InstanceData.TargetActor);
	AHumanCharacter* MyHumanCharacter = Cast<AHumanCharacter>(Context.GetOwner());
	
	if (TargetBaseCharacter == nullptr || TargetBaseCharacter->bIsDead || MyHumanCharacter == nullptr) return EStateTreeRunStatus::Failed;

	ABasePlayerState* TargetBasePlayerState = TargetBaseCharacter->GetPlayerState<ABasePlayerState>();
	ABasePlayerState* MyBasePlayerState = MyHumanCharacter->GetPlayerState<ABasePlayerState>();
	if (TargetBasePlayerState && MyBasePlayerState)
	{
		if (TargetBasePlayerState->Team == MyBasePlayerState->Team)
		{
			return EStateTreeRunStatus::Failed;
		}
	}
	
	if (MyHumanCharacter->CombatComponent)
	{
		if (AWeapon* Weapon = MyHumanCharacter->CombatComponent->GetCurWeapon())
		{
			if (Weapon->Ammo != 0)
			{
				InstanceData.bIsShooting = true;
				InstanceData.CurrentStateTimer = 0.0f;
				InstanceData.CurrentTargetDuration = FMath::RandRange(0.5f, 0.9f);

				MyHumanCharacter->FireButtonPressed(FInputActionValue());

				return EStateTreeRunStatus::Running;
			}
			else
			{
				return EStateTreeRunStatus::Succeeded;
			}
		}
	}
	
	return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FStateTreeTask_HumanFire::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	if (InstanceData.TargetActor == nullptr) return EStateTreeRunStatus::Failed;
	
	ABaseCharacter* TargetBaseCharacter = Cast<ABaseCharacter>(InstanceData.TargetActor);
	AHumanCharacter* MyHumanCharacter = Cast<AHumanCharacter>(Context.GetOwner());
	
	if (TargetBaseCharacter == nullptr || TargetBaseCharacter->bIsDead || MyHumanCharacter == nullptr) return EStateTreeRunStatus::Failed;

	ABasePlayerState* TargetBasePlayerState = TargetBaseCharacter->GetPlayerState<ABasePlayerState>();
	ABasePlayerState* MyBasePlayerState = MyHumanCharacter->GetPlayerState<ABasePlayerState>();
	if (TargetBasePlayerState && MyBasePlayerState)
	{
		if (TargetBasePlayerState->Team == MyBasePlayerState->Team)
		{
			return EStateTreeRunStatus::Failed;
		}
	}
	
	// 没有弹药了
	if (MyHumanCharacter->CombatComponent)
	{
		if (AWeapon* Weapon = MyHumanCharacter->CombatComponent->GetCurWeapon())
		{
			if (Weapon->Ammo == 0)
			{
				return EStateTreeRunStatus::Succeeded;
			}
		}
	}
	
	// 累积已运行时间
	InstanceData.CurrentStateTimer += DeltaTime;

	// 射击或停顿已运行时间达到要求
	if (InstanceData.CurrentStateTimer >= InstanceData.CurrentTargetDuration)
	{
		InstanceData.CurrentStateTimer = 0.f;
		InstanceData.bIsShooting = !InstanceData.bIsShooting;

		if (InstanceData.bIsShooting)
		{
			InstanceData.CurrentTargetDuration = FMath::RandRange(0.6f, 0.8f);
			MyHumanCharacter->FireButtonPressed(FInputActionValue());
		}
		else
		{
			InstanceData.CurrentTargetDuration = FMath::RandRange(2.f, 3.f);
			MyHumanCharacter->FireButtonReleased(FInputActionValue());
		}
	}
	
	if (APlayerController* PC = Cast<APlayerController>(MyHumanCharacter->GetController()))
	{
		FVector MyLoc = MyHumanCharacter->GetActorLocation();
		FVector TargetLoc = InstanceData.TargetActor->GetActorLocation();

		FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(MyLoc + FVector(0, 0, 60), TargetLoc + FVector(0, 0, 60));

		FRotator CurrentRot = PC->GetControlRotation();
		FRotator NewRot = FMath::RInterpTo(CurrentRot, LookAtRot, DeltaTime, InstanceData.RotationInterpSpeed);

		PC->SetControlRotation(NewRot);
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeTask_HumanFire::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (AHumanCharacter* MyHumanCharacter = Cast<AHumanCharacter>(Context.GetOwner()))
	{
		MyHumanCharacter->FireButtonReleased(FInputActionValue());
	}
}

// 换弹
EStateTreeRunStatus FStateTreeTask_HumanReload::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AHumanCharacter* MyHumanCharacter = Cast<AHumanCharacter>(Context.GetOwner());
	if (MyHumanCharacter ==nullptr || MyHumanCharacter->CombatComponent == nullptr)
	{
		return EStateTreeRunStatus::Failed;
	}
	
	if (AWeapon* Weapon = MyHumanCharacter->CombatComponent->GetCurWeapon())
	{
		if (Weapon->Ammo == 0)
		{
			MyHumanCharacter->ReloadButtonPressed(FInputActionValue());
			
			return EStateTreeRunStatus::Running;
		}
	}
	
	return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FStateTreeTask_HumanReload::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	AHumanCharacter* MyHumanCharacter = Cast<AHumanCharacter>(Context.GetOwner());
	if (MyHumanCharacter == nullptr || MyHumanCharacter->CombatComponent == nullptr)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (MyHumanCharacter->CombatComponent->CombatState == ECombatState::Reloading)
	{
		return EStateTreeRunStatus::Running;
	}

	if (AWeapon* Weapon = MyHumanCharacter->CombatComponent->GetCurWeapon())
	{
		if (Weapon->Ammo > 0 || Weapon->CarriedAmmo == 0)
		{
			return EStateTreeRunStatus::Succeeded;
		}
	}

	return EStateTreeRunStatus::Failed;
}

// 随机走动
EStateTreeRunStatus FStateTreeTask_HumanWander::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	AHumanCharacter* MyHumanCharacter = Cast<AHumanCharacter>(Context.GetOwner());
	
	if (MyHumanCharacter == nullptr || MyHumanCharacter->bIsDead) 
	{
		return EStateTreeRunStatus::Failed;
	}

	// 获取导航系统，在角色周围寻找一个随机可达点
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(MyHumanCharacter->GetWorld()))
	{
		FNavLocation RandomNavLocation;
		// 使用 GetRandomReachablePointInRadius 获取半径内的随机点
		if (NavSys->GetRandomReachablePointInRadius(MyHumanCharacter->GetActorLocation(), InstanceData.WanderRadius, RandomNavLocation))
		{
			InstanceData.TargetLocation = RandomNavLocation.Location;
			return EStateTreeRunStatus::Running;
		}
	}

	// 如果找不到导航点，直接失败
	return EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FStateTreeTask_HumanWander::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	AHumanCharacter* MyHumanCharacter = Cast<AHumanCharacter>(Context.GetOwner());
	
	if (MyHumanCharacter == nullptr || MyHumanCharacter->bIsDead) return EStateTreeRunStatus::Failed;

	FVector MyLoc = MyHumanCharacter->GetActorLocation();

	// 检查是否到达了目标点
	float DistSq = FVector::DistSquared(MyLoc, InstanceData.TargetLocation);
	if (DistSq < FMath::Square(InstanceData.AcceptanceRadius))
	{
		// 到达目标点，任务成功完成
		return EStateTreeRunStatus::Succeeded;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(MyHumanCharacter->GetWorld());
	if (!NavSys)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 寻路并移动 (复用你在 Chase 中的逻辑，但目标换成了固定的随机坐标)
	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(MyHumanCharacter->GetWorld(), MyLoc, InstanceData.TargetLocation);

	if (NavPath && NavPath->PathPoints.Num() > 1)
	{
		FVector NextPoint = NavPath->PathPoints[1];
		FVector Direction = (NextPoint - MyLoc).GetSafeNormal();

		MyHumanCharacter->AddMovementInput(Direction, 0.5f); // 0.5f 让走动速度比追逐(1.0f)稍微慢一点，看起来更像闲逛

		if (APlayerController* PC = Cast<APlayerController>(MyHumanCharacter->GetController()))
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
