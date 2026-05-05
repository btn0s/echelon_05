using UnrealBuildTool;

public class BlueprintAutoLayout : ModuleRules
{
	public BlueprintAutoLayout(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"BlueprintGraph",
			"Engine",
			"GraphEditor",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UnrealEd"
		});
	}
}
