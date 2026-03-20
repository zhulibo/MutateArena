#include "MAGameplayCueManager.h"

#include "AbilitySystemGlobals.h"

UMAGameplayCueManager* UMAGameplayCueManager::Get()
{
	return Cast<UMAGameplayCueManager>(UAbilitySystemGlobals::Get().GetGameplayCueManager());
}

// 异步加载会导致首次切到 Kukri 首帧处于 Idle 状态
// https://github.com/tranek/GASDocumentation?tab=readme-ov-file#concepts-gc-manager
// bool UMAGameplayCueManager::ShouldAsyncLoadRuntimeObjectLibraries() const
// {
// 	return false;
// }
