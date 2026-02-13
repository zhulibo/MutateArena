## Mutate Arena

Mutate Arena is an online multiplayer first-person shooter game, the main gameplay is mutation mode.

## Overview

Built with Unreal Engine 5.6

Currently, first-person and third-person perspectives use the same models and animations, which reduces workload and maintains consistency (potentially beneficial for gameplay in the future).  
This approach has some issues, and if not properly addressed in the future, they may need to be separated.

## Binary files

This repository does not include binary files. You can download the complete project from [Onedrive](https://1drv.ms/f/c/c8d6dc43bb25e3ee/IgD93fUZ7ZNZTpOmeMLcDS6lAT9Fw6xc_htj9Up2uCiw0sc).

## Art Assets

.blender and .ssp files are available on [OneDrive](https://1drv.ms/f/c/c8d6dc43bb25e3ee/IgDu4yW7Q9zWIIDIowoAAAAAAZE-GKmT3dQX2dS5ddXecZM?e=KLbs87).
The version is Blender 5.0 and Substance 3D Painter 11.1

The traditional High-to-Low Poly workflow was not used (I used Nanite), and many parts of the model remain unbeveled to allow for easier change later.

If you want import fbx to UE, you need change source code
form  
bIsBlenderArmatureBone = (GrandFather == nullptr || GrandFather == SDKScene->GetRootNode()) && (RootBoneParentName.Compare(TEXT("armature"), ESearchCase::IgnoreCase) == 0);  
to  
bIsBlenderArmatureBone = (GrandFather == nullptr || GrandFather == SDKScene->GetRootNode()) && (RootBoneParentName.Compare(TEXT("￩ﾪﾨ￦ﾞﾶ"), ESearchCase::IgnoreCase) == 0);  
(The "armature" is hardcoded, and my Chinese version .blender file is named "骨架", and it will be garbled when imported into UE.)  
Otherwise, your root bone will have an extra bone named "骨架".

## Setup

If you want to run your own version of the game, you need to register your project at https://dev.epicgames.com/portal,  
and replace the IDs under the [OnlineServices.EOS](Config/DefaultEngine.ini) section in DefaultEngine.ini. 

Please pay attention to the comments in the [/Script/Engine.Engine](Config/DefaultEngine.ini) section of DefaultEngine.ini.

## Development status

It's currently in its earliest playable state and will remain in development for an extended period.

## README FOR PLAYERS

[README_PLAYER.txt](README_PLAYER.txt)

## Links

[www.mutatearena.com](https://www.mutatearena.com/)

[哔哩哔哩](https://space.bilibili.com/11741743/)
