## Art Assets

The traditional High-to-Low Poly workflow was not used (I used Nanite), and many parts of the model remain unbeveled to allow for easier change later.

If you want import fbx to UE, you need change UE source code
form  
bIsBlenderArmatureBone = (GrandFather == nullptr || GrandFather == SDKScene->GetRootNode()) && (RootBoneParentName.Compare(TEXT("armature"), ESearchCase::IgnoreCase) == 0);  
to  
bIsBlenderArmatureBone = (GrandFather == nullptr || GrandFather == SDKScene->GetRootNode()) && (RootBoneParentName.Compare(TEXT("￩ﾪﾨ￦ﾞﾶ"), ESearchCase::IgnoreCase) == 0);  
The "armature" is hardcoded, and my Chinese version .blender file is named "骨架", and it will be garbled to "￩ﾪﾨ￦ﾞﾶ" when importing.  
if you don't change UE source code, your root bone will have an extra bone named "骨架", you need to remove it manually.

