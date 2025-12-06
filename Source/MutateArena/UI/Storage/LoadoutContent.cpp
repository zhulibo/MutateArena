#include "LoadoutContent.h"

#include "StorageButton.h"

void ULoadoutContent::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	Tool->SetIsEnabled(false);
	Tonic->SetIsEnabled(false);
}
