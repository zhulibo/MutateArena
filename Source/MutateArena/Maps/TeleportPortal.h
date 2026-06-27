#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TeleportPortal.generated.h"

class UBoxComponent;
class UArrowComponent;
class UStaticMeshComponent;
class USceneComponent;

UCLASS()
class MUTATEARENA_API ATeleportPortal : public AActor
{
	GENERATED_BODY()
    
public: 
	ATeleportPortal();

protected:
	virtual void BeginPlay() override;

	// 触发器重叠事件
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	UPROPERTY(VisibleAnywhere)
	USceneComponent* SceneRoot;
    
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* PortalMesh;
    
	// 触发区域
	UPROPERTY(VisibleAnywhere)
	UBoxComponent* TriggerBox;

	// 传送出口标记（位置与朝向）
	UPROPERTY(VisibleAnywhere)
	UArrowComponent* ExitPoint;

	// 目标传送门（在关卡蓝图中相互指定）
	UPROPERTY(EditAnywhere)
	ATeleportPortal* TargetPortal;

	// 传送冷却时间，防止两扇门之间无限循环
	UPROPERTY(EditAnywhere)
	float TeleportCooldown = 0.5f;

	// 记录每个对象上次传送的时间，使用弱指针防止子弹/弹壳销毁后产生悬指针
	TMap<TWeakObjectPtr<AActor>, float> ActorTeleportCooldowns;
	
};
