#pragma once

#include "Modules/ModuleManager.h"

class FExtender;
class FUICommandList;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;

class FBlueprintAutoLayoutModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedRef<FExtender> ExtendGraphContextMenu(const TSharedRef<FUICommandList> CommandList, const UEdGraph* Graph, const UEdGraphNode* Node, const UEdGraphPin* Pin, bool bIsDebugging);
	void AddGraphContextMenuEntry(class FMenuBuilder& MenuBuilder, TWeakObjectPtr<const UEdGraph> Graph, TWeakObjectPtr<const UEdGraphNode> Node);
	void OrganizeSelection(TWeakObjectPtr<const UEdGraph> Graph, TWeakObjectPtr<const UEdGraphNode> ContextNode) const;

private:
	int32 GraphContextMenuExtenderHandle = INDEX_NONE;
};
