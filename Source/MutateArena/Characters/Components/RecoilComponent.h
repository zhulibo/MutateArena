#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RecoilComponent.generated.h"

enum class EEquipmentSlot : uint8;

/**
 * 开火后在一段时间内应用增加的后坐力，停火后，再在一段时间内恢复增加的总后坐力。
 * 到开火当前帧，未来得及应用的后坐力会并入下一发继续应用（不会丢弃），正在恢复的后坐力会被打断。
 * 玩家反向拉鼠标压枪时，会抵消镜头需要自动恢复的后坐力，避免压枪停火后镜头继续往下掉。
 */
UCLASS()
class MUTATEARENA_API URecoilComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URecoilComponent();
	friend class AHumanCharacter;

	void IncRecoil(int32 ShotCount);
	void OnControllerLookInput(float YawInput, float PitchInput);
	void ResetRecoil();
	int32 GetCurrentShotCount() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY()
	AHumanCharacter* HumanCharacter;

	// 记录产生后坐力的武器，切枪、切刀或丢枪时清空残留后坐力
	TWeakObjectPtr<class AWeapon> ActiveWeapon;

	bool bIsApplyingRecoil = false;
	bool bIsWaitingRecover = false;
	bool bIsRecoveringRecoil = false;

	// 本次开火需要应用的目标后坐力
	float RecoilVertCurFire = 0.f;
	float RecoilHorCurFire = 0.f;
	// 上一帧已应用的后坐力
	float RecoilVertLastTick = 0.f;
	float RecoilHorLastTick = 0.f;
	// 应用后坐力已经消耗的时间
	float RecoilIncCostTime = 0.f;
public:
	// 总后坐力（用于计算子弹偏移和准星扩散）
	float RecoilVertTotal = 0.f;
	float RecoilHorTotal = 0.f;
protected:
	// 镜头还需要自动恢复的后坐力（玩家反向拉鼠标压枪时会减少）
	float CameraRecoilVertTotal = 0.f;
	float CameraRecoilHorTotal = 0.f;

	void PollApplyRecoil(float DeltaSeconds);
	void TryStartRecoverRecoil();

	// 恢复后坐力需要的时间
	float RecoilDecTime = 0.f;
	// 恢复后坐力已经消耗的时间
	float RecoilDecCostTime = 0.f;
	// 需要恢复的总后坐力与上一帧已恢复的后坐力
	float RecoilVertRecoverTotal = 0.f;
	float RecoilHorRecoverTotal = 0.f;
	float RecoilVertRecoverLastTick = 0.f;
	float RecoilHorRecoverLastTick = 0.f;
	float CameraRecoilVertRecoverTotal = 0.f;
	float CameraRecoilHorRecoverTotal = 0.f;
	float CameraRecoilVertRecoverLastTick = 0.f;
	float CameraRecoilHorRecoverLastTick = 0.f;
	void PollRecoverRecoil(float DeltaSeconds);

	// 连续射击发数（连射时递增，停火后平滑衰减回 1）
	float RecoilShotIndex = 1.f;
	float RecoilShotDecayRate = 0.f;
	double LastRecoilFireTime = -100.0;
	void PollDecayShotCount(float DeltaSeconds);

public:
	float GetCurRecoilVert();
	float GetCurRecoilHor();
	FVector2D GetCurSpreadVector();

};
