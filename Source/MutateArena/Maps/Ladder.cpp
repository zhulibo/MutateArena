#include "Ladder.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
// 引入你的 Tag 宏定义文件（如果 TAG_LADDER 定义在这里）
#include "MutateArena/MutateArena.h"
#include "MutateArena/System/Tags/ProjectTags.h" 

ALadder::ALadder()
{
	PrimaryActorTick.bCanEverTick = false;
	
	RootAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("RootAnchor"));
	SetRootComponent(RootAnchor);
	
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(RootComponent);
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	
	LadderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LadderMesh"));
	LadderMesh->SetupAttachment(RootComponent);
	
	Tags.Add(TAG_LADDER); 
}

void ALadder::BeginPlay()
{
	Super::BeginPlay();
}
