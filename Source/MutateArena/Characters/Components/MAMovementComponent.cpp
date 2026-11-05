#include "MAMovementComponent.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "MutateArena/Characters/BaseCharacter.h"

UMAMovementComponent::UMAMovementComponent()
{
}

bool UMAMovementComponent::CanAttemptJump() const
{
   // 移除引擎源码中的 !bWantsToCrouch 限制，允许蹲跳
   return IsJumpAllowed() && (IsMovingOnGround() || IsFalling());
}

void UMAMovementComponent::ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations)
{
   // 记录落地时间并打开 bCanTriggerBHop
   LastLandedTime = GetWorld()->GetTimeSeconds();
   bCanTriggerBHop = true;

   Super::ProcessLanded(Hit, remainingTime, Iterations);
}

bool UMAMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
{
   if (ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(CharacterOwner))
   {
      const double CurrentTime = GetWorld()->GetTimeSeconds();

      // 无论是“落地前 BHopPreLandBufferTime 预输入(在Landed里触发Jump)”还是“落地后 BHopPostLandGraceTime 内按键”，
      // 最终走到 DoJump 时，距离 ProcessLanded 的时间差都在 BHopPostLandGraceTime 以内！
      // 服务端放宽 0.02s 容差以吸收网络帧切片误差
      const float PostLandWindow = (BaseCharacter->HasAuthority() && !BaseCharacter->IsLocallyControlled()) ? BHopPostLandGraceTime + 0.02f : BHopPostLandGraceTime;
      const bool bIsBHop = bCanTriggerBHop && ((CurrentTime - LastLandedTime) <= PostLandWindow);

      if (bIsBHop)
      {
         JumpZVelocity = BaseCharacter->GetJumpZVelocity() * 1.2f;
      }
      else
      {
         JumpZVelocity = BaseCharacter->GetJumpZVelocity();
      }

      // 消耗掉连跳状态，防止单次落地多次触发
      bCanTriggerBHop = false;
   }

   return Super::DoJump(bReplayingMoves, DeltaTime);
}

float UMAMovementComponent::GetMaxSpeed() const
{
   if (IsCrouching())
   {
      // 1. 在空中蹲跳时，最大速度上限按正常站立行走速度（或专门的鬼跳速度上限）计算
      // 否则在空中按住 WASD 时，AirControl 会把你的目标速度限制在 MaxWalkSpeedCrouched
      if (IsFalling())
      {
         return MaxWalkSpeed;
      }

      // 2. 落地瞬间如果处于鬼跳宽限期内，同样返回正常速度上限
      // 防止 CalcVelocity 在落地那一帧把 Velocity 直接 Clamp 到蹲伏速度
      if (GetWorld() && GetWorld()->GetTimeSeconds() - LastLandedTime <= BHopPostLandGraceTime)
      {
         return MaxWalkSpeed;
      }
   }

   return Super::GetMaxSpeed();
}

// 临时提高 MaxStepHeight 为45.f（ABaseCharacter 中设为了 MaxStepHeight = 20.f）。允许角色跨上较高障碍物，同时保证从较低边缘走下时能脱离地面进入 Falling 状态并触发 Landed 事件
bool UMAMovementComponent::StepUp(const FVector& GravDir, const FVector& Delta, const FHitResult& InHit, FStepDownResult* OutStepDownResult)
{
	TGuardValue<float> StepHeightGuard(MaxStepHeight, 45.f);

	return Super::StepUp(GravDir, Delta, InHit, OutStepDownResult);
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

void UMAMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
   // 1. 空中阶段：执行 Quake/Source 风格的空速投影加速
   if (MovementMode == MOVE_Falling && CustomMovementMode == CMOVE_None && !HasAnimRootMotion() && !CurrentRootMotion.HasActiveRootMotionSources())
   {
      const float OriginalZ = Velocity.Z;
      const FVector WishDir = Acceleration.GetSafeNormal2D();
      const float WishSpeed = WishDir.IsNearlyZero() ? 0.0f : MaxAirSpeed;

      ApplyQuakeAirAccelerate(DeltaTime, WishDir, WishSpeed);

      Velocity.Z = OriginalZ;
      return; // 绕过引擎默认的空气阻力和截断
   }

   // 2. 地面蹲伏阶段：如果处于鬼跳落地宽限期内，将摩擦力与制动减速度清零
   if (IsCrouching() && IsMovingOnGround() && GetWorld())
   {
      if (GetWorld()->GetTimeSeconds() - LastLandedTime <= BHopPostLandGraceTime)
      {
         Friction = 0.f;
         BrakingDeceleration = 0.f;
      }
   }

   // 3. 地面及其他常规移动状态（走原生的摩擦力、制动及爬坡结算）
   Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
}

void UMAMovementComponent::ApplyQuakeAirAccelerate(float DeltaTime, FVector WishDir, float WishSpeed)
{
   // 如果玩家在空中松开了键盘，不提供额外加速（保留现有动量）
   if (WishSpeed == 0.0f) 
   {
      return;
   }

   // 1. 获取当前速度的平面向量 (忽略 Z 轴)
   FVector Velocity2D = FVector(Velocity.X, Velocity.Y, 0.0f);

   // 2. 【核心：基于向量点乘的投影】计算当前平面速度在玩家期望方向 (WishDir) 上的投影大小
   float CurrentSpeed = FVector::DotProduct(Velocity2D, WishDir);

   // 3. 计算加速余量 (期望速度 - 投影速度)
   float AddSpeed = WishSpeed - CurrentSpeed;

   // 如果投影方向上的速度已经达到或超过了期望速度，停止在该方向施加加速度
   // 这就是为什么鼠标转动能突破速度限制：只要鼠标转得够快，投影速度 CurrentSpeed 就会很小甚至为负！
   if (AddSpeed <= 0.0f) 
   {
      return;
   }

   // 4. 计算当前帧理论上能提供的加速度
   float AccelSpeed = AirAcceleration * DeltaTime;

   // 5. 截断加速量，防止加速过猛超过了加成余量
   AccelSpeed = FMath::Min(AccelSpeed, AddSpeed);

   // 6. 将计算得出的纯净加速量应用到总速度上
   Velocity += AccelSpeed * WishDir;
}
