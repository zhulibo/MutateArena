#include "MysticVeil.h"
#include "Components/StaticMeshComponent.h"

AMysticVeil::AMysticVeil()
{
	PrimaryActorTick.bCanEverTick = false;
	
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	VeilMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VeilMesh"));
	VeilMesh->SetupAttachment(RootComponent);
    
	VeilMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	VeilMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	// 阻挡 ECC_Visibility
	VeilMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}
