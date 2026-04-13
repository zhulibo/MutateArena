#include "Minimap.h"
#include "MinimapIcon.h"
#include "MutateArena/System/UISubsystem.h"
#include "MutateArena/GameStates/BaseGameState.h"
#include "MutateArena/PlayerStates/BasePlayerState.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/Characters/BaseCharacter.h"
#include "MutateArena/Characters/HumanCharacter.h"

void UMinimap::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
    {
       UISubsystem->OnTeamPlayerStatesUpdated.AddUObject(this, &UMinimap::UpdateTeammateIcons);
       UISubsystem->OnLocalTeamChange.AddWeakLambda(this, [this](ETeam Team)
       {
          UpdateTeammateIcons();
       });
    }
}

void UMinimap::NativeConstruct()
{
    Super::NativeConstruct();
}

void UMinimap::NativeDestruct()
{
    if (UUISubsystem* UISubsystem = ULocalPlayer::GetSubsystem<UUISubsystem>(GetOwningLocalPlayer()))
    {
       UISubsystem->OnTeamPlayerStatesUpdated.RemoveAll(this);
       UISubsystem->OnLocalTeamChange.RemoveAll(this);
    }

    Super::NativeDestruct();
}

void UMinimap::UpdateTeammateIcons()
{
    ABaseGameState* BaseGameState = GetWorld()->GetGameState<ABaseGameState>();
    ABasePlayerState* LocalPS = GetOwningPlayerState<ABasePlayerState>();
    if (!BaseGameState || !LocalPS || !IconWidgetClass || !IconContainer) return;

    // 初始化本地玩家图标
    if (!LocalPlayerIcon)
    {
       LocalPlayerIcon = CreateWidget<UMinimapIcon>(GetWorld(), IconWidgetClass);
       if (LocalPlayerIcon)
       {
          if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(IconContainer->AddChild(LocalPlayerIcon)))
          {
             CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
             CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
             CanvasSlot->SetSize(FVector2D(40.f, 40.f));
             CanvasSlot->SetPosition(FVector2D(0.f, 0.f));
          }
          LocalPlayerIcon->UpdateIconAppearance(true, C_BLUE);
          LocalPlayerIcon->SetRenderTransformAngle(0.f);
          LocalPlayerIcon->SetRenderOpacity(1.0f);
       }
    }

    // 获取并更新队友列表
    TArray<ABasePlayerState*> TeammateStates;
    BaseGameState->GetPlayerStates(LocalPS->Team, TeammateStates);
    TSet<ABasePlayerState*> CurrentTeammates;

    for (ABasePlayerState* PS : TeammateStates)
    {
       if (!PS || PS == LocalPS) continue;

       CurrentTeammates.Add(PS);

       // 添加新队友的图标
       if (!TeammateIconMap.Contains(PS))
       {
          if (UMinimapIcon* NewIcon = CreateWidget<UMinimapIcon>(GetWorld(), IconWidgetClass))
          {
             if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(IconContainer->AddChild(NewIcon)))
             {
                CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
                CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
                CanvasSlot->SetSize(FVector2D(40.f, 40.f));
                CanvasSlot->SetPosition(FVector2D(0.f, 0.f));
             }
             TeammateIconMap.Add(PS, NewIcon);
          }
       }
    }

    // 移除已不在队伍中的玩家图标
    for (auto It = TeammateIconMap.CreateIterator(); It; ++It)
    {
       if (!CurrentTeammates.Contains(It.Key()))
       {
          if (UMinimapIcon* IconToRemove = It.Value())
          {
             IconToRemove->RemoveFromParent();
          }
          It.RemoveCurrent();
       }
    }
}

void UMinimap::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    APlayerController* PC = GetOwningPlayer();
    ABasePlayerState* LocalPS = GetOwningPlayerState<ABasePlayerState>();
    if (!PC || !LocalPS) return;

    APawn* LocalPawn = PC->GetPawn();

    // 获取本地玩家状态
    bool bIsLocalAlive = false;
    bool bIsLocalImmune = false; 

    if (ABaseCharacter* LocalChar = Cast<ABaseCharacter>(LocalPawn))
    {
       bIsLocalAlive = !LocalChar->bIsDead;
       if (AHumanCharacter* HumanChar = Cast<AHumanCharacter>(LocalChar))
       {
           bIsLocalImmune = HumanChar->bIsImmune;
       }
    }

    // 确定小地图中心坐标和旋转
    FVector SelfLoc = FVector::ZeroVector;
    FRotator SelfRot = FRotator::ZeroRotator;

    if (bIsLocalAlive && LocalPawn)
    {
       // 存活跟随玩家
       SelfLoc = LocalPawn->GetActorLocation();
       SelfRot = LocalPawn->GetActorRotation();
    }
    else if (PC->PlayerCameraManager)
    {
       // 死亡跟随摄像机观战
       SelfLoc = PC->PlayerCameraManager->GetCameraLocation();
       SelfRot = PC->PlayerCameraManager->GetCameraRotation();
    }
    else
    {
       return; // 均无效则跳过此帧
    }

    // 更新本地玩家图标UI
    if (LocalPlayerIcon)
    {
       if (bIsLocalAlive)
       {
          LocalPlayerIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
          const FLinearColor LocalColor = bIsLocalImmune ? C_YELLOW : C_BLUE;
          LocalPlayerIcon->UpdateIconAppearance(true, LocalColor);
       }
       else
       {
          LocalPlayerIcon->SetVisibility(ESlateVisibility::Hidden);
       }
    }

    // 小地图数学计算常量
    constexpr float OrthoWidth = GameConstants::MinimapOrthoWidth;
    constexpr float WorldHalfSize = OrthoWidth * 0.5f;
    constexpr float UIMapHalfSize = 128.0f; // 256.f / 2.f 必须为UI尺寸的一半
    constexpr float MapScale = UIMapHalfSize / WorldHalfSize;
    constexpr float MaxDistanceSq = FMath::Square(WorldHalfSize * 1.415f); // 预计算最大剔除距离的平方

    // 遍历更新队友图标
    for (auto It = TeammateIconMap.CreateIterator(); It; ++It)
    {
       ABasePlayerState* PS = It.Key();
       UMinimapIcon* Icon = It.Value();
       if (!PS || !Icon) continue;

       APawn* TeammatePawn = PS->GetPawn();
       
       // 队友状态检测
       bool bIsTeammateAlive = false;
       bool bIsTeammateImmune = false;
       
       if (ABaseCharacter* TeammateChar = Cast<ABaseCharacter>(TeammatePawn))
       {
          bIsTeammateAlive = !TeammateChar->bIsDead;
          if (AHumanCharacter* HumanTeammate = Cast<AHumanCharacter>(TeammateChar))
          {
              bIsTeammateImmune = HumanTeammate->bIsImmune;
          }
       }

       if (!TeammatePawn || !bIsTeammateAlive)
       {
          Icon->SetVisibility(ESlateVisibility::Hidden);
          continue;
       }

       // 距离与相对位置计算
       FVector RelativeLoc = TeammatePawn->GetActorLocation() - SelfLoc;
       const bool bSameFloor = FMath::Abs(RelativeLoc.Z) <= 250.0f;
       RelativeLoc.Z = 0.0f; // 抹平Z轴计算2D距离

       // 先判断是否超出范围，超出直接隐藏，免去后续UI更新逻辑
       if (RelativeLoc.SizeSquared2D() > MaxDistanceSq)
       {
          Icon->SetVisibility(ESlateVisibility::Hidden);
          continue;
       }

       Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

       // 更新颜色与外观
       const FLinearColor TargetColor = bIsTeammateImmune ? C_YELLOW : ((PS->Team == LocalPS->Team) ? C_BLUE : C_RED);
       Icon->UpdateIconAppearance(bSameFloor, TargetColor);

       // 计算UI空间坐标与旋转映射
       const FVector LocalRelativeLoc = SelfRot.UnrotateVector(RelativeLoc);
       FVector2D UIPos(LocalRelativeLoc.Y * MapScale, -LocalRelativeLoc.X * MapScale);

       // 限制在小地图边框内
       const float MaxAbsPos = FMath::Max(FMath::Abs(UIPos.X), FMath::Abs(UIPos.Y));
       if (MaxAbsPos > UIMapHalfSize)
       {
          UIPos *= (UIMapHalfSize / MaxAbsPos);
          Icon->SetRenderOpacity(0.5f);
       }
       else
       {
          Icon->SetRenderOpacity(1.0f);
       }

       // 应用坐标和旋转
       Icon->SetRenderTranslation(UIPos);
       Icon->SetRenderTransformAngle(TeammatePawn->GetActorRotation().Yaw - SelfRot.Yaw);
    }
}
