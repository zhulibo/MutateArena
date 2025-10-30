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
#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Utils/LibraryCommon.h"

void UOverheadWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UOverheadWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 定时判断是否显示OverheadWidget
	GetWorld()->GetTimerManager().SetTimer(TraceTimerHandle, this, &ThisClass::TraceOverheadWidget, .2f, true, .1f);
}

void UOverheadWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 获取Team不好处理，直接循环调用InitOverheadWidget
	// ESlateVisibility::Visible时NativeTick才会执行，所有只有近处的玩家才会循环调用InitOverheadWidget
	InitOverheadWidget();
}

void UOverheadWidget::NativeDestruct()
{
	GetWorld()->GetTimerManager().ClearTimer(TraceTimerHandle);

	Super::NativeDestruct();
}

void UOverheadWidget::InitOverheadWidget()
{
	// double Time1 = FPlatformTime::Seconds();
	
	if (BaseCharacter)
	{
		if (BasePlayerState == nullptr) BasePlayerState = Cast<ABasePlayerState>(BaseCharacter->GetPlayerState());

		if (LocalBasePlayerState == nullptr)
		{
			if (LocalBaseController == nullptr) LocalBaseController = Cast<ABaseController>(GetWorld()->GetFirstPlayerController());
			if (LocalBaseController) LocalBasePlayerState = Cast<ABasePlayerState>(LocalBaseController->PlayerState);
		}

		if (BasePlayerState && LocalBasePlayerState)
		{
			// UE_LOG(LogTemp, Warning, TEXT("SetColorAndOpacity Base GetTeam %d Local GetTeam %d"), BasePlayerState->GetTeam(), LocalBasePlayerState->GetTeam());
			if (BasePlayerState->Team != ETeam::NoTeam && LocalBasePlayerState->Team != ETeam::NoTeam)
			{
				FColor TeamColor = BasePlayerState->Team == LocalBasePlayerState->Team ? C_BLUE : C_RED;
				if (AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(BaseCharacter))
				{
					if (HumanCharacter->bIsImmune)
					{
						TeamColor = C_YELLOW;
					}
				}
				
				// 设置名字
				PlayerName->SetText(FText::FromString(ULibraryCommon::ObfuscateName(BasePlayerState->GetPlayerName(), this)));
				PlayerName->SetColorAndOpacity(TeamColor);

				// 设置血条颜色
				if (UMaterialInstanceDynamic* MID = HealthBar->GetDynamicMaterial())
				{
					MID->SetVectorParameterValue(TEXT("TeamColor"), TeamColor);
				}

				// 设置血条刻度
				if (UMaterialInstanceDynamic* MID = HealthBarLine->GetDynamicMaterial())
				{
					MID->SetScalarParameterValue(FName("LineNum"), GetHealthBarLineNum());
				}
			}
		}
	}

	// double Time2 = FPlatformTime::Seconds();
	// UE_LOG(LogTemp, Warning, TEXT("InitOverheadWidget %f"), Time2 - Time1);
}

// 判断是否显示OverheadWidget
void UOverheadWidget::TraceOverheadWidget()
{
	if (!bIsAllowShow) return;
	
	if (LocalBaseCharacter == nullptr)
	{
		if (LocalBaseController == nullptr) LocalBaseController = Cast<ABaseController>(GetWorld()->GetFirstPlayerController());
		if (LocalBaseController) LocalBaseCharacter = Cast<ABaseCharacter>(LocalBaseController->GetPawn());
	}

	if (BaseCharacter && LocalBaseCharacter && BaseCharacter != LocalBaseCharacter)
	{
		FHitResult HitResult;
		FVector Start = LocalBaseCharacter->Camera->GetComponentLocation();
		FVector End = BaseCharacter->Camera->GetComponentLocation();

		// 距离过远不显示PlayerName
		if (FVector::Dist(Start, End) > 2500.f)
		{
			SetVisibility(ESlateVisibility::Hidden);
			return;
		}

		// 射线检测玩家是否被阻挡
		FCollisionQueryParams QueryParams;
		TArray<AActor*> AllPlayers;
		if (BaseGameState == nullptr) BaseGameState = GetWorld()->GetGameState<ABaseGameState>();
		if (BaseGameState)
		{
			QueryParams.AddIgnoredActors(BaseGameState->AllEquipments);

			if (BaseGameState)
			{
				TArray<ABasePlayerState*> PlayerStates = BaseGameState->GetPlayerStates({});
				for (int32 i = 0; i < PlayerStates.Num(); ++i)
				{
					if (PlayerStates[i])
					{
						AllPlayers.AddUnique(PlayerStates[i]->GetPawn());
					}
				}
			}
		}
		QueryParams.AddIgnoredActors(AllPlayers);
		
		GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, QueryParams);
		
		SetVisibility(HitResult.bBlockingHit ? ESlateVisibility::Hidden :ESlateVisibility::Visible);
	}
}

int32 UOverheadWidget::GetHealthBarLineNum()
{
	float MaxHealth = BaseCharacter->GetMaxHealth();
	// 每格血量
	float UnitHealth = MaxHealth > 1000 ? 1000 : 100;

	return FMath::FloorToInt(MaxHealth / UnitHealth);
}

void UOverheadWidget::OnMaxHealthChange(float MaxHealth)
{
	if (UMaterialInstanceDynamic* MID = HealthBarLine->GetDynamicMaterial())
	{
		MID->SetScalarParameterValue(FName("LineNum"), GetHealthBarLineNum());
	}
}

void UOverheadWidget::OnHealthChange(float OldHealth, float NewHealth)
{
	if (BaseCharacter == nullptr) return;
	float MaxHealth = BaseCharacter->GetMaxHealth();

	// UE_LOG(LogTemp, Warning, TEXT("OldHealth %f NewHealth %f MaxHealth %f"), OldHealth, NewHealth, MaxHealth);

	float OldValue = FMath::Clamp(OldHealth / MaxHealth, 0.f, 1.f);
	float NewValue = FMath::Clamp(NewHealth / MaxHealth, 0.f, 1.f);

	if (UMaterialInstanceDynamic* MID = HealthBar->GetDynamicMaterial())
	{
		MID->SetScalarParameterValue(FName("OldValue"), OldValue);
		MID->SetScalarParameterValue(FName("NewValue"), NewValue);

		float PlaybackSpeed = FMath::Abs(NewValue - OldValue) * 3;

		// UE_LOG(LogTemp, Warning, TEXT("OldValue %f NewValue %f PlaybackSpeed %f"), OldValue, NewValue, PlaybackSpeed);

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

	SetVisibility(bIsAllowShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}

void UOverheadWidget::PlayFlashbangEffect(float Speed)
{
	PlayAnimationTimeRange(FadeIn, 0, 1, 1,  EUMGSequencePlayMode::Forward, Speed);
}
