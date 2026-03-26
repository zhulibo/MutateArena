#include "HighAlert.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/System/Tags/ProjectTags.h"
#include "MutateArena/Characters/BaseCharacter.h" 

void UHighAlert::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UHighAlert::NativeConstruct()
{
	Super::NativeConstruct();

	if (AlertImage)
	{
		AlertMID = AlertImage->GetDynamicMaterial();
	}
	if (AlertMID)
	{
		AlertMID->SetVectorParameterValue(FName("AlertColor"), C_YELLOW);
	}
}

void UHighAlert::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!AlertMID) return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	APawn* MyPawn = PC->GetPawn();
	if (!MyPawn) return;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MyPawn);
	
	// 检查玩家是否满足高危预警触发条件
	if (!ASC || !ASC->HasMatchingGameplayTag(TAG_STATE_DNA_SubconsciousAwareness))
	{
		TargetLeftAlert = 0.f;
		TargetRightAlert = 0.f;
		TargetBottomAlert = 0.f;
		TimeSinceLastTrace = TraceUpdateInterval;
	}
	else
	{
		TimeSinceLastTrace += InDeltaTime;
		if (TimeSinceLastTrace >= TraceUpdateInterval)
		{
			TimeSinceLastTrace = 0.0f;
			
			UWorld* World = GetWorld();

			if (ABaseGameState* BaseGameState = World ? World->GetGameState<ABaseGameState>() : nullptr)
			{
				FVector CameraLoc;
				FRotator CameraRot;
				PC->GetPlayerViewPoint(CameraLoc, CameraRot);

				FVector CameraForward = CameraRot.Vector();
				FVector CameraRight = FRotationMatrix(CameraRot).GetScaledAxis(EAxis::Y);

				float MaxLeft = 0.0f;
				float MaxRight = 0.0f;
				float MaxBottom = 0.0f;

				const float EnemyFOVThreshold = 0.707f;
				const float AlertRadiusSq = FMath::Square(AlertRadius);

				if (TargetTeam == ETeam::NoTeam)
				{
					if (ABasePlayerState* PS = PC->GetPlayerState<ABasePlayerState>())
					{
						OnTeamChange(PS->Team);
					}
				}
				if (TargetTeam == ETeam::NoTeam) return;
				
				TArray<ABasePlayerState*> PlayerStates;
				BaseGameState->GetPlayerStates(TargetTeam, PlayerStates);
				for (ABasePlayerState* PS : PlayerStates)
				{
					if (!PS) continue;

					ABaseCharacter* EnemyCharacter = Cast<ABaseCharacter>(PS->GetPawn());
					// 确保目标有效且存活
					if (!EnemyCharacter || EnemyCharacter->bIsDead || EnemyCharacter == MyPawn) continue; 
					
					FVector EnemyLoc = EnemyCharacter->GetActorLocation();
					FVector DirFromEnemyToPlayer = CameraLoc - EnemyLoc;
					float DistanceSq = DirFromEnemyToPlayer.SizeSquared();

					// 距离过滤
					if (DistanceSq > AlertRadiusSq) continue;

					DirFromEnemyToPlayer.Normalize();

					// 视锥体判断：敌人是否正在看向玩家
					FVector EnemyAimDir = EnemyCharacter->GetBaseAimRotation().Vector();
					float EnemyAimDot = FVector::DotProduct(EnemyAimDir, DirFromEnemyToPlayer);

					if (EnemyAimDot < EnemyFOVThreshold) continue;

					// 视线遮挡检测 (Line of Sight)
					FVector EnemyEyeLoc;
					FRotator EnemyEyeRot;
					
					EnemyCharacter->GetActorEyesViewPoint(EnemyEyeLoc, EnemyEyeRot);

					FHitResult HitResult;
					FCollisionQueryParams QueryParams;
					QueryParams.AddIgnoredActor(EnemyCharacter);
					QueryParams.AddIgnoredActor(MyPawn);

					FCollisionObjectQueryParams ObjectQueryParams;
					ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

					// 射线检测
					bool bHit = World->LineTraceSingleByObjectType(
						HitResult,
						EnemyEyeLoc, 
						CameraLoc, 
						ObjectQueryParams, 
						QueryParams
					);

					if (bHit) continue;

					// 计算提示的相对方位
					FVector DirToEnemy = -DirFromEnemyToPlayer; 

					// 将世界坐标系下的方向转换到相机的局部坐标系中
					FVector LocalDir = CameraRot.UnrotateVector(DirToEnemy);

					// 计算敌人相对于视角的精确角度 (-180 到 180 度)
					// LocalDir.X 是正前方，LocalDir.Y 是正右方
					float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(LocalDir.Y, LocalDir.X));

					// 强度根据距离衰减 (保持你原有的逻辑)
					float Intensity = 1.0f - FMath::Clamp(FMath::Sqrt(DistanceSq) / AlertRadius, 0.0f, 1.0f);

					// 划分严格的象限，确保单个敌人只触发一个方向的警报
					// 1. 正前方视野内不予提示 (-60度 到 60度)
					if (AngleDeg > -60.0f && AngleDeg < 60.0f) 
					{
						continue;
					}
					// 2. 右侧 (60度 到 135度)
					else if (AngleDeg >= 60.0f && AngleDeg <= 135.0f) 
					{
						MaxRight = FMath::Max(MaxRight, Intensity);
					}
					// 3. 左侧 (-135度 到 -60度)
					else if (AngleDeg >= -135.0f && AngleDeg <= -60.0f) 
					{
						MaxLeft = FMath::Max(MaxLeft, Intensity);
					}
					// 4. 后方 (剩下的就是绝对值 > 135度 的区域，即左后和右后)
					else 
					{
						MaxBottom = FMath::Max(MaxBottom, Intensity);
					}
				}
				
				// if (MyPawn->HasAuthority())
				// {
				// 	UE_LOG(LogTemp, Warning, TEXT("MaxLeft MaxRight MaxBottom %f %f %f"), MaxLeft, MaxRight, MaxBottom);
				// }
				
				// 将本轮计算出来的最大值作为插值目标
				TargetLeftAlert = MaxLeft;
				TargetRightAlert = MaxRight;
				TargetBottomAlert = MaxBottom;
			}
		}
	}

	// 【体验优化】：每帧使用 FMath::FInterpTo 平滑过渡材质参数
	CurrentLeftAlert = FMath::FInterpTo(CurrentLeftAlert, TargetLeftAlert, InDeltaTime, InterpSpeed);
	CurrentRightAlert = FMath::FInterpTo(CurrentRightAlert, TargetRightAlert, InDeltaTime, InterpSpeed);
	CurrentBottomAlert = FMath::FInterpTo(CurrentBottomAlert, TargetBottomAlert, InDeltaTime, InterpSpeed);

	AlertMID->SetScalarParameterValue(FName("LeftAlert"), CurrentLeftAlert);
	AlertMID->SetScalarParameterValue(FName("RightAlert"), CurrentRightAlert);
	AlertMID->SetScalarParameterValue(FName("BottomAlert"), CurrentBottomAlert);
}

void UHighAlert::OnTeamChange(ETeam Team)
{
	if (Team == ETeam::Team1)
	{
		TargetTeam = ETeam::Team2;
	}
	else if (Team == ETeam::Team2)
	{
		TargetTeam = ETeam::Team1;
	}
	else
	{
		TargetTeam = ETeam::NoTeam;
	}
}
