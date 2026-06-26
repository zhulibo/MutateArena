#include "AssetThread.h"

#include "AssetSubsystem.h"
#include "HAL/Runnable.h"

AssetThread::AssetThread(UAssetSubsystem* TempAssetSubsystem)
{
	AssetSubsystem = TempAssetSubsystem;
}

bool AssetThread::Init()
{
	return FRunnable::Init();
}

uint32 AssetThread::Run()
{
	if (AssetSubsystem)
	{
		AssetSubsystem->Test();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AssetSubsystem nullptr!"));
	}

	return 0;
}

void AssetThread::Stop()
{
	FRunnable::Stop();
}

void AssetThread::Exit()
{
	FRunnable::Exit();
}
