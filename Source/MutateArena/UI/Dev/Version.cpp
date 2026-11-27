#include "Version.h"

#include "CommonTextBlock.h"
#include "IEOSSDKManager.h"
#include "Misc/EngineVersion.h"
#include "Misc/ConfigCacheIni.h"
#include "MutateArena/Utils/LibraryCommon.h"

void UVersion::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UVersion::NativeConstruct()
{
	Super::NativeConstruct();

	// 判断当前的构建配置
	FString BuildConfiguration;
#if UE_BUILD_SHIPPING
	BuildConfiguration = TEXT("Shipping");
#elif UE_BUILD_TEST
	BuildConfiguration = TEXT("Test");
#elif UE_BUILD_DEVELOPMENT
	BuildConfiguration = TEXT("Development");
#elif UE_BUILD_DEBUG
	BuildConfiguration = TEXT("Debug");
#else
	BuildConfiguration = TEXT("Unknown");
#endif

	// 格式示例：Shipping - 1.0.0
	FString FullProjectVersion = FString::Printf(TEXT("%s - %s"), *BuildConfiguration, *ULibraryCommon::GetProjectVersion());
	Project->SetText(FText::FromString(FullProjectVersion));

	// 格式示例：UE 5.8.0
	const FEngineVersion& CurVersion = FEngineVersion::Current();
	int32 Major = CurVersion.GetMajor();
	int32 Minor = CurVersion.GetMinor();
	int32 Patch = CurVersion.GetPatch();
	Engine->SetText(FText::FromString(FString::Printf(TEXT("UE %d.%d.%d"), Major, Minor, Patch)));

	// 从 EOS 获取当前生效的 SandboxId 和 SDK 版本
	FString CurrentSandboxId;
	FString SDKVersion;
	if (IEOSSDKManager* SDKManager = IEOSSDKManager::Get())
	{
		SDKVersion = SDKManager->GetSDKVersion();
		TArray<IEOSPlatformHandlePtr> ActivePlatforms = SDKManager->GetActivePlatforms();
		if (!ActivePlatforms.IsEmpty())
		{
			if (const FEOSSDKPlatformConfig* PlatformConfig = SDKManager->GetPlatformConfig(ActivePlatforms[0]->GetConfigName()))
			{
				CurrentSandboxId = PlatformConfig->SandboxId;
			}
		}
	}

	// 与 ini 配置中的 SandboxId 对比，映射为环境名称
	FString EOSEnvironment = TEXT("Unknown");
	if (GConfig && !CurrentSandboxId.IsEmpty())
	{
		FString SandboxIdDev, SandboxIdStage, SandboxIdLive;
		GConfig->GetString(TEXT("OnlineServices.EOS"), TEXT("SandboxId"), SandboxIdDev, GEngineIni);
		GConfig->GetString(TEXT("OnlineServices.EOS"), TEXT("SandboxId_Stage"), SandboxIdStage, GEngineIni);
		GConfig->GetString(TEXT("OnlineServices.EOS"), TEXT("SandboxId_Live"), SandboxIdLive, GEngineIni);

		if (CurrentSandboxId == SandboxIdDev)
		{
			EOSEnvironment = TEXT("Dev");
		}
		else if (CurrentSandboxId == SandboxIdStage)
		{
			EOSEnvironment = TEXT("Stage");
		}
		else if (CurrentSandboxId == SandboxIdLive)
		{
			EOSEnvironment = TEXT("Live");
		}
	}

	// 格式示例：Live - 1.19.1.2-53289219
	FString FullEOSSDKVersion = FString::Printf(TEXT("EOSSDK %s - %s"), *EOSEnvironment, *SDKVersion);
	EOSSDK->SetText(FText::FromString(FullEOSSDKVersion));
}
