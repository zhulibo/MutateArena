#pragma once

#include "CoreMinimal.h"

namespace ObfuscateDict
{
	inline const TArray<FString> Adjectives = {
		// --- 颜色与视觉 (30) ---
		TEXT("Red"), TEXT("Blue"), TEXT("Green"), TEXT("Yellow"), TEXT("Black"), 
		TEXT("White"), TEXT("Golden"), TEXT("Silver"), TEXT("Crimson"), TEXT("Purple"),
		TEXT("Neon"), TEXT("Dark"), TEXT("Pale"), TEXT("Shiny"), TEXT("Rusty"),
		TEXT("Pink"), TEXT("Brown"), TEXT("Orange"), TEXT("Cyan"), TEXT("Magenta"),
		TEXT("Grey"), TEXT("Violet"), TEXT("Amber"), TEXT("Ruby"), TEXT("Emerald"),
		TEXT("Sapphire"), TEXT("Pearl"), TEXT("Clear"), TEXT("Opaque"), TEXT("Glowing"),

		// --- 状态与性格 (45) ---
		TEXT("Crazy"), TEXT("Brave"), TEXT("Silent"), TEXT("Angry"), TEXT("Happy"), 
		TEXT("Sleepy"), TEXT("Blind"), TEXT("Swift"), TEXT("Hidden"), TEXT("Lost"), 
		TEXT("Wild"), TEXT("Calm"), TEXT("Fierce"), TEXT("Lazy"), TEXT("Sneaky"), 
		TEXT("Grumpy"), TEXT("Savage"), TEXT("Elite"), TEXT("Epic"), TEXT("Clumsy"),
		TEXT("Proud"), TEXT("Shy"), TEXT("Clever"), TEXT("Dumb"), TEXT("Fast"),
		TEXT("Slow"), TEXT("Strong"), TEXT("Weak"), TEXT("Tough"), TEXT("Fragile"),
		TEXT("Cool"), TEXT("Hot"), TEXT("Cold"), TEXT("Warm"), TEXT("Loyal"),
		TEXT("Evil"), TEXT("Holy"), TEXT("Rad"), TEXT("Mad"), TEXT("Sad"),
		TEXT("Glad"), TEXT("Gentle"), TEXT("Furious"), TEXT("Bored"), TEXT("Anxious"),

		// --- 元素、材质与科幻 (45) ---
		TEXT("Fire"), TEXT("Ice"), TEXT("Storm"), TEXT("Thunder"), TEXT("Shadow"), 
		TEXT("Light"), TEXT("Iron"), TEXT("Steel"), TEXT("Stone"), TEXT("Crystal"), 
		TEXT("Toxic"), TEXT("Cyber"), TEXT("Magic"), TEXT("Cosmic"), TEXT("Bloody"),
		TEXT("Plasma"), TEXT("Acid"), TEXT("Mud"), TEXT("Sand"), TEXT("Dust"),
		TEXT("Ash"), TEXT("Smoke"), TEXT("Void"), TEXT("Brass"), TEXT("Copper"),
		TEXT("Wood"), TEXT("Glass"), TEXT("Bone"), TEXT("Meat"), TEXT("Tech"),
		TEXT("Bio"), TEXT("Nano"), TEXT("Liquid"), TEXT("Solid"), TEXT("Gas"),
		TEXT("Atomic"), TEXT("Quantum"), TEXT("Solar"), TEXT("Lunar"), TEXT("Stellar"),
		TEXT("Aero"), TEXT("Aqua"), TEXT("Terra"), TEXT("Pyro"), TEXT("Cryo"),

		// --- 体型与程度 (30) ---
		TEXT("Big"), TEXT("Tiny"), TEXT("Giant"), TEXT("Fat"), TEXT("Skinny"),
		TEXT("Heavy"), TEXT("Ultra"), TEXT("Super"), TEXT("Mega"), TEXT("Mini"),
		TEXT("Micro"), TEXT("Macro"), TEXT("Mystic"), TEXT("Grand"), TEXT("Minor"),
		TEXT("Prime"), TEXT("Alpha"), TEXT("Beta"), TEXT("Omega"), TEXT("Hyper"),
		TEXT("Psycho"), TEXT("Supreme"), TEXT("Ultimate"), TEXT("Basic"), TEXT("Pro"),
		TEXT("Noob"), TEXT("Godly"), TEXT("Mortal"), TEXT("Immortal"), TEXT("Undead")
	};

	inline const TArray<FString> Nouns = {
		// --- 动物与生物 (40) ---
		TEXT("Bird"), TEXT("Wolf"), TEXT("Bear"), TEXT("Tiger"), TEXT("Shark"), 
		TEXT("Eagle"), TEXT("Fox"), TEXT("Frog"), TEXT("Cat"), TEXT("Dog"), 
		TEXT("Snake"), TEXT("Dragon"), TEXT("Panda"), TEXT("Owl"), TEXT("Raven"),
		TEXT("Lion"), TEXT("Panther"), TEXT("Hawk"), TEXT("Bat"), TEXT("Rat"),
		TEXT("Mouse"), TEXT("Cow"), TEXT("Bull"), TEXT("Pig"), TEXT("Horse"),
		TEXT("Spider"), TEXT("Ant"), TEXT("Bee"), TEXT("Wasp"), TEXT("Fly"),
		TEXT("Beetle"), TEXT("Worm"), TEXT("Snail"), TEXT("Crab"), TEXT("Fish"),
		TEXT("Whale"), TEXT("Squid"), TEXT("Octopus"), TEXT("Ape"), TEXT("Monkey"),

		// --- 身份与职业 (40) ---
		TEXT("Hunter"), TEXT("Sniper"), TEXT("Ninja"), TEXT("Knight"), TEXT("Ghost"), 
		TEXT("Phantom"), TEXT("Runner"), TEXT("Doctor"), TEXT("Pilot"), TEXT("Ranger"), 
		TEXT("Warrior"), TEXT("Goblin"), TEXT("Mutant"), TEXT("Cyborg"), TEXT("Zombie"),
		TEXT("Shooter"), TEXT("Camper"), TEXT("Hacker"), TEXT("Scout"), TEXT("Medic"),
		TEXT("Killer"), TEXT("Healer"), TEXT("Tank"), TEXT("DPS"), TEXT("Boss"),
		TEXT("Chief"), TEXT("King"), TEXT("Queen"), TEXT("Lord"), TEXT("Peasant"),
		TEXT("Pirate"), TEXT("Samurai"), TEXT("Agent"), TEXT("Spy"), TEXT("Cop"),
		TEXT("Thief"), TEXT("Robber"), TEXT("Bandit"), TEXT("Rebel"), TEXT("Guard"),

		// --- 物品与武器 (40) ---
		TEXT("Cake"), TEXT("Paper"), TEXT("Apple"), TEXT("Potato"), TEXT("Cheese"), 
		TEXT("Bread"), TEXT("Pizza"), TEXT("Coffee"), TEXT("Box"), TEXT("Blade"), 
		TEXT("Gun"), TEXT("Bullet"), TEXT("Rocket"), TEXT("Shield"), TEXT("Armor"),
		TEXT("Knife"), TEXT("Sword"), TEXT("Axe"), TEXT("Bow"), TEXT("Arrow"),
		TEXT("Spear"), TEXT("Mace"), TEXT("Club"), TEXT("Stick"), TEXT("Bomb"),
		TEXT("Nuke"), TEXT("Mine"), TEXT("Trap"), TEXT("Wire"), TEXT("Gear"),
		TEXT("Wheel"), TEXT("Engine"), TEXT("Motor"), TEXT("Tomato"), TEXT("Onion"),
		TEXT("Garlic"), TEXT("Burger"), TEXT("Taco"), TEXT("Sushi"), TEXT("Soup"),

		// --- 自然与意象 (30) ---
		TEXT("Tree"), TEXT("Rock"), TEXT("Star"), TEXT("Moon"), TEXT("Sun"), 
		TEXT("Cloud"), TEXT("Storm"), TEXT("Mountain"), TEXT("River"), TEXT("Skull"), 
		TEXT("Demon"), TEXT("Angel"), TEXT("Soul"), TEXT("Reaper"), TEXT("Beast"),
		TEXT("Ocean"), TEXT("Sea"), TEXT("Lake"), TEXT("Pond"), TEXT("Rain"),
		TEXT("Snow"), TEXT("Hail"), TEXT("Wind"), TEXT("Tornado"), TEXT("Hurricane"),
		TEXT("Quake"), TEXT("Volcano"), TEXT("Island"), TEXT("Planet"), TEXT("Galaxy")
	};
}
