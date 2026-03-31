#include "ArenaGround.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshBooleanFunctions.h"
#include "GeometryScript/MeshMaterialFunctions.h" 
#include "Materials/MaterialInterface.h"          
#include "UDynamicMesh.h"
#include "Components/DynamicMeshComponent.h"

AArenaGround::AArenaGround()
{
    PrimaryActorTick.bCanEverTick = false;

    DynamicMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("DynamicMeshComponent"));
    RootComponent = DynamicMeshComponent;
    
    DynamicMeshComponent->bEnableComplexCollision = true;
    DynamicMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    DynamicMeshComponent->CollisionType = ECollisionTraceFlag::CTF_UseComplexAsSimple;
}

void AArenaGround::BeginPlay()
{
    Super::BeginPlay();
}

void AArenaGround::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    
    Generate();
}

void AArenaGround::Generate()
{
    if (!DynamicMeshComponent) return;
    UDynamicMesh* TargetMesh = DynamicMeshComponent->GetDynamicMesh();
    if (!TargetMesh) return;

    TargetMesh->Reset();
    FGeometryScriptPrimitiveOptions PrimOptions;
    
    FGeometryScriptMeshBooleanOptions BoolOptions;
    BoolOptions.bFillHoles = true; 

    // 绑定材质槽位 (新增了管道外壁的槽位 3)
    if (GroundMaterial)   DynamicMeshComponent->SetMaterial(0, GroundMaterial);
    if (PlatformMaterial) DynamicMeshComponent->SetMaterial(1, PlatformMaterial);
    if (HoleMaterial)     DynamicMeshComponent->SetMaterial(2, HoleMaterial);
    if (PipeMaterial)     DynamicMeshComponent->SetMaterial(3, PipeMaterial);
    
    // 生成地面 (ID 0)
    FTransform GroundTransform(FVector(GroundSize.X / 2.f, GroundSize.Y / 2.f, 0.f));
    UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendBox(
        TargetMesh, PrimOptions, GroundTransform, 
        GroundSize.X, GroundSize.Y, GroundSize.Z, 
        1, 1, 1 
    );
    UGeometryScriptLibrary_MeshMaterialFunctions::ClearMaterialIDs(TargetMesh, 0);

    // 构造外圈平台 (ID 1)
    UDynamicMesh* PlatformMesh = NewObject<UDynamicMesh>();
    float SafeOverlap = 1.0f; 
    FTransform PlatformTransform(FVector(GroundSize.X / 2.f, GroundSize.Y / 2.f, GroundSize.Z - SafeOverlap));
    UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendCone(
        PlatformMesh, PrimOptions, PlatformTransform, 
        PlatformBottomRadius, PlatformRadius, 
        PlatformHeight + SafeOverlap, 
        RadialSteps, 1, true
    );
    UGeometryScriptLibrary_MeshMaterialFunctions::ClearMaterialIDs(PlatformMesh, 1);
    
    UGeometryScriptLibrary_MeshBooleanFunctions::ApplyMeshBoolean(
        TargetMesh, FTransform::Identity, PlatformMesh, FTransform::Identity,
        EGeometryScriptBooleanOperation::Union, BoolOptions
    );
    PlatformMesh->ConditionalBeginDestroy();

    // 构造管道的实心外壳并融合 (ID 3)
    UDynamicMesh* PipeOuterMesh = NewObject<UDynamicMesh>();
    // 管道从地面的底面 (Z=0) 开始向下延伸，所以 Z起点 为 -PipeLength
    FTransform PipeTransform(FVector(GroundSize.X / 2.f, GroundSize.Y / 2.f, -PipeLength));
    // 管道外壁的半径 = 洞口半径 + 我们设定的厚度
    float OuterPipeRadius = HoleRadius + PipeThickness;
    
    // 生成实心圆柱（高度要加上安全穿插量插进地面）
    UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendCylinder(
        PipeOuterMesh, PrimOptions, PipeTransform, 
        OuterPipeRadius, PipeLength + SafeOverlap, 
        RadialSteps, 1, true
    );
    UGeometryScriptLibrary_MeshMaterialFunctions::ClearMaterialIDs(PipeOuterMesh, 3);

    // 把管道外壳也“焊接”到主体上
    UGeometryScriptLibrary_MeshBooleanFunctions::ApplyMeshBoolean(
        TargetMesh, FTransform::Identity, PipeOuterMesh, FTransform::Identity,
        EGeometryScriptBooleanOperation::Union, BoolOptions
    );
    PipeOuterMesh->ConditionalBeginDestroy();

    // 构造挖洞大刀 (ID 2)
    UDynamicMesh* ToolMesh = NewObject<UDynamicMesh>();
    
    // 刀的底部 Z 坐标 = 管道最底部 (-PipeLength) 再往下多伸出 100
    float HoleBottomZ = -PipeLength - 100.f;
    
    // 刀的总高度 = 管道长度 + 地面厚度 + 平台高度 + 上下冗余
    float ToolHeight = PipeLength + GroundSize.Z + PlatformHeight + 200.f;
    
    FTransform HoleTransform(FVector(GroundSize.X / 2.f, GroundSize.Y / 2.f, HoleBottomZ));
    UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendCylinder(
       ToolMesh, PrimOptions, HoleTransform, 
       HoleRadius, ToolHeight, 
       RadialSteps, 1, true
    );
    
    // 切出来的内壁材质为 2
    UGeometryScriptLibrary_MeshMaterialFunctions::ClearMaterialIDs(ToolMesh, 2);

    // 一刀捅穿全部 (平台、地面、实心管道全部被掏出中央通道)
    UGeometryScriptLibrary_MeshBooleanFunctions::ApplyMeshBoolean(
        TargetMesh, FTransform::Identity, ToolMesh, FTransform::Identity,
        EGeometryScriptBooleanOperation::Subtract, BoolOptions
    );

    ToolMesh->ConditionalBeginDestroy();
    DynamicMeshComponent->NotifyMeshUpdated();
}
