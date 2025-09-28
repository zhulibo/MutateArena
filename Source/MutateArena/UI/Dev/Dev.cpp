#include "Dev.h"

#include "CommonTextBlock.h"
#include "KnownIssueLine.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "MutateArena/System/EOSSubsystem.h"
#include "MutateArena/Utils/LibraryCommon.h"

void UDev::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	EOSSubsystem = GetGameInstance()->GetSubsystem<UEOSSubsystem>();
	if (EOSSubsystem)
	{
		EOSSubsystem->OnEnumerateTitleFilesComplete.AddUObject(this, &ThisClass::OnEnumerateTitleFilesComplete);
		EOSSubsystem->OnReadTitleFileComplete.AddUObject(this, &ThisClass::OnReadTitleFileComplete);
	}
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

void UDev::OnEnumerateTitleFilesComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful) return;
	
	if (EOSSubsystem && EOSSubsystem->GetEnumeratedTitleFiles().Contains(TitleFile_KnownIssues))
	{
		EOSSubsystem->ReadTitleFile(TitleFile_KnownIssues);
	}
}

void UDev::OnReadTitleFileComplete(bool bWasSuccessful, const UE::Online::FTitleFileContentsRef& FileContents)
{
	if (!bWasSuccessful) return;

	FString JsonString;
	FFileHelper::BufferToString(JsonString, FileContents->GetData(), FileContents->Num());
	
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		if (JsonObject->GetStringField(TEXT("FileName")) != TitleFile_KnownIssues) return;
		// UE_LOG(LogTemp, Log, TEXT("JsonString %s"), *JsonString);
		
		FString Language;
		if (ULibraryCommon::GetLanguage().Contains(TEXT("zh")))
		{
			Language = TEXT("zh");
		}
		else
		{
			Language = TEXT("en");
		}
		
		const TArray<TSharedPtr<FJsonValue>> Items = JsonObject->GetArrayField(TEXT("Content"));
		for (auto item : Items)
		{
			const TSharedPtr<FJsonObject> itemObject = item->AsObject();
			if (!itemObject.IsValid())
			{
				continue;
			}

			FString itemString = itemObject->GetStringField(Language);
			
			UKnownIssueLine* KnownIssueLine = CreateWidget<UKnownIssueLine>(this, KnownIssueLineClass);
			if (KnownIssueLine == nullptr) continue;
			
			KnownIssueLine->IssueText->SetText(FText::FromString(itemString));
			
			if (UVerticalBoxSlot* NewSlot = Cast<UVerticalBoxSlot>(KnownIssuesContainer->AddChild(KnownIssueLine)))
			{
				NewSlot->SetPadding(FMargin(0, 10, 0, 0));
			}

		}
	}
}
