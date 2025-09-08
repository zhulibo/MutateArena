如果将来弃用bEnableLegacyInputScales会导致后坐力手感发生较大变化，所以提前弃用
修改bEnableLegacyInputScales=False后
不知道为什么ASpectatorCharacter与ABaseCharacter俯仰方向相反
又修改了项目设置-引擎-输入-轴映射中的 MouseY和Gamepad_RightY bInvert=True

DefaultEngine.ini中的
;Only used for PIE
[/Script/Engine.Engine]
!NetDriverDefinitions=ClearArray
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="SocketSubsystemEOS.NetDriverEOSBase",DriverClassNameFallback="OnlineSubsystemUtils.IpNetDriver")
+NetDriverDefinitions=(DefName="DemoNetDriver",DriverClassName="/Script/Engine.DemoNetDriver",DriverClassNameFallback="/Script/Engine.DemoNetDriver")
[/Script/Engine.GameEngine]
!NetDriverDefinitions=ClearArray
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="SocketSubsystemEOS.NetDriverEOSBase",DriverClassNameFallback="OnlineSubsystemUtils.IpNetDriver")
+NetDriverDefinitions=(DefName="DemoNetDriver",DriverClassName="/Script/Engine.DemoNetDriver",DriverClassNameFallback="/Script/Engine.DemoNetDriver")
只是为了PIE在当前关卡中运行，打包或以独立进程运行需注释掉！！！