#include "CorpseController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "MutateArena/Characters/HumanCharacter.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

ACorpseController::ACorpseController()
{
	bWantsPlayerState = false; // 小兵不需要 PlayerState，节省服务器开销

	// 1. 创建并绑定感知组件
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*AIPerceptionComp);

	// 2. 配置视觉
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 2000.0f;
	SightConfig->LoseSightRadius = 2200.0f;
	SightConfig->PeripheralVisionAngleDegrees = 90.0f;
	SightConfig->SetMaxAge(3.0f);

	// 配置阵营检测（默认开启所有，可配合 IGenericTeamAgentInterface 优化）
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	AIPerceptionComp->ConfigureSense(*SightConfig);
	AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

	// 3. 绑定感知更新回调
	AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &ACorpseController::OnTargetDetected);
}

void ACorpseController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 服务器端生成小兵并附身后，自动运行行为树
	if (HasAuthority() && BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void ACorpseController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
	// 1. 尝试将感知到的 Actor 转换为人类角色
	AHumanCharacter* HumanTarget = Cast<AHumanCharacter>(Actor);

	// 如果转换失败（说明感知到的可能是墙壁、其他突变体小兵等），直接忽略
	if (!HumanTarget)
	{
		return;
	}

	// 2. 将人类目标写入自动生成的黑板中
	if (UBlackboardComponent* BlackboardComp = GetBlackboardComponent())
	{
		const FName TargetKeyName = FName("TargetActor");

		if (Stimulus.WasSuccessfullySensed())
		{
			// 看见人类：记录为目标
			BlackboardComp->SetValueAsObject(TargetKeyName, HumanTarget);
		}
		else
		{
			// 丢失视野：防坑优化
			// 只有当丢失视野的这个人类，刚好是黑板里记录的当前目标时，才清空黑板。
			// 避免小兵在追击人类A时，人类B路过并离开视野，导致小兵莫名其妙清空了对人类A的记忆。
			UObject* CurrentTarget = BlackboardComp->GetValueAsObject(TargetKeyName);
			if (CurrentTarget == HumanTarget)
			{
				BlackboardComp->ClearValue(TargetKeyName);
			}
		}
	}
}
