#include "OverheadWidget.h"
#include "CommonTextBlock.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/WidgetComponent.h"
#include "CommonLazyImage.h"
#include "TimerManager.h"
#include "Components/VerticalBox.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/Utils/LibraryCommon.h"

void UOverheadWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
	{
		UISubsystem->OnOverheadWidgetNeedUpdate.AddUObject(this, &ThisClass::MakeDirty);
	}
}

void UOverheadWidget::NativeConstruct()
{
	Super::NativeConstruct();

	HealthBarMID = HealthBar->GetDynamicMaterial();
	HealthBarLineMID = HealthBarLine->GetDynamicMaterial();

	// 定时判断是否显示OverheadWidget。加入随机初始延迟，防止全场玩家在同一帧进行射线检测造成卡顿
	GetWorld()->GetTimerManager().SetTimer(TraceTimerHandle, this, &ThisClass::TraceOverheadWidget, .1f, true, FMath::RandRange(0.f, .1f));
}

void UOverheadWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bNeedUpdate)
	{
		InitOverheadWidget();
	}
}

void UOverheadWidget::NativeDestruct()
{
	GetWorld()->GetTimerManager().ClearTimer(TraceTimerHandle);

	Super::NativeDestruct();
}

void UOverheadWidget::MakeDirty()
{
	bNeedUpdate = true;
}

void UOverheadWidget::InitOverheadWidget()
{
	if (!BaseCharacter.IsValid()) return;

	// 实时从 BaseCharacter 获取最新的 PlayerState，防止旧缓存失效
	BasePlayerState = BaseCharacter->GetPlayerState<ABasePlayerState>();

	if (!LocalBaseController.IsValid())
	{
		LocalBaseController = Cast<ABaseController>(GetWorld()->GetFirstPlayerController());
	}
	if (!LocalBaseController.IsValid()) return;

	// 优先获取本地玩家自身的 PlayerState
	LocalBasePlayerState = LocalBaseController->GetPlayerState<ABasePlayerState>();

	// 如果本地玩家没有队伍（例如纯观察者），尝试获取当前第一人称被观战角色的 PlayerState 作为阵营参照
	ABasePlayerState* ReferencePS = LocalBasePlayerState.Get();
	if (!ReferencePS || ReferencePS->Team == ETeam::NoTeam)
	{
		if (ABaseCharacter* ViewTargetChar = Cast<ABaseCharacter>(LocalBaseController->GetViewTarget()))
		{
			ReferencePS = ViewTargetChar->GetPlayerState<ABasePlayerState>();
		}
	}

	if (BasePlayerState.IsValid() && ReferencePS)
	{
		if (BasePlayerState->Team != ETeam::NoTeam && ReferencePS->Team != ETeam::NoTeam)
		{
			bNeedUpdate = false;

			FColor TeamColor = BasePlayerState->Team == ReferencePS->Team ? Colors::Blue : Colors::Red;

			// 设置名字 (字符串混淆计算开销大，只在状态更新时执行)
			FString ObfuscatedName = ULibraryCommon::ObfuscateName(BasePlayerState->GetPlayerName(), this);
			PlayerName->SetText(FText::FromString(ObfuscatedName));
			PlayerName->SetColorAndOpacity(TeamColor);

			// 设置血条颜色
			if (HealthBarMID)
			{
				if (AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(BaseCharacter.Get()))
				{
					if (HumanCharacter->bIsImmune)
					{
						TeamColor = Colors::Yellow;
					}
				}
				HealthBarMID->SetVectorParameterValue(TEXT("TeamColor"), TeamColor);
			}

			// 设置血条刻度
			if (HealthBarLineMID)
			{
				HealthBarLineMID->SetScalarParameterValue(FName("LineNum"), GetHealthBarLineNum());
			}
		}
	}
}

// 判断是否显示OverheadWidget
void UOverheadWidget::TraceOverheadWidget()
{
	if (!bIsAllowShow || !BaseCharacter.IsValid()) return;

	// 如果该角色当前正处于本地玩家的第一人称视角（自己操控或第一人称观战），直接隐藏并跳过检测
	if (BaseCharacter->bIsCurrentViewTarget)
	{
		SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	if (!LocalBaseController.IsValid())
	{
		LocalBaseController = Cast<ABaseController>(GetWorld()->GetFirstPlayerController());
	}
	if (!LocalBaseController.IsValid() || !LocalBaseController->PlayerCameraManager) return;

	AActor* CurrentViewTarget = LocalBaseController->GetViewTarget();
	if (!CurrentViewTarget || CurrentViewTarget == BaseCharacter.Get())
	{
		SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	// 直接从 PlayerCameraManager 获取相机位置与朝向，同时完美兼容：
	// 1. 本地第一人称操控 (ABaseCharacter)
	// 2. 第一人称观战队友 (ABaseCharacter)
	// 3. 自由飞行观战 (ASpectatorPawn)
	FVector Start = LocalBaseController->PlayerCameraManager->GetCameraLocation();
	FVector End = BaseCharacter->Camera->GetComponentLocation();

	// 距离检测
	if (FVector::DistSquared(Start, End) > TraceDistance * TraceDistance)
	{
		SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	// 视野遮挡检测
	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TraceOverheadWidget), false);
	QueryParams.AddIgnoredActor(CurrentViewTarget);
	QueryParams.AddIgnoredActor(BaseCharacter.Get());
	if (APawn* LocalPawn = LocalBaseController->GetPawn())
	{
		QueryParams.AddIgnoredActor(LocalPawn);
	}
	if (!BaseGameState.IsValid())
	{
		BaseGameState = GetWorld()->GetGameState<ABaseGameState>();
	}
	if (BaseGameState.IsValid())
	{
		QueryParams.AddIgnoredActors(BaseGameState->AllEquipments);
	}

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams);
	if (bHit)
	{
		SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	// 控制血条是否显示
	FVector CameraForward = LocalBaseController->PlayerCameraManager->GetCameraRotation().Vector();
	// 计算从相机指向目标的方向向量，并归一化
	FVector DirToTarget = (End - Start).GetSafeNormal();
	// 进行点乘运算
	float DotResult = FVector::DotProduct(CameraForward, DirToTarget);
	// 准星容差角度
	float ConeAngleDegrees = 10.f;
	// 转换为弧度，并求余弦值
	float AimThreshold = FMath::Cos(FMath::DegreesToRadians(ConeAngleDegrees));
	// 目标是否在准星范围内
	if (DotResult > AimThreshold)
	{
		LastAimTime = GetWorld()->GetTimeSeconds();
		CT->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else if (GetWorld()->GetTimeSeconds() - LastAimTime > HealthBarLingerTime)
	{
		CT->SetVisibility(ESlateVisibility::Hidden);
	}
}

int32 UOverheadWidget::GetHealthBarLineNum()
{
	if (!BaseCharacter.IsValid()) return 1;

	float MaxHealth = BaseCharacter->GetMaxHealth();
	float UnitHealth = MaxHealth > 1000 ? 1000 : 100;
	return FMath::FloorToInt(MaxHealth / UnitHealth);
}

void UOverheadWidget::OnMaxHealthChange(float MaxHealth)
{
	if (HealthBarLineMID)
	{
		HealthBarLineMID->SetScalarParameterValue(FName("LineNum"), GetHealthBarLineNum());
	}
}

void UOverheadWidget::OnHealthChange(float OldHealth, float NewHealth)
{
	if (!BaseCharacter.IsValid()) return;
	float MaxHealth = BaseCharacter->GetMaxHealth();

	float OldValue = FMath::Clamp(OldHealth / MaxHealth, 0.f, 1.f);
	float NewValue = FMath::Clamp(NewHealth / MaxHealth, 0.f, 1.f);

	if (HealthBarMID)
	{
		if (bIsFirstHealthSync)
		{
			bIsFirstHealthSync = false;
			HealthBarMID->SetScalarParameterValue(FName("OldValue"), NewValue);
			HealthBarMID->SetScalarParameterValue(FName("NewValue"), NewValue);
			return;
		}

		HealthBarMID->SetScalarParameterValue(FName("OldValue"), OldValue);
		HealthBarMID->SetScalarParameterValue(FName("NewValue"), NewValue);

		float PlaybackSpeed = FMath::Abs(NewValue - OldValue) * 3;
		if (OldValue > NewValue)
		{
			PlayAnimationTimeRange(HealthDec, 1 - OldValue, 1 - NewValue, 1, EUMGSequencePlayMode::Forward, PlaybackSpeed);
		}
		else if (OldValue < NewValue)
		{
			PlayAnimationTimeRange(HealthInc, OldValue, NewValue, 1, EUMGSequencePlayMode::Forward, PlaybackSpeed);
		}
	}
}

void UOverheadWidget::ShowOverheadWidget(bool bIsShow)
{
	bIsAllowShow = bIsShow;
	SetVisibility(bIsAllowShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}

void UOverheadWidget::PlayFlashbangEffect(float Speed)
{
	PlayAnimationTimeRange(FadeIn, 0, 1, 1, EUMGSequencePlayMode::Forward, Speed);
}
