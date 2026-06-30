#include "TeleportPortal.h"

#include "MetaSoundSource.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h" 

ATeleportPortal::ATeleportPortal()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
    
    PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
    PortalMesh->SetupAttachment(RootComponent);
    PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TriggerBox->SetupAttachment(RootComponent);
    // 设置碰撞预设，需确保子弹和投掷物的碰撞设置能与 "Trigger" 产生 Overlap
    TriggerBox->SetCollisionProfileName(TEXT("Trigger"));

    ExitPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("ExitPoint"));
    ExitPoint->SetupAttachment(RootComponent);
    ExitPoint->ArrowColor = FColor::Cyan;
   
   // 初始化循环音效组件
   PortalLoopAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("PortalLoopAudioComp"));
   PortalLoopAudioComp->SetupAttachment(RootComponent);
   // 禁用自动激活，我们可以在资源加载好之后手动播放
   PortalLoopAudioComp->bAutoActivate = false;
}

void ATeleportPortal::BeginPlay()
{
    Super::BeginPlay();

    // 多人游戏中，传送逻辑仅由具有权威性的服务器处理
    if (HasAuthority())
    {
       TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ATeleportPortal::OnOverlapBegin);
    }
   
   if (PortalLoopSound)
   {
      PortalLoopAudioComp->SetSound(PortalLoopSound);
      PortalLoopAudioComp->Play();
   }
}

void ATeleportPortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 安全性检查与权限检查
    if (!HasAuthority() || !TargetPortal || !OtherActor)
    {
       return;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();
    TWeakObjectPtr<AActor> WeakOtherActor(OtherActor);

    // 1. 自动清理已销毁对象的指针或已过期的冷却记录，防止高频子弹导致 Map 无限膨胀
    for (auto It = ActorTeleportCooldowns.CreateIterator(); It; ++It)
    {
       if (!It.Key().IsValid() || (CurrentTime - It.Value() >= TeleportCooldown))
       {
          It.RemoveCurrent();
       }
    }

    // 2. 检查当前进入的 Actor 是否还在冷却中
    if (ActorTeleportCooldowns.Contains(WeakOtherActor))
    {
       return; 
    }

    // 在执行 TeleportTo 触发同步 Overlap 之前，提前锁定双方传送门的冷却 防止传送瞬间目标传送门再次触发 OnOverlapBegin 导致无限递归栈溢出
    this->ActorTeleportCooldowns.Add(WeakOtherActor, CurrentTime);
    TargetPortal->ActorTeleportCooldowns.Add(WeakOtherActor, CurrentTime);

    // 预先计算好进出传送门的空间转换变量，供所有类型的传送对象共用
    FTransform EntryTransform = this->ExitPoint->GetComponentTransform();
    FTransform ExitTransform = TargetPortal->ExitPoint->GetComponentTransform();
    // 构建一个 Yaw 轴旋转 180 度的四元数，用于翻转进出门的朝向关系
    FQuat Flip180 = FQuat(FRotator(0.f, 180.f, 0.f));
    
    bool bHasTeleported = false;

    // ------------------- [类型1：角色传送] -------------------
    if (ACharacter* OverlappingCharacter = Cast<ACharacter>(OtherActor))
    {
       // 处理角色的速度向量转换 (保持动量方向正确)
       if (UCharacterMovementComponent* MovementComp = OverlappingCharacter->GetCharacterMovement())
       {
          FVector CurrentVelocity = MovementComp->Velocity;
          FVector LocalVelocity = EntryTransform.InverseTransformVectorNoScale(CurrentVelocity);
          FVector FlippedLocalVelocity = Flip180.RotateVector(LocalVelocity);
          FVector NewVelocity = ExitTransform.TransformVectorNoScale(FlippedLocalVelocity);
          
          MovementComp->Velocity = NewVelocity;
       }

       // 处理角色的身体朝向转换
       FQuat ActorOldRot = OverlappingCharacter->GetActorQuat();
       FQuat ActorLocalRot = EntryTransform.GetRotation().Inverse() * ActorOldRot;
       FQuat ActorNewRot = ExitTransform.GetRotation() * (Flip180 * ActorLocalRot);

       // 传送角色 (保持原有逻辑：始终从目标箭头的中心原点穿出，防止角色被卡在墙体或地面边缘)
       OverlappingCharacter->TeleportTo(ExitTransform.GetLocation(), ActorNewRot.Rotator());

       // 处理控制器的视角指向 (ControlRotation)
       if (APlayerController* PC = Cast<APlayerController>(OverlappingCharacter->GetController()))
       {
          FQuat PCOldRot = PC->GetControlRotation().Quaternion();
          FQuat PCLocalRot = EntryTransform.GetRotation().Inverse() * PCOldRot;
          FRotator PCNewRot = (ExitTransform.GetRotation() * (Flip180 * PCLocalRot)).Rotator();
          
          PC->SetControlRotation(PCNewRot);
          // 强制下发 RPC 到客户端，防止客户端视角被本地输入覆盖拉扯
          PC->ClientSetRotation(PCNewRot);
       }
       
       bHasTeleported = true;
    }
    // ------------------- [类型2：实体子弹和投掷物传送] -------------------
    else if (UProjectileMovementComponent* ProjMovement = OtherActor->FindComponentByClass<UProjectileMovementComponent>())
    {
       // 转换抛射物的相对位置 (通过计算相对于入口的空间偏移，镜像映射到出口空间)
       FVector ActorOldLocation = OtherActor->GetActorLocation();
       FVector LocalLocation = EntryTransform.InverseTransformPosition(ActorOldLocation);
       FVector FlippedLocalLocation = Flip180.RotateVector(LocalLocation);
       FVector NewLocation = ExitTransform.TransformPosition(FlippedLocalLocation);

       // 转换抛射物的速度向量
       FVector CurrentVelocity = ProjMovement->Velocity;
       FVector LocalVelocity = EntryTransform.InverseTransformVectorNoScale(CurrentVelocity);
       FVector FlippedLocalVelocity = Flip180.RotateVector(LocalVelocity);
       FVector NewVelocity = ExitTransform.TransformVectorNoScale(FlippedLocalVelocity);
       
       ProjMovement->Velocity = NewVelocity;

       // 转换抛射物 Actor 的朝向
       FQuat ActorOldRot = OtherActor->GetActorQuat();
       FQuat ActorLocalRot = EntryTransform.GetRotation().Inverse() * ActorOldRot;
       FQuat ActorNewRot = ExitTransform.GetRotation() * (Flip180 * ActorLocalRot);

       // 传送抛射物到映射后的精确世界空间坐标
       OtherActor->TeleportTo(NewLocation, ActorNewRot.Rotator());
       
       bHasTeleported = true;
    }
    // ------------------- [类型3：纯物理模拟对象传送（如弹壳、物理碎片）] -------------------
    else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(OtherActor->GetRootComponent()))
    {
       if (PrimComp->IsSimulatingPhysics())
       {
          // 获取当前的物理属性和世界坐标
          FVector ActorOldLocation = OtherActor->GetActorLocation();
          FVector CurrentLinVel = PrimComp->GetPhysicsLinearVelocity();
          FVector CurrentAngVel = PrimComp->GetPhysicsAngularVelocityInDegrees();

          // 转换物理对象的相对位置
          FVector LocalLocation = EntryTransform.InverseTransformPosition(ActorOldLocation);
          FVector FlippedLocalLocation = Flip180.RotateVector(LocalLocation);
          FVector NewLocation = ExitTransform.TransformPosition(FlippedLocalLocation);

          // 转换线速度
          FVector LocalLinVel = EntryTransform.InverseTransformVectorNoScale(CurrentLinVel);
          FVector FlippedLocalLinVel = Flip180.RotateVector(LocalLinVel);
          FVector NewLinVel = ExitTransform.TransformVectorNoScale(FlippedLocalLinVel);

          // 转换角速度
          FVector LocalAngVel = EntryTransform.InverseTransformVectorNoScale(CurrentAngVel);
          FVector FlippedLocalAngVel = Flip180.RotateVector(CurrentAngVel); 
          FVector NewAngVel = ExitTransform.TransformVectorNoScale(FlippedLocalAngVel);

          // 转换 Actor 的朝向
          FQuat ActorOldRot = OtherActor->GetActorQuat();
          FQuat ActorLocalRot = EntryTransform.GetRotation().Inverse() * ActorOldRot;
          FQuat ActorNewRot = ExitTransform.GetRotation() * (Flip180 * ActorLocalRot);

          // 传送物理对象到映射后的精确世界空间坐标
          OtherActor->TeleportTo(NewLocation, ActorNewRot.Rotator());

          // 传送后，重新强制赋予计算好的新线速度和角速度
          PrimComp->SetPhysicsLinearVelocity(NewLinVel);
          PrimComp->SetPhysicsAngularVelocityInDegrees(NewAngVel);
          
          bHasTeleported = true;
       }
    }

    // 【容错处理】：如果最终因为类型不匹配等原因没有发生实际传送，需回退冷却状态
    if (!bHasTeleported)
    {
       this->ActorTeleportCooldowns.Remove(WeakOtherActor);
       TargetPortal->ActorTeleportCooldowns.Remove(WeakOtherActor);
    }
}
