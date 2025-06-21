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
#include "Kismet/GameplayStatics.h"

void UOverheadWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UE_LOG(LogTemp, Warning, TEXT("NativeOnInitialized"));
	
	LocalBaseController = Cast<ABaseController>(GetWorld()->GetFirstPlayerController());
	if (LocalBaseController)
	{
		LocalBaseController->OnMatchEnd.AddUObject(this, &ThisClass::OnMatchEnd);
	}
}

void UOverheadWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BaseCharacter->HasAuthority())
	{
		if (BaseCharacter->IsLocallyControlled())
		{
			UE_LOG(LogTemp, Warning, TEXT("NativeConstruct 1"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("NativeConstruct 2"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NativeConstruct 3"));
	}
	
	// 定时判断是否显示OverheadWidget
	GetWorld()->GetTimerManager().SetTimer(TraceTimerHandle, this, &ThisClass::TraceOverheadWidget, .2f, true, .1f);

	SetPlayerName();
	
	if (BaseCharacter)
	{
		if (!BaseCharacter->IsLocallyControlled())
		{
			InitOverheadWidget();
		}
		// 本地玩家队伍改变时，初始化本机所有玩家的OverheadWidget
		else
		{
			TArray<AActor*> AllPlayers;
			UGameplayStatics::GetAllActorsWithTag(GetWorld(), TAG_CHARACTER_BASE, AllPlayers);
			for (AActor* Player : AllPlayers)
			{
				if (ABaseCharacter* PlayerCharacter = Cast<ABaseCharacter>(Player))
				{
					if (UWidgetComponent* OverheadWidget = PlayerCharacter->OverheadWidget)
					{
						if (UOverheadWidget* OverheadWidgetClass = Cast<UOverheadWidget>(OverheadWidget->GetUserWidgetObject()))
						{
							OverheadWidgetClass->InitOverheadWidget();
						}
					}
				}
			}
		}
	}
}

void UOverheadWidget::NativeDestruct()
{
	GetWorld()->GetTimerManager().ClearTimer(TraceTimerHandle);

	Super::NativeDestruct();
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
		if (FVector::Dist(Start, End) > 2000.f)
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
		if (HitResult.bBlockingHit)
		{
			SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void UOverheadWidget::OnMatchEnd()
{
	bCanUseSetTimerForNextTick = false;
}

// 设置用户名
void UOverheadWidget::SetPlayerName()
{
	if (!GetWorld()) return;
	
	if (BaseCharacter)
	{
		if (BasePlayerState == nullptr) BasePlayerState = Cast<ABasePlayerState>(BaseCharacter->GetPlayerState());
		if (BasePlayerState)
		{
			PlayerName->SetText(FText::FromString(ULibraryCommon::ObfuscatePlayerName(BasePlayerState->GetPlayerName(), this)));
			return;
		}
	}

	if (!bCanUseSetTimerForNextTick) return;
	GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
		SetPlayerName();
	});
}

void UOverheadWidget::InitOverheadWidget()
{
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

				// 设置名字颜色
				PlayerName->SetColorAndOpacity(TeamColor);

				// 设置血条颜色
				if (UMaterialInstanceDynamic* MID = HealthBar->GetDynamicMaterial())
				{
					MID->SetVectorParameterValue(TEXT("TeamColor"), TeamColor);
						
					if (AHumanCharacter* HumanCharacter = Cast<AHumanCharacter>(BaseCharacter))
					{
						if (HumanCharacter->bIsImmune)
						{
							MID->SetVectorParameterValue(TEXT("TeamColor"), C_YELLOW);
						}
					}
				}

				// 设置血条刻度
				if (UMaterialInstanceDynamic* MID = HealthBarLine->GetDynamicMaterial())
				{
					MID->SetScalarParameterValue(TEXT("LineNum"), GetHealthBarLineNum());
				}

				return;
			}
			else
			{
				// 游戏结束了关卡才加载完成GetTeam() == ETeam::NoTeam，中断循环，不然会crash。
				if (BaseGameState == nullptr) BaseGameState = GetWorld()->GetGameState<ABaseGameState>();
				if (BaseGameState)
				{
					if (BaseGameState->GetMatchState() == FName(TEXT("WaitingPostMatch")) || BaseGameState->GetMatchState() == FName(TEXT("LeavingMap")))
					{
						return;
					}
				}
				else
				{
					return;
				}
			}
		}
	}

	if (!bCanUseSetTimerForNextTick) return;
	GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
		InitOverheadWidget();
	});
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
		MID->SetScalarParameterValue(TEXT("LineNum"), GetHealthBarLineNum());
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
		MID->SetScalarParameterValue(TEXT("OldValue"), OldValue);
		MID->SetScalarParameterValue(TEXT("NewValue"), NewValue);

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
