#include "OverheadWidget.h"
#include "CommonTextBlock.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerControllers/BaseController.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/PlayerStates/TeamType.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "CommonLazyImage.h"
#include "Components/VerticalBox.h"
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
	// 弱指针安全校验
	if (!BaseCharacter.IsValid()) return;

	if (!BasePlayerState.IsValid()) BasePlayerState = Cast<ABasePlayerState>(BaseCharacter->GetPlayerState());

	if (!LocalBasePlayerState.IsValid())
	{
		if (!LocalBaseController.IsValid()) LocalBaseController = Cast<ABaseController>(GetWorld()->GetFirstPlayerController());
		if (LocalBaseController.IsValid()) LocalBasePlayerState = Cast<ABasePlayerState>(LocalBaseController->PlayerState);
	}

	if (BasePlayerState.IsValid() && LocalBasePlayerState.IsValid())
	{
		if (BasePlayerState->Team != ETeam::NoTeam && LocalBasePlayerState->Team != ETeam::NoTeam)
		{
			bNeedUpdate = false;
			
			FColor TeamColor = BasePlayerState->Team == LocalBasePlayerState->Team ? C_BLUE : C_RED;
			
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
						TeamColor = C_YELLOW;
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
	if (!bIsAllowShow) return;
	
	if (!LocalBaseCharacter.IsValid())
	{
		if (!LocalBaseController.IsValid()) LocalBaseController = Cast<ABaseController>(GetWorld()->GetFirstPlayerController());
		if (LocalBaseController.IsValid()) LocalBaseCharacter = Cast<ABaseCharacter>(LocalBaseController->GetPawn());
	}

	if (BaseCharacter.IsValid() && LocalBaseCharacter.IsValid() && BaseCharacter != LocalBaseCharacter)
	{
		FVector Start = LocalBaseCharacter->Camera->GetComponentLocation();
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
		QueryParams.AddIgnoredActor(LocalBaseCharacter.Get());
		QueryParams.AddIgnoredActor(BaseCharacter.Get()); 
		if (BaseGameState == nullptr) BaseGameState = GetWorld()->GetGameState<ABaseGameState>();
		if (BaseGameState.Get())
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
		FVector CameraForward = LocalBaseCharacter->Camera->GetForwardVector();
		// 计算从相机指向敌人的方向向量，并归一化
		FVector DirToEnemy = (End - Start).GetSafeNormal();
		// 进行点乘运算
		float DotResult = FVector::DotProduct(CameraForward, DirToEnemy);
		// 准星容差角度
		float ConeAngleDegrees = 10.f;
		// 转换为弧度，并求其次余弦值
		float AimThreshold = FMath::Cos(FMath::DegreesToRadians(ConeAngleDegrees));
		// 敌人是否在准星范围内
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
		HealthBarMID->SetScalarParameterValue(FName("OldValue"), OldValue);
		HealthBarMID->SetScalarParameterValue(FName("NewValue"), NewValue);

		float PlaybackSpeed = FMath::Abs(NewValue - OldValue) * 3;

		if (OldValue > NewValue)
		{
			PlayAnimationTimeRange(HealthDec, 1 - OldValue, 1 - NewValue, 1,  EUMGSequencePlayMode::Forward, PlaybackSpeed);
		}
		else if (OldValue < NewValue)
		{
			PlayAnimationTimeRange(HealthInc, OldValue, NewValue, 1,  EUMGSequencePlayMode::Forward, PlaybackSpeed);
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
	PlayAnimationTimeRange(FadeIn, 0, 1, 1,  EUMGSequencePlayMode::Forward, Speed);
}
