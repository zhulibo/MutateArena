#include "SmokeBlocker.h"
#include "Components/SphereComponent.h"
#include "MutateArena/MutateArena.h"
#include "MutateArena/System/Tags/ProjectTags.h"

ASmokeBlocker::ASmokeBlocker()
{
	PrimaryActorTick.bCanEverTick = false;
	
	SmokeBlockerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("SmokeBlockerSphere"));
	RootComponent = SmokeBlockerSphere;
	SmokeBlockerSphere->SetSphereRadius(400.f);
    
	// 设置碰撞与遮挡
	SmokeBlockerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// 阻挡视线 UOverheadWidget
	SmokeBlockerSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	
	Tags.Add(TAG_SMOKE_ACTOR);
}
