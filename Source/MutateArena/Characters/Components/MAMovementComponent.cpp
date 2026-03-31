#include "MAMovementComponent.h"

UMAMovementComponent::UMAMovementComponent()
{
}

void UMAMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
    Super::PhysCustom(deltaTime, Iterations);

    // 根据当前的自定义模式路由到不同的逻辑
    switch (CustomMovementMode)
    {
    case CMOVE_Ladder:
       PhysLadder(deltaTime, Iterations);
       break;
    default:
       break;
    }
}

void UMAMovementComponent::PhysLadder(float deltaTime, int32 Iterations)
{
    if (deltaTime < MIN_TICK_TIME) return;

    // 【动态计算梯子速度】：读取角色的 MaxWalkSpeed，如果角色被加减速，爬梯子速度会自动同步变化
    float CurrentLadderSpeed = MaxWalkSpeed * LadderSpeedMultiplier;

    // 1. 获取输入并计算目标速度
    FVector InputAcceleration = GetCurrentAcceleration();
    
    // 提取垂直方向意图 (+1 向上, -1 向下, 0 不动)
    float ZDirection = FMath::Sign(InputAcceleration.Z);

    // 提取水平方向意图，并应用动态速度和横向灵敏度倍率
    FVector HorizontalVelocity = InputAcceleration;
    HorizontalVelocity.Z = 0.0f; 
    HorizontalVelocity = HorizontalVelocity.GetSafeNormal() * (CurrentLadderSpeed * LadderLateralSpeedMultiplier); 

    // 组合出当前帧在梯子上的期望速度
    Velocity = FVector(HorizontalVelocity.X, HorizontalVelocity.Y, ZDirection * CurrentLadderSpeed);

    // 【核心分离物理运算：先算上下，再算左右】
    
    // --- 第一步：处理垂直移动 (Z轴) ---
    if (Velocity.Z != 0.0f)
    {
       FVector ZDelta = FVector(0.0f, 0.0f, Velocity.Z) * deltaTime;
       FHitResult ZHit(1.f);
       SafeMoveUpdatedComponent(ZDelta, UpdatedComponent->GetComponentQuat(), true, ZHit);

       // 如果头顶或脚底撞到东西
       if (ZHit.IsValidBlockingHit())
       {
          // 检查我们踩到的东西是不是另一个玩家 (Pawn)
          bool bHitOtherPlayer = ZHit.GetActor() && ZHit.GetActor()->IsA<APawn>();

          // 【退到底部检测】：只有往下退，且撞到平地（符合行走坡度），且【不是踩在别人头上】时，才退出梯子
          if (Velocity.Z < 0.0f && ZHit.ImpactNormal.Z >= GetWalkableFloorZ() && !bHitOtherPlayer) 
          {
             // 确实踩到真正的地面了，退出梯子模式
             SetMovementMode(MOVE_Falling); 
             
             // 将速度清零，避免引擎将残留的向下速度计算为掉落伤害
             Velocity = FVector::ZeroVector; 
             
             return; // 已经脱离梯子，直接结束本帧的梯子物理计算
          }

          // 其他所有阻挡情况：撞到天花板、撞到下方/上方玩家的胶囊体、或者踩到不可站立的陡峭岩石
          // 仅仅停住垂直移动，保留在梯子上
          Velocity.Z = 0.0f; 
       }
    }

    // --- 第二步：处理水平移动 (XY轴) ---
    if (Velocity.SizeSquared2D() > 0.0f)
    {
       FVector XYDelta = FVector(Velocity.X, Velocity.Y, 0.0f) * deltaTime;
       FHitResult XYHit(1.f);
       SafeMoveUpdatedComponent(XYDelta, UpdatedComponent->GetComponentQuat(), true, XYHit);

       // 如果左右平移撞到了墙壁或旁边的玩家
       if (XYHit.IsValidBlockingHit())
       {
          // 使用引擎原生的滑动函数，这样在梯子上左右平移撞墙时会贴着墙壁顺滑移动
          SlideAlongSurface(XYDelta, 1.f - XYHit.Time, XYHit.Normal, XYHit, true);
       }
    }
}
