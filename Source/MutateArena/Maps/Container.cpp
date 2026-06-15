#include "Container.h"
#include "Components/StaticMeshComponent.h"

AContainer::AContainer()
{
	PrimaryActorTick.bCanEverTick = false; 

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	RootComponent = BodyMesh;

	FrontLeftDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontLeftDoor"));
	FrontLeftDoor->SetupAttachment(RootComponent);

	FrontRightDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontRightDoor"));
	FrontRightDoor->SetupAttachment(RootComponent);

	BackLeftDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackLeftDoor"));
	BackLeftDoor->SetupAttachment(RootComponent);

	BackRightDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackRightDoor"));
	BackRightDoor->SetupAttachment(RootComponent);
}

void AContainer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 1. 处理门的显示与隐藏
	FrontLeftDoor->SetVisibility(bShowFrontLeftDoor);
	FrontRightDoor->SetVisibility(bShowFrontRightDoor);
	BackLeftDoor->SetVisibility(bShowBackLeftDoor);
	BackRightDoor->SetVisibility(bShowBackRightDoor);

	// 2. 处理碰撞逻辑（非常重要：隐藏的门必须失去碰撞）
	FrontLeftDoor->SetCollisionEnabled(bShowFrontLeftDoor ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	FrontRightDoor->SetCollisionEnabled(bShowFrontRightDoor ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	BackLeftDoor->SetCollisionEnabled(bShowBackLeftDoor ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	BackRightDoor->SetCollisionEnabled(bShowBackRightDoor ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

	// 3. 处理旋转逻辑 (根据关卡设计师填入的角度调整 Yaw 轴)
	if (bShowFrontLeftDoor)  FrontLeftDoor->SetRelativeRotation(FRotator(0.0f, FrontLeftDoorAngle, 0.0f));
	if (bShowFrontRightDoor) FrontRightDoor->SetRelativeRotation(FRotator(0.0f, FrontRightDoorAngle, 0.0f));
	if (bShowBackLeftDoor)   BackLeftDoor->SetRelativeRotation(FRotator(0.0f, BackLeftDoorAngle, 0.0f));
	if (bShowBackRightDoor)  BackRightDoor->SetRelativeRotation(FRotator(0.0f, BackRightDoorAngle, 0.0f));

	// 4. 应用静态材质实例 (MIC)
	if (SkinMap.Contains(CurrentColor))
	{
		const FContainerSkin& ActiveSkin = SkinMap[CurrentColor];

		// 主体材质
		if (ActiveSkin.BodyMaterial)
		{
			BodyMesh->SetMaterial(0, ActiveSkin.BodyMaterial);
		}

		// 交叉分配左门材质：前左门 和 后右门（后右门由左门网格体旋转180度而来）
		if (ActiveSkin.DoorLeftMaterial)
		{
			FrontLeftDoor->SetMaterial(0, ActiveSkin.DoorLeftMaterial);
			BackLeftDoor->SetMaterial(0, ActiveSkin.DoorLeftMaterial);
		}

		// 交叉分配右门材质：前右门 和 后左门（后左门由右门网格体旋转180度而来）
		if (ActiveSkin.DoorRightMaterial)
		{
			FrontRightDoor->SetMaterial(0, ActiveSkin.DoorRightMaterial);
			BackRightDoor->SetMaterial(0, ActiveSkin.DoorRightMaterial);
		}
	}
}