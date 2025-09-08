#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LibraryNotify.generated.h"

#define NOTIFY(Context, DisplayColor, Msg) ULibraryNotify::AddNotify(Context, DisplayColor, Msg)

UCLASS()
class MUTATEARENA_API ULibraryNotify : public UObject
{
	GENERATED_BODY()

public:
	static void AddNotify(const UObject* Context, FColor DisplayColor, const FText& Msg);

};
