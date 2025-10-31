#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RecoilComponent.generated.h"

enum class ECombatState : uint8;
enum class EEquipmentType : uint8;

UENUM(BlueprintType)
enum class ERecoilHorDirection : uint8
{
	Random,
	Right,
	Left,
};

// TODO 重构后坐力系统

/**
 * 开火后在一个周期内应用掉后坐力，应用完毕后，在一个周期内回复掉增加的后坐力
 * 开火会打断后坐力的应用和回复，到开火当前帧未来得及应用或未回复的坐力会被弃用
 */
UCLASS()
class MUTATEARENA_API URecoilComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URecoilComponent();
	friend class AHumanCharacter;

	void IncRecoil();

	void SetRecoilHorDirection(ERecoilHorDirection TempRecoilHorDirection);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY()
	AHumanCharacter* HumanCharacter;

	// 本次开火需要应用的目标后坐力
	float RecoilVertCurFire = 0.f;
	float RecoilHorCurFire = 0.f;
	// 上一帧的后坐力
	float RecoilVertLastTick = 0.f;
	float RecoilHorLastTick = 0.f;
	// 应用后坐力已经消耗的时间
	float RecoilIncCostTime = 0.f;
public:
	// 总后坐力
	float RecoilVertTotal = 0.f;
	float RecoilHorTotal = 0.f;
protected:
	// 水平后坐力方向
	ERecoilHorDirection RecoilHorDirection = ERecoilHorDirection::Random;
	void PollApplyRecoil(float DeltaSeconds);

	// 回复后坐力需要的时间
	float RecoilDecTime = 0.f;
	// 回复后坐力已经消耗的时间
	float RecoilDecCostTime = 0.f;
	// 需要回复的总后坐力
	float RecoilVertRecoverTotal = 0.f;
	float RecoilHorRecoverTotal = 0.f;
	// 上一帧的后坐力
	float RecoilVertRecoverLastTick = 0.f;
	float RecoilHorRecoverLastTick = 0.f;
	void PollRecoverRecoil(float DeltaSeconds);

public:
	float GetCurRecoilVert();
	float GetCurRecoilHor();
	FVector2D GetCurSpreadVector();

};
