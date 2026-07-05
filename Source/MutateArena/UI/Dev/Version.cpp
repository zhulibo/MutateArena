#include "Version.h"

#include "CommonTextBlock.h"
#include "Misc/EngineVersion.h"
#include "Misc/CommandLine.h"
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

	// 获取当前使用的 SandboxId 逻辑同FOnlineServicesEpicCommonPlatformFactory::LoadEOSPlatformConfig
	FString CurrentSandboxId;

	if (GConfig)
	{
		// 1. 先读取默认 SandboxId
		GConfig->GetString(TEXT("OnlineServices.EOS"), TEXT("SandboxId"), CurrentSandboxId, GEngineIni);

#if UE_BUILD_SHIPPING
		// 2. 如果是 SHIPPING 包，读取 Live 环境的 ID 覆盖
		GConfig->GetString(TEXT("OnlineServices.EOS"), TEXT("SandboxId_Live"), CurrentSandboxId, GEngineIni);
#endif
	}

	// 3. 命令行参数具有最高优先级，如果游戏是从 Epic Games Launcher 启动的，用命令行参数覆盖
	FParse::Value(FCommandLine::Get(), TEXT("EpicSandboxId="), CurrentSandboxId);

	// 将 SandboxId 映射为环境名称
	FString Environment = TEXT("Unknown");
	if (CurrentSandboxId.Contains(TEXT("p-9hdddp2tmrprnmnbt2gbur4m6dqs6d")))
	{
		Environment = TEXT("Dev");
	}
	else if (CurrentSandboxId.Contains(TEXT("p-juzhxm7jefpr8xgvygjlfvk6ayx7yq")))
	{
		Environment = TEXT("Stage");
	}
	else if (CurrentSandboxId.Contains(TEXT("89f212c83ee24545b54c54a268018166")))
	{
		Environment = TEXT("Live");
	}

	// 将构建类型、项目版本与环境拼接，格式示例："Shipping - 1.0.0 - Live"
	FString FullProjectVersion = FString::Printf(TEXT("%s-%s-%s"), *BuildConfiguration, *ULibraryCommon::GetProjectVersion(), *Environment);
	ProjectVersion->SetText(FText::FromString(FullProjectVersion));

	// UE 5.8.0
	const FEngineVersion& CurVersion = FEngineVersion::Current();
	int32 Major = CurVersion.GetMajor();
	int32 Minor = CurVersion.GetMinor();
	int32 Patch = CurVersion.GetPatch();
	EngineVersion->SetText(FText::FromString(FString::Printf(TEXT("UE%d.%d.%d"), Major, Minor, Patch)));
}
