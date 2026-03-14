#include "GC_Equipment_SwapIn.h"

#include "MutateArena/Characters/HumanCharacter.h"
#include "MutateArena/Equipments/Equipment.h"
#include "MutateArena/Equipments/AnimInstEquipment.h"

UGC_Equipment_SwapIn::UGC_Equipment_SwapIn()
{
}

bool UGC_Equipment_SwapIn::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	AHumanCharacter* Instigator = Cast<AHumanCharacter>(Parameters.Instigator);
	AEquipment* CurEquipment = Cast<AEquipment>(const_cast<UObject*>(Parameters.SourceObject.Get()));
	if (Instigator && CurEquipment)
	{
		if (UAnimInstEquipment* EquipmentAnimInst = CurEquipment->GetEquipmentAnimInst())
		{
			// TODO 不知道为什么本地执行了两次
			// 仅当：是本地玩家 + 不是服务器 + 带有服务器标记时，才拦截
			if (Instigator->IsLocallyControlled() && !Instigator->HasAuthority() && Parameters.RawMagnitude == 1.f)
			{
				// 这是纯客户端收到了来自服务端的网络多播，因为客户端在GA里已经本地预测过一次了，所以忽略它
				// UE_LOG(LogTemp, Warning, TEXT("GC %d - Ignore Server Sync"), UE::GetPlayInEditorID());
			}
			else
			{
				// 1. 客户端本地预测执行 (Mag == 0) -> 播放
				// 2. 模拟端收到服务端多播 (IsLocallyControlled == false, Mag == 1) -> 播放
				// 3. 房主本地执行 (HasAuthority == true, Mag == 1) -> 播放
				EquipmentAnimInst->Montage_Play(CurEquipment->SwapInMontage_E);
				// UE_LOG(LogTemp, Warning, TEXT("GC %d - Play Montage"), UE::GetPlayInEditorID());
			}
		}
	}
	
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}
