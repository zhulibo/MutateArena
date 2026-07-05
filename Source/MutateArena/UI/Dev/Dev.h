#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Online/TitleFile.h"
#include "Dev.generated.h"

class UVersion;

UCLASS()
class MUTATEARENA_API UDev : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY()
	class UEOSSubsystem* EOSSubsystem;
	
	UPROPERTY(meta = (BindWidget))
	UVersion* Version;
	
	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* KnownIssuesContainer;
	UPROPERTY(EditAnywhere)
	TSubclassOf<class UKnownIssueLine> KnownIssueLineClass;
	UPROPERTY()
	FString TitleFile_KnownIssues = TEXT("KnownIssues");
	void OnEnumerateTitleFilesComplete(bool bWasSuccessful);
	void OnReadTitleFileComplete(bool bWasSuccessful, const UE::Online::FTitleFileContentsRef& FileContents, const FString& Filename);

};
