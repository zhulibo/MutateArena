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
	ABaseCharacter(const FObjectInitializer& ObjectInitializer);
	
	UPROPERTY()
	class UMAMovementComponent* MovementComp;
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
	
	UPROPERTY(VisibleAnywhere)
	class USpringArmComponent* MinimapBoom;
	UPROPERTY(VisibleAnywhere)
	USceneCaptureComponent2D* MinimapCapture;
	
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void Destroyed() override;
	
protected:
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
	float DefaultMaxWalkSpeed = 500.f;

	bool bIsPlayerStateTeamReady = false;
protected:
	void PollInit_PlayerStateTeam();

	bool bIsControllerAndPSAndTeamReady = false;
	void PollInit_ControllerAndPSAndTeam();
	bool bIsLocallyControllerReady = false;
	virtual void OnLocallyControllerReady();
	FTimerHandle MinimapUpdateTimer;
	void UpdateMinimapCapture();
	
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
	float GetBodyResistance();

	UPROPERTY()
	FColor BloodColor;
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* BloodEffect;
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* BloodSmokeEffect;
protected:
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
	void GetFootstepAudioSettings(float& OutVolumeMultiplier, class USoundAttenuation*& OutAttenuation);
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
	UFUNCTION(Server, Unreliable)
	void ServerSprayPaint(int32 RadioIndex, FVector_NetQuantize ImpactPoint, FVector_NetQuantizeNormal ImpactNormal, UPrimitiveComponent* HitComp, FName BoneName);
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSprayPaint(int32 RadioIndex, FVector_NetQuantize ImpactPoint, FVector_NetQuantizeNormal ImpactNormal, UPrimitiveComponent* HitComp, FName BoneName);

	UPROPERTY()
	UMaterialInstanceDynamic* MID_Hurt;
	UPROPERTY()
	float CurrentBloodIntensity = 0.f;
	UPROPERTY()
	float BloodRecoveryRate = 0.2f;
	UPROPERTY()
	float BloodRecoveryDelayTimer = 0.f; 
	void ApplyHurtEffect(const FOnAttributeChangeData& Data);
	void UpdateHurtEffect(float DeltaSeconds);

	UPROPERTY()
	UMaterialInstanceDynamic* MID_Flashbang;
	FTimerHandle TimerHandle_FlashbangEnd;
	// 记录当前闪光效果的绝对结束时间戳，用于防止较弱的闪光弹覆盖强闪光弹
	float FlashbangEndTime = 0.f; 
	void ClearFlashbangEffect();
public:
	void ApplyFlashbangEffect(float InRadius, float InMaxFlashTime, float InMaxCapTime, float InFlashTime, float InDistance, float InAngle);

	UFUNCTION()
	void OnLadderBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnLadderEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	UPROPERTY()
	AActor* CurrentLadder = nullptr;
	float LadderGrabCooldown = 0.0f;
	void ExitLadderAndJump();
	UFUNCTION(Server, Reliable)
	void Server_JumpOffLadder();
	
};
