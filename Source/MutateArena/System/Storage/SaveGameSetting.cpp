#include "SaveGameSetting.h"

#include "DefaultConfig.h"

USaveGameSetting::USaveGameSetting()
{
	const UDefaultConfig* DefaultConfig = GetDefault<UDefaultConfig>();

	if (DefaultConfig == nullptr) return;

	Language = DefaultConfig->Language;
	bHideSkins = DefaultConfig->bHideSkins;
	bObfuscateName = DefaultConfig->bObfuscateName;
	bObfuscateTextChat = DefaultConfig->bObfuscateTextChat;

	MouseSensitivity = DefaultConfig->MouseSensitivity;
	ControllerSensitivity = DefaultConfig->ControllerSensitivity;

	WindowMode = DefaultConfig->WindowMode;
	ScreenResolution = DefaultConfig->ScreenResolution;
	Brightness = DefaultConfig->Brightness;

	MasterVolume = DefaultConfig->MasterVolume;
	EffectsVolume = DefaultConfig->EffectsVolume;
	MusicVolume = DefaultConfig->MusicVolume;
	DialogueVolume = DefaultConfig->DialogueVolume;
}
