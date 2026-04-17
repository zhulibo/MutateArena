#include "CorpseCharacter.h"
#include "Perception/AISense_Sight.h"

ACorpseCharacter::ACorpseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	// 确保被 GAS 或 Server 动态召唤时，AI 控制器能自动附身
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	StimuliSourceComp = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSourceComponent"));
	StimuliSourceComp->RegisterForSense(UAISense_Sight::StaticClass());
	
	// TODO 不与突变体碰撞
}

void ACorpseCharacter::BeginPlay()
{
	Super::BeginPlay();
}
