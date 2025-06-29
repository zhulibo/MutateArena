#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

enum class ECommonInputType : uint8;

UCLASS()
class MUTATEARENA_API ABaseCharacter : public ACharacter, public IAbilitySystemInterface
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

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void Destroyed() override;
	
	UFUNCTION()
	void OnInputMethodChanged(ECommonInputType TempInputType);
	UFUNCTION(Server, Unreliable)
	void ServerSetInputType(ECommonInputType TempInputType);
	
public:
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
	class UMAAbilitySystemComponent* AbilitySystemComponent;
	UPROPERTY()
	class UAttributeSetBase* AttributeSetBase;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UGameplayEffect> DefaultAttrEffect;
	void InitAbilityActorInfo();
	virtual void OnAbilitySystemComponentInit();
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

public:
	UPROPERTY()
	FColor BloodColor;
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* BloodEffect;
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

	// 交互
	UPROPERTY()
	AActor* InteractTarget;
	void TraceInteractTarget(FHitResult& OutHit);
	void InteractStarted(const FInputActionValue& Value);
	void InteractOngoing(const FInputActionValue& Value);
	void InteractTriggered(const FInputActionValue& Value);
	UFUNCTION(Server, Reliable)
	void ServerInteractTriggered(AActor* TempInteractTarget);
	void InteractCompleted(const FInputActionValue& Value);
	void InteractCanceled(const FInputActionValue& Value);

	void ScoreboardButtonPressed(const FInputActionValue& Value);
	void ScoreboardButtonReleased(const FInputActionValue& Value);
	
	void PauseMenuButtonPressed(const FInputActionValue& Value);

	void RadialMenuButtonPressed(const FInputActionValue& Value);
	void RadialMenuButtonReleased(const FInputActionValue& Value);
	void RadialMenuChangeButtonPressed(const FInputActionValue& Value);
	void RadialMenuSelect(const FInputActionValue& Value);

	void TextChat(const FInputActionValue& Value);

	// 血量
public:
	bool bIsDead = false;
	void SetHealth(float TempHealth);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetHealth(float TempHealth, AController* AttackerController);
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
	void PlayFootLandSound();
protected:
	virtual void FellOutOfWorld(const UDamageType& DmgType) override;

	// 无线电
	UPROPERTY(EditAnywhere)
	TArray<UMetaSoundSource*> RadioSounds;
	// 样式：射击游戏中的无线电语音，类似于cs中的无线电
	// 文本：Storm the front / Good job / Affirmative / Enemy spotted / Keep front line team / Stick together team / Negative / Follow me
	// 嗓音：Solider 萨达奇比亚 Tank 土卫二 Ghost 带领
public:
	void SendRadio(int32 RadioIndex);
protected:
	UFUNCTION(Server, Reliable)
	void ServerPlayRadioSound(int32 RadioIndex);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayRadioSound(int32 RadioIndex);
	void LocalPlayRadioSound(int32 RadioIndex);

	UPROPERTY()
	UDecalComponent* SprayPaintDecal;
public:
	void SprayPaint(int32 RadioIndex);
	
};
