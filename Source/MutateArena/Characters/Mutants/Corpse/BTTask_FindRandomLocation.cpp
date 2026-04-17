#include "BTTask_FindRandomLocation.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "NavigationSystem.h" // 必须包含导航系统头文件

UBTTask_FindRandomLocation::UBTTask_FindRandomLocation()
{
	NodeName = "Find Random Location";
	
	// 限定这个节点在编辑器里只能绑定 Vector（坐标）类型的黑板键
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindRandomLocation, BlackboardKey));
}

EBTNodeResult::Type UBTTask_FindRandomLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	APawn* AIPawn = AIController->GetPawn();
	if (!AIPawn) return EBTNodeResult::Failed;

	// 获取小兵当前位置
	FVector Origin = AIPawn->GetActorLocation();
	FNavLocation RandomLocation;

	// 获取世界的导航系统
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (NavSystem && NavSystem->GetRandomReachablePointInRadius(Origin, SearchRadius, RandomLocation))
	{
		// 找到点后，写入我们指定的黑板键
		OwnerComp.GetBlackboardComponent()->SetValueAsVector(GetSelectedBlackboardKey(), RandomLocation.Location);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}
