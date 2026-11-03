#include "AnimInstGhost.h"

UAnimInstGhost::UAnimInstGhost()
{
}

void UAnimInstGhost::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UAnimInstGhost::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
}

void UAnimInstGhost::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
}
