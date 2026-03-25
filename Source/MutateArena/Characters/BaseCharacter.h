#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "ModularCharacter.h"
#include "BaseCharacter.generated.h"

enum class ECommonInputType : uint8;

UCLASS()
class MUTATEARENA_API ABaseCharacter : public AModularCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter();

	UPROPERTY(VisibleAnywhere)
	class USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* Camera;
	UPROPERTY(VisibleAnywhere)
	USceneCaptureComponent2D* SceneCapture;
	UPROPERTY(VisibleAnywhere)
	class UWidgetComponent* OverheadWidget;
	UPROPERTY()
	class UOverheadWidget* OverheadWidgetClass;
	UPROPERTY(VisibleAnywhere)
	class UInteractorComponent* InteractorComp;
	UPROPERTY(VisibleAnywhere)
	class UAutoHostComponent* AutoHostComp;
	// AFK
	UPROPERTY(EditAnywhere)
	class UStateTreeComponent* StateTreeComp;
	UPROPERTY(EditAnywhere)
	class UAIPerceptionComponent* AIPerceptionComp;
	UPROPERTY(EditAnywhere)
	class UAIPerceptionStimuliSourceComponent* StimuliSourceComp;
	UPROPERTY(EditAnywhere)
	class UAISenseConfig_Sight* SightConfig;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void Destroyed() override;
	
	UPROPERTY()
	class UAssetSubsystem* AssetSubsystem;
	UPROPERTY()
	class UStorageSubsystem* StorageSubsystem;
	UPROPERTY()
	class ABaseMode* BaseMode;
	UPROPERTY()
	class ABaseGameState* BaseGameState;
	UPROPERTY()
	class ABasePlayerState* BasePlayerState;
	UPROPERTY()
	class ABaseController* BaseController;
	
	UFUNCTION()
	void OnInputMethodChanged(ECommonInputType TempInputType);
	UFUNCTION(Server, Unreliable)
	void ServerSetInputType(ECommonInputType TempInputType);
	
public:
	// 标准行走速度参考值，用于计算行走动画播放速度。在子类中会被赋值，赋值时需保证与角色ASC中的MaxWalkSpeed一级速度一致
	UPROPERTY()
	float DefaultMaxWalkSpeed = 600.f;

	bool bIsPlayerStateTeamReady = false;
protected:
	void PollInit_PlayerStateTeam();

	bool bIsControllerAndPSAndTeamReady = false;
	void PollInit_ControllerAndPSAndTeam();
	bool bIsLocallyControllerReady = false;
	virtual void OnLocallyControllerReady();
	
	UPROPERTY(Replicated)
	float ControllerPitch;
public:
	UPROPERTY()
	float AimPitch;
protected:
	void CalcAimPitch();
public:
	float MappingAimPitch(float TempAimPitch);

	// GAS
protected:
	UPROPERTY()
	class UMAAbilitySystemComponent* ASC;
	UPROPERTY()
	class UAttributeSetBase* AttributeSetBase;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UGameplayEffect> DefaultAttrEffect;
	void InitAbilityActorInfo();
	virtual void OnASCInit();
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAttributeSetBase* GetAttributeSetBase();
	float GetMaxHealth();
	float GetHealth();
	float GetDamageReceivedMul();
	float GetRepelReceivedMul();
	float GetCharacterLevel();
	float GetMaxWalkSpeed();
	float GetJumpZVelocity();

	UPROPERTY()
	FColor BloodColor;
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* BloodEffect;
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* BloodSmokeEffect;
protected:
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	// 基础运动
	virtual void MoveStarted(const struct FInputActionValue& Value) {}
	void Move(const FInputActionValue& Value);
	virtual void MoveCompleted(const FInputActionValue& Value) {}
	void LookMouse(const FInputActionValue& Value);
	void LookStick(const FInputActionValue& Value);
	void JumpButtonPressed(const FInputActionValue& Value);
	void CrouchButtonPressed(const FInputActionValue& Value);
	void CrouchButtonReleased(const FInputActionValue& Value);
	void CrouchControllerButtonPressed(const FInputActionValue& Value);

	// 血量
public:
	UPROPERTY(ReplicatedUsing = OnRep_bIsDead)
	bool bIsDead = false;
	UFUNCTION()
	virtual void OnRep_bIsDead();
	void SetHealth(float TempHealth);
protected:
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	virtual void OnHealthChanged(const FOnAttributeChangeData& Data);
	float HealthRateThreshold = .2f;
	float Desaturation = .7f;
	float Power = .7f;

	// 跌落
	virtual void Landed(const FHitResult& Hit) override;
	float CalcFallDamageRate();
	UPROPERTY(EditAnywhere)
	class UMetaSoundSource* OuchSound;
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayOuchSound(float DamageRate);
public:
	void PlayFootSound();
	void PlayFootLandSound();
protected:
	virtual void FellOutOfWorld(const UDamageType& DmgType) override;

	// 无线电
	UPROPERTY(EditAnywhere)
	TArray<UMetaSoundSource*> RadioSounds;
	// https://aistudio.google.com/generate-speech 提示语：我在制作一款射击游戏，我需要生成游戏中的无线电语音，类似于cs中的无线电
	// 文本：Storm the front / Good job / Affirmative / Enemy spotted / Keep front line team / Stick together team / Negative / Follow me
	// 嗓音：Solider 萨达奇比亚 Tank 土卫二 Ghost 带领
public:
	void SendRadio(int32 RadioIndex);
protected:
	UFUNCTION(Server, Unreliable)
	void ServerPlayRadioSound(int32 RadioIndex);
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayRadioSound(int32 RadioIndex);
	void LocalPlayRadioSound(int32 RadioIndex);

	UPROPERTY()
	UDecalComponent* SprayPaintDecal;
public:
	void SprayPaint(int32 RadioIndex);
	
protected:
	UPROPERTY()
	class UMaterialInstanceDynamic* MID_Flashbang;
	FTimerHandle TimerHandle_FlashbangEnd;
	// 记录当前闪光效果的绝对结束时间戳，用于防止较弱的闪光弹覆盖强闪光弹
	float FlashbangEndTime = 0.f; 
	void ClearFlashbangEffect();
public:
	void ApplyFlashbangEffect(float InRadius, float InMaxFlashTime, float InMaxCapTime, float InFlashTime, float InDistance, float InAngle);
	
};
