#include "LibraryNotify.h"

#include "MutateArena/MutateArena.h"
#include "MutateArena/System/PlayerSubsystem.h"

void ULibraryNotify::AddNotify(const UObject* Context, const FColor DisplayColor, const FText& Msg)
{
	if (Context && Context->GetWorld())
	{
		if (UPlayerSubsystem* PlayerSubsystem = ULocalPlayer::GetSubsystem<UPlayerSubsystem>(Context->GetWorld()->GetFirstLocalPlayerFromController()))
		{
			PlayerSubsystem->AddNotify(DisplayColor, Msg);

			return;
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 20.f, C_RED, TEXT("Show notify failed!"), false);
}
