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
#include "MutateArena/MutateArena.h"

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

	// 更改碰撞配置：隔离角色与高初速子弹的交互方式
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	TriggerBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	TriggerBox->SetCollisionResponseToChannel(ECC_PROJECTILE, ECollisionResponse::ECR_Block);

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

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ATeleportPortal::OnOverlapBegin);

	if (PortalLoopSound)
	{
		PortalLoopAudioComp->SetSound(PortalLoopSound);
		PortalLoopAudioComp->Play();
	}
}

bool ATeleportPortal::TryTeleportActor(AActor* ActorToTeleport)
{
	if (!TargetPortal || !ActorToTeleport)
	{
		return false;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	TWeakObjectPtr<AActor> WeakOtherActor(ActorToTeleport);

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
		return false;
	}

	// 在执行 TeleportTo 触发同步 Overlap 之前，提前锁定双方传送门的冷却 防止传送瞬间目标传送门再次触发导致无限递归栈溢出
	this->ActorTeleportCooldowns.Add(WeakOtherActor, CurrentTime);
	TargetPortal->ActorTeleportCooldowns.Add(WeakOtherActor, CurrentTime);

	// 预先计算好进出传送门的空间转换变量，供所有类型的传送对象共用
	FTransform EntryTransform = this->ExitPoint->GetComponentTransform();
	FTransform ExitTransform = TargetPortal->ExitPoint->GetComponentTransform();
	// 构建一个 Yaw 轴旋转 180 度的四元数，用于翻转进出门的朝向关系
	FQuat Flip180 = FQuat(FRotator(0.f, 180.f, 0.f));

	// 定义一个向前推出的安全距离，防止强行传送后物体边缘卡在墙壁内部
	float SafePushOffset = -40.f;
	FVector ForwardPush = ExitTransform.GetRotation().GetForwardVector() * SafePushOffset;

	bool bHasTeleported = false;

	// ------------------- [类型1：角色传送] -------------------
	if (ACharacter* OverlappingCharacter = Cast<ACharacter>(ActorToTeleport))
	{
		if (!HasAuthority()) return false;

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

		// 计算角色目标位置时，向出口前方偏移一点
		FVector DestLocation = ExitTransform.GetLocation() + ForwardPush / 2.f; // 角色胶囊体较大传的远，除以2
		OverlappingCharacter->TeleportTo(DestLocation, ActorNewRot.Rotator());

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
	else if (UProjectileMovementComponent* ProjMovement = ActorToTeleport->FindComponentByClass<UProjectileMovementComponent>())
	{
		// 转换抛射物的相对位置 (通过计算相对于入口的空间偏移，镜像映射到出口空间)
		FVector ActorOldLocation = ActorToTeleport->GetActorLocation();
		FVector LocalLocation = EntryTransform.InverseTransformPosition(ActorOldLocation);
		FVector FlippedLocalLocation = Flip180.RotateVector(LocalLocation);

		// 在最终世界坐标的基础上，叠加向前的安全偏移量
		FVector NewLocation = ExitTransform.TransformPosition(FlippedLocalLocation) + ForwardPush;

		// 转换抛射物的速度向量（若因撞击被底层清零，则采用朝向与初速恢复）
		FVector CurrentVelocity = ProjMovement->Velocity;
		if (CurrentVelocity.IsNearlyZero())
		{
			CurrentVelocity = ActorToTeleport->GetActorForwardVector() * ProjMovement->InitialSpeed;
		}

		FVector LocalVelocity = EntryTransform.InverseTransformVectorNoScale(CurrentVelocity);
		FVector FlippedLocalVelocity = Flip180.RotateVector(LocalVelocity);
		FVector NewVelocity = ExitTransform.TransformVectorNoScale(FlippedLocalVelocity);

		ProjMovement->Velocity = NewVelocity;

		// 转换抛射物 Actor 的朝向
		FQuat ActorOldRot = ActorToTeleport->GetActorQuat();
		FQuat ActorLocalRot = EntryTransform.GetRotation().Inverse() * ActorOldRot;
		FQuat ActorNewRot = ExitTransform.GetRotation() * (Flip180 * ActorLocalRot);

		// 传送抛射物到映射后的精确世界空间坐标
		ActorToTeleport->TeleportTo(NewLocation, ActorNewRot.Rotator(), false, true);

		bHasTeleported = true;
	}
	// ------------------- [类型3：纯物理模拟对象传送（如弹壳、物理碎片）] -------------------
	else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(ActorToTeleport->GetRootComponent()))
	{
		if (PrimComp->IsSimulatingPhysics())
		{
			// 获取当前的物理属性和世界坐标
			FVector ActorOldLocation = ActorToTeleport->GetActorLocation();
			FVector CurrentLinVel = PrimComp->GetPhysicsLinearVelocity();
			FVector CurrentAngVel = PrimComp->GetPhysicsAngularVelocityInDegrees();

			// 转换物理对象的相对位置
			FVector LocalLocation = EntryTransform.InverseTransformPosition(ActorOldLocation);
			FVector FlippedLocalLocation = Flip180.RotateVector(LocalLocation);

			// 叠加向前的安全偏移量
			FVector NewLocation = ExitTransform.TransformPosition(FlippedLocalLocation) + ForwardPush;

			// 转换线速度
			FVector LocalLinVel = EntryTransform.InverseTransformVectorNoScale(CurrentLinVel);
			FVector FlippedLocalLinVel = Flip180.RotateVector(LocalLinVel);
			FVector NewLinVel = ExitTransform.TransformVectorNoScale(FlippedLocalLinVel);

			// 转换角速度
			FVector LocalAngVel = EntryTransform.InverseTransformVectorNoScale(CurrentAngVel);
			FVector FlippedLocalAngVel = Flip180.RotateVector(LocalAngVel);
			FVector NewAngVel = ExitTransform.TransformVectorNoScale(FlippedLocalAngVel);

			// 转换 Actor 的朝向
			FQuat ActorOldRot = ActorToTeleport->GetActorQuat();
			FQuat ActorLocalRot = EntryTransform.GetRotation().Inverse() * ActorOldRot;
			FQuat ActorNewRot = ExitTransform.GetRotation() * (Flip180 * ActorLocalRot);

			// 传送物理对象到映射后的精确世界空间坐标
			ActorToTeleport->TeleportTo(NewLocation, ActorNewRot.Rotator(), false, true);

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
		return false;
	}

	return true;
}

void ATeleportPortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                     bool bFromSweep, const FHitResult& SweepResult)
{
	TryTeleportActor(OtherActor);
}
