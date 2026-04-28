#include "DM_CubeSubCylinder.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshBooleanFunctions.h"
#include "GeometryScript/MeshMaterialFunctions.h" 
#include "Materials/MaterialInterface.h"          
#include "UDynamicMesh.h"
#include "Components/DynamicMeshComponent.h"

ADM_CubeSubCylinder::ADM_CubeSubCylinder()
{
    PrimaryActorTick.bCanEverTick = false;

    DynamicMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("DynamicMeshComponent"));
    RootComponent = DynamicMeshComponent;
    
    DynamicMeshComponent->bEnableComplexCollision = true;
    DynamicMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    DynamicMeshComponent->CollisionType = ECollisionTraceFlag::CTF_UseComplexAsSimple;
}

void ADM_CubeSubCylinder::BeginPlay()
{
    Super::BeginPlay();
}

void ADM_CubeSubCylinder::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    
    Generate();
}

void ADM_CubeSubCylinder::Generate()
{
    if (!DynamicMeshComponent) return;
    UDynamicMesh* TargetMesh = DynamicMeshComponent->GetDynamicMesh();
    if (!TargetMesh) return;

    TargetMesh->Reset();
    FGeometryScriptPrimitiveOptions PrimOptions;
    
    FGeometryScriptMeshBooleanOptions BoolOptions;
    BoolOptions.bFillHoles = true; 

    // 绑定材质槽位
    if (BodyMaterial)   DynamicMeshComponent->SetMaterial(0, BodyMaterial);
    if (CutoutMaterial) DynamicMeshComponent->SetMaterial(1, CutoutMaterial);
    
    // 生成主体矩形 (ID 0)
    // 逻辑和 ArenaGround 一样，把左下角放在原点，向 +X 和 +Y 延伸
    // 修复：AppendBox 默认 OriginMode 为 Base，Z 轴默认从 0 开始，因此 Z 偏移设为 0 即可对齐底部
    FTransform BodyTransform(FVector(BodySize.X / 2.f, BodySize.Y / 2.f, 0.f));
    
    UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendBox(
        TargetMesh, PrimOptions, BodyTransform, 
        BodySize.X, BodySize.Y, BodySize.Z, 
        1, 1, 1 
    );
    UGeometryScriptLibrary_MeshMaterialFunctions::ClearMaterialIDs(TargetMesh, 0);

    // 构造挖洞大刀 (ID 1)
    UDynamicMesh* ToolMesh = NewObject<UDynamicMesh>();
    
    // 安全穿插量，防止 Z 轴上刚好平齐导致布尔运算出错（切出零厚度面片）
    float SafeOverlap = 10.f; 
    
    // 刀的总高度 = 矩形厚度 + 上下冗余
    float ToolHeight = BodySize.Z + SafeOverlap * 2.f;
    
    // 刀的位置：放在左侧边缘的中间 (X=0, Y=BodySize.Y/2)，Z轴从底端多伸出一点
    // AppendCylinder 默认 OriginMode 也是 Base，所以给一个 -SafeOverlap 就能向下穿透
    FTransform CutoutTransform(FVector(0.f, BodySize.Y / 2.f, -SafeOverlap));
    
    UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendCylinder(
       ToolMesh, PrimOptions, CutoutTransform, 
       CutoutRadius, ToolHeight, 
       RadialSteps, 1, true
    );
    
    // 切出来的内壁材质为 1
    UGeometryScriptLibrary_MeshMaterialFunctions::ClearMaterialIDs(ToolMesh, 1);

    // 一刀切出边缘半圆缺口
    UGeometryScriptLibrary_MeshBooleanFunctions::ApplyMeshBoolean(
        TargetMesh, FTransform::Identity, ToolMesh, FTransform::Identity,
        EGeometryScriptBooleanOperation::Subtract, BoolOptions
    );

    ToolMesh->ConditionalBeginDestroy();
    DynamicMeshComponent->NotifyMeshUpdated();
}
