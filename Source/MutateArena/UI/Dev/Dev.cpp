#include "Dev.h"

#include "CommonTextBlock.h"
#include "MutateArena/Utils/LibraryCommon.h"

void UDev::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UDev::NativeConstruct()
{
	Super::NativeConstruct();

	ProjectVersion->SetText(FText::FromString(ULibraryCommon::GetProjectVersion()));
	
	const FEngineVersion& CurVersion = FEngineVersion::Current();
	int32 Major = CurVersion.GetMajor();
	int32 Minor = CurVersion.GetMinor();
	int32 Patch = CurVersion.GetPatch();
	EngineVersion->SetText(FText::FromString(FString::Printf(TEXT("UE %d.%d.%d"), Major, Minor, Patch)));
}

UWidget* UDev::NativeGetDesiredFocusTarget() const
{
	return Super::NativeGetDesiredFocusTarget();
}
