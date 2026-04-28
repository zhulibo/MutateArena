#include "DM_SemiCylinder.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshBooleanFunctions.h"
#include "UDynamicMesh.h"
#include "Components/DynamicMeshComponent.h"

ADM_SemiCylinder::ADM_SemiCylinder()
{
    PrimaryActorTick.bCanEverTick = false;

    DynamicMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("DynamicMeshComponent"));
    RootComponent = DynamicMeshComponent;
    
    DynamicMeshComponent->bEnableComplexCollision = true;
    DynamicMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    DynamicMeshComponent->CollisionType = ECollisionTraceFlag::CTF_UseComplexAsSimple;
}

void ADM_SemiCylinder::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    Generate();
}

void ADM_SemiCylinder::Generate()
{
    if (!DynamicMeshComponent) return;
    UDynamicMesh* TargetMesh = DynamicMeshComponent->GetDynamicMesh();
    if (!TargetMesh) return;

    TargetMesh->Reset();
    FGeometryScriptPrimitiveOptions PrimOptions;
    
    FGeometryScriptMeshBooleanOptions BoolOptions;
    BoolOptions.bFillHoles = true; 

    // 绑定材质槽位
    if (Material) DynamicMeshComponent->SetMaterial(0, Material);

    // 计算生成完整圆柱需要的总段数
    int32 TotalCylinderSteps = ArcSteps;

    // 1. 生成完整的圆柱体 (中心在原点，默认 OriginMode 是 Base，Z轴贴地)
    UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendCylinder(
        TargetMesh, PrimOptions, FTransform::Identity, 
        Radius, Height, TotalCylinderSteps, 1, true
    );

    // 2. 构造用来切除一半的“切刀”网格体 (使用 Box)
    UDynamicMesh* ToolMesh = NewObject<UDynamicMesh>();
    
    // 安全穿插量，确保切刀比圆柱大，防止边缘和上下底面切出零厚度面片
    float SafeOverlap = 10.0f;
    
    // 切刀的长宽要大于圆柱的直径，高度大于圆柱高度
    float BoxSize = Radius * 2.0f + SafeOverlap;
    float BoxHeight = Height + SafeOverlap * 2.0f;

    // 刀的位置：X 轴居中，Y 轴向负方向偏移盒子宽度的一半，Z 轴向下偏移安全量
    // 这样包围盒恰好覆盖所有 Y < 0 的区域，切完后只剩下 Y > 0 的半个圆柱
    FTransform CutoutTransform(FVector(0.f, -BoxSize / 2.f, -SafeOverlap));
    
    UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendBox(
        ToolMesh, PrimOptions, CutoutTransform, 
        BoxSize, BoxSize, BoxHeight, 
        1, 1, 1 
    );

    // 3. 一刀切掉 Y 轴负半区的圆柱体
    UGeometryScriptLibrary_MeshBooleanFunctions::ApplyMeshBoolean(
        TargetMesh, FTransform::Identity, ToolMesh, FTransform::Identity,
        EGeometryScriptBooleanOperation::Subtract, BoolOptions
    );

    ToolMesh->ConditionalBeginDestroy();
    DynamicMeshComponent->NotifyMeshUpdated();
}