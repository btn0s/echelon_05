#include "BlueprintAutoLayoutModule.h"

#include "EdGraphSchema_K2.h"
#include "EdGraphNode_Comment.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GraphEditor.h"
#include "GraphEditorModule.h"
#include "Layout/SlateRect.h"
#include "Misc/ScopedSlowTask.h"
#include "ScopedTransaction.h"
#include "ToolMenuSection.h"

#define LOCTEXT_NAMESPACE "BlueprintAutoLayout"

namespace BlueprintAutoLayout
{
	constexpr int32 HorizontalSpacing = 420;
	constexpr int32 VerticalSpacing = 170;
	constexpr int32 ExecSpacingX = 360;
	constexpr int32 ExecRowSpacingY = 260;
	constexpr int32 InputStackStartY = 80;
	constexpr int32 InputStackSpacingX = 190;
	constexpr int32 InputStackSpacingY = 72;
	constexpr int32 InputStackColumnBackX = 160;
	constexpr int32 InputStackDepthIndentX = 130;
	constexpr int32 CommentPadding = 64;
	constexpr int32 DefaultNodeWidth = 180;
	constexpr int32 DefaultNodeHeight = 90;
	constexpr int32 GridSnap = 16;

	struct FExecPlacement
	{
		int32 Column = 0;
		int32 Row = 0;
	};

	struct FCommentWrapInfo
	{
		UEdGraphNode_Comment* Comment = nullptr;
		TArray<UEdGraphNode*> ContainedNodes;
	};

	static bool IsRealGraphNode(const UEdGraphNode* Node, const UEdGraph* Graph)
	{
		return IsValid(Node) && Node->GetGraph() == Graph;
	}

	static TArray<UEdGraphNode*> GetNodesToOrganize(const UEdGraph* Graph, const UEdGraphNode* ContextNode)
	{
		TArray<UEdGraphNode*> Nodes;
		if (!Graph)
		{
			return Nodes;
		}

		if (TSharedPtr<SGraphEditor> GraphEditor = SGraphEditor::FindGraphEditorForGraph(Graph))
		{
			const FGraphPanelSelectionSet& SelectedObjects = GraphEditor->GetSelectedNodes();
			for (UObject* SelectedObject : SelectedObjects)
			{
				UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(SelectedObject);
				if (IsRealGraphNode(SelectedNode, Graph))
				{
					Nodes.AddUnique(SelectedNode);
				}
			}
		}

		if (Nodes.IsEmpty())
		{
			UEdGraphNode* MutableContextNode = const_cast<UEdGraphNode*>(ContextNode);
			if (IsRealGraphNode(MutableContextNode, Graph))
			{
				Nodes.Add(MutableContextNode);
			}
		}

		return Nodes;
	}

	static bool HasOutputLinkTo(const UEdGraphNode* SourceNode, const UEdGraphNode* TargetNode)
	{
		if (!SourceNode || !TargetNode)
		{
			return false;
		}

		for (const UEdGraphPin* Pin : SourceNode->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Output)
			{
				continue;
			}

			for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				if (LinkedPin && LinkedPin->GetOwningNode() == TargetNode)
				{
					return true;
				}
			}
		}

		return false;
	}

	static bool IsCommentNode(const UEdGraphNode* Node)
	{
		return Node && Node->IsA<UEdGraphNode_Comment>();
	}

	static int32 GetEstimatedNodeWidth(const UEdGraphNode* Node)
	{
		return Node && Node->NodeWidth > 0 ? Node->NodeWidth : DefaultNodeWidth;
	}

	static int32 GetEstimatedNodeHeight(const UEdGraphNode* Node)
	{
		return Node && Node->NodeHeight > 0 ? Node->NodeHeight : DefaultNodeHeight;
	}

	static bool IsNodeInsideComment(const UEdGraphNode* Node, const UEdGraphNode_Comment* Comment)
	{
		if (!Node || !Comment || Node == Comment)
		{
			return false;
		}

		const int32 CommentRight = Comment->NodePosX + GetEstimatedNodeWidth(Comment);
		const int32 CommentBottom = Comment->NodePosY + GetEstimatedNodeHeight(Comment);
		const int32 NodeRight = Node->NodePosX + GetEstimatedNodeWidth(Node);
		const int32 NodeBottom = Node->NodePosY + GetEstimatedNodeHeight(Node);

		return Node->NodePosX >= Comment->NodePosX && Node->NodePosY >= Comment->NodePosY && NodeRight <= CommentRight && NodeBottom <= CommentBottom;
	}

	static bool IsExecPin(const UEdGraphPin* Pin)
	{
		return Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec;
	}

	static bool HasExecPin(const UEdGraphNode* Node)
	{
		if (!Node)
		{
			return false;
		}

		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (IsExecPin(Pin))
			{
				return true;
			}
		}

		return false;
	}

	static bool HasExecInputFromSelected(const UEdGraphNode* Node, const TSet<UEdGraphNode*>& ExecNodes)
	{
		if (!Node)
		{
			return false;
		}

		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (!IsExecPin(Pin) || Pin->Direction != EGPD_Input)
			{
				continue;
			}

			for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				if (LinkedPin && ExecNodes.Contains(LinkedPin->GetOwningNode()))
				{
					return true;
				}
			}
		}

		return false;
	}

	static TArray<UEdGraphNode*> GetExecSuccessors(const UEdGraphNode* Node, const TSet<UEdGraphNode*>& ExecNodes)
	{
		TArray<UEdGraphNode*> Successors;
		if (!Node)
		{
			return Successors;
		}

		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (!IsExecPin(Pin) || Pin->Direction != EGPD_Output)
			{
				continue;
			}

			for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				UEdGraphNode* TargetNode = LinkedPin ? LinkedPin->GetOwningNode() : nullptr;
				if (ExecNodes.Contains(TargetNode))
				{
					Successors.AddUnique(TargetNode);
				}
			}
		}

		return Successors;
	}

	static TMap<UEdGraphNode*, FExecPlacement> ComputeExecPlacements(const TArray<UEdGraphNode*>& ExecNodeList)
	{
		TSet<UEdGraphNode*> ExecNodes(ExecNodeList);
		TMap<UEdGraphNode*, FExecPlacement> Placements;
		TArray<UEdGraphNode*> Roots;

		for (UEdGraphNode* Node : ExecNodeList)
		{
			if (!HasExecInputFromSelected(Node, ExecNodes))
			{
				Roots.Add(Node);
			}
		}

		if (Roots.IsEmpty())
		{
			Roots = ExecNodeList;
		}

		Roots.Sort([](const UEdGraphNode& Left, const UEdGraphNode& Right)
		{
			return Left.NodePosX == Right.NodePosX ? Left.NodePosY < Right.NodePosY : Left.NodePosX < Right.NodePosX;
		});

		struct FPendingExecNode
		{
			UEdGraphNode* Node = nullptr;
			FExecPlacement Placement;
		};

		TArray<FPendingExecNode> Pending;
		int32 NextFreeRow = 0;
		for (UEdGraphNode* Root : Roots)
		{
			Pending.Add({Root, FExecPlacement{0, NextFreeRow++}});
		}

		while (!Pending.IsEmpty())
		{
			const FPendingExecNode Current = Pending[0];
			Pending.RemoveAt(0);
			if (!Current.Node || Placements.Contains(Current.Node))
			{
				continue;
			}

			Placements.Add(Current.Node, Current.Placement);

			TArray<UEdGraphNode*> Successors = GetExecSuccessors(Current.Node, ExecNodes);
			for (int32 Index = 0; Index < Successors.Num(); ++Index)
			{
				if (Placements.Contains(Successors[Index]))
				{
					continue;
				}

				FPendingExecNode PendingNode;
				PendingNode.Node = Successors[Index];
				PendingNode.Placement.Column = Current.Placement.Column + 1;
				PendingNode.Placement.Row = Index == 0 ? Current.Placement.Row : NextFreeRow++;
				Pending.Add(PendingNode);
			}
		}

		for (UEdGraphNode* Node : ExecNodeList)
		{
			if (!Placements.Contains(Node))
			{
				Placements.Add(Node, FExecPlacement{0, NextFreeRow++});
			}
		}

		return Placements;
	}

	static void CollectInputDependencies(UEdGraphNode* Node, const TSet<UEdGraphNode*>& SelectedNodes, const TSet<UEdGraphNode*>& ExecNodes, int32 Depth, TMap<UEdGraphNode*, int32>& OutDepths, TSet<UEdGraphNode*>& TraversalStack)
	{
		if (!Node || TraversalStack.Contains(Node))
		{
			return;
		}

		TraversalStack.Add(Node);
		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Input || IsExecPin(Pin))
			{
				continue;
			}

			for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				UEdGraphNode* SourceNode = LinkedPin ? LinkedPin->GetOwningNode() : nullptr;
				if (!SelectedNodes.Contains(SourceNode) || ExecNodes.Contains(SourceNode))
				{
					continue;
				}

				int32& ExistingDepth = OutDepths.FindOrAdd(SourceNode, Depth);
				ExistingDepth = FMath::Min(ExistingDepth, Depth);
				CollectInputDependencies(SourceNode, SelectedNodes, ExecNodes, Depth + 1, OutDepths, TraversalStack);
			}
		}
		TraversalStack.Remove(Node);
	}

	static TArray<UEdGraphNode*> GetDirectInputDependenciesInPinOrder(const UEdGraphNode* Node, const TSet<UEdGraphNode*>& SelectedNodes, const TSet<UEdGraphNode*>& ExecNodes)
	{
		TArray<UEdGraphNode*> Dependencies;
		if (!Node)
		{
			return Dependencies;
		}

		for (const UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Input || IsExecPin(Pin))
			{
				continue;
			}

			for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				UEdGraphNode* SourceNode = LinkedPin ? LinkedPin->GetOwningNode() : nullptr;
				if (SelectedNodes.Contains(SourceNode) && !ExecNodes.Contains(SourceNode))
				{
					Dependencies.AddUnique(SourceNode);
				}
			}
		}

		return Dependencies;
	}

	static TMap<UEdGraphNode*, TArray<UEdGraphNode*>> AssignInputStacks(const TArray<UEdGraphNode*>& ExecNodeList, const TArray<UEdGraphNode*>& AllNodes, const TSet<UEdGraphNode*>& ExecNodes, const TMap<UEdGraphNode*, FExecPlacement>& ExecPlacements, TMap<UEdGraphNode*, int32>& OutInputDepths)
	{
		TSet<UEdGraphNode*> SelectedNodes(AllNodes);
		TSet<UEdGraphNode*> AssignedDataNodes;
		TMap<UEdGraphNode*, TArray<UEdGraphNode*>> OwnerToInputNodes;

		TArray<UEdGraphNode*> SortedExecNodes = ExecNodeList;
		SortedExecNodes.Sort([&ExecPlacements](const UEdGraphNode& Left, const UEdGraphNode& Right)
		{
			const FExecPlacement LeftPlacement = ExecPlacements.FindRef(const_cast<UEdGraphNode*>(&Left));
			const FExecPlacement RightPlacement = ExecPlacements.FindRef(const_cast<UEdGraphNode*>(&Right));
			return LeftPlacement.Row == RightPlacement.Row ? LeftPlacement.Column < RightPlacement.Column : LeftPlacement.Row < RightPlacement.Row;
		});

		for (UEdGraphNode* ExecNode : SortedExecNodes)
		{
			TMap<UEdGraphNode*, int32> CandidateDepths;
			TSet<UEdGraphNode*> TraversalStack;
			CollectInputDependencies(ExecNode, SelectedNodes, ExecNodes, 0, CandidateDepths, TraversalStack);

			CandidateDepths.ValueSort([](int32 LeftDepth, int32 RightDepth)
			{
				return LeftDepth < RightDepth;
			});

			for (const TPair<UEdGraphNode*, int32>& Candidate : CandidateDepths)
			{
				if (AssignedDataNodes.Contains(Candidate.Key))
				{
					continue;
				}

				OwnerToInputNodes.FindOrAdd(ExecNode).Add(Candidate.Key);
				OutInputDepths.Add(Candidate.Key, Candidate.Value);
				AssignedDataNodes.Add(Candidate.Key);
			}

			TArray<UEdGraphNode*>& InputNodes = OwnerToInputNodes.FindOrAdd(ExecNode);
			const TArray<UEdGraphNode*> DirectInputs = GetDirectInputDependenciesInPinOrder(ExecNode, SelectedNodes, ExecNodes);
			InputNodes.Sort([&DirectInputs, &OutInputDepths](const UEdGraphNode& Left, const UEdGraphNode& Right)
			{
				const int32 LeftDepth = OutInputDepths.FindRef(const_cast<UEdGraphNode*>(&Left));
				const int32 RightDepth = OutInputDepths.FindRef(const_cast<UEdGraphNode*>(&Right));
				if (LeftDepth != RightDepth)
				{
					return LeftDepth < RightDepth;
				}

				const int32 LeftDirectIndex = DirectInputs.IndexOfByKey(const_cast<UEdGraphNode*>(&Left));
				const int32 RightDirectIndex = DirectInputs.IndexOfByKey(const_cast<UEdGraphNode*>(&Right));
				if (LeftDirectIndex != INDEX_NONE || RightDirectIndex != INDEX_NONE)
				{
					return (LeftDirectIndex == INDEX_NONE ? MAX_int32 : LeftDirectIndex) < (RightDirectIndex == INDEX_NONE ? MAX_int32 : RightDirectIndex);
				}

				return Left.NodePosY == Right.NodePosY ? Left.NodePosX < Right.NodePosX : Left.NodePosY < Right.NodePosY;
			});
		}

		return OwnerToInputNodes;
	}

	static TMap<UEdGraphNode*, int32> ComputeLayers(const TArray<UEdGraphNode*>& Nodes)
	{
		TSet<UEdGraphNode*> NodeSet(Nodes);
		TSet<UEdGraphNode*> PlacedNodes;
		TMap<UEdGraphNode*, int32> Layers;
		for (UEdGraphNode* Node : Nodes)
		{
			Layers.Add(Node, 0);
		}

		while (PlacedNodes.Num() < Nodes.Num())
		{
			UEdGraphNode* BestNode = nullptr;
			for (UEdGraphNode* CandidateNode : Nodes)
			{
				if (PlacedNodes.Contains(CandidateNode))
				{
					continue;
				}

				bool bHasUnplacedInput = false;
				for (UEdGraphNode* PotentialSourceNode : Nodes)
				{
					if (!PlacedNodes.Contains(PotentialSourceNode) && PotentialSourceNode != CandidateNode && HasOutputLinkTo(PotentialSourceNode, CandidateNode))
					{
						bHasUnplacedInput = true;
						break;
					}
				}

				if (!bHasUnplacedInput)
				{
					BestNode = CandidateNode;
					break;
				}

				if (!BestNode || CandidateNode->NodePosX < BestNode->NodePosX || (CandidateNode->NodePosX == BestNode->NodePosX && CandidateNode->NodePosY < BestNode->NodePosY))
				{
					BestNode = CandidateNode;
				}
			}

			if (!BestNode)
			{
				break;
			}

			PlacedNodes.Add(BestNode);
			const int32 SourceLayer = Layers.FindRef(BestNode);
			for (UEdGraphNode* TargetNode : Nodes)
			{
				if (!NodeSet.Contains(TargetNode) || TargetNode == BestNode || !HasOutputLinkTo(BestNode, TargetNode))
				{
					continue;
				}

				int32& TargetLayer = Layers.FindOrAdd(TargetNode);
				TargetLayer = FMath::Max(TargetLayer, SourceLayer + 1);
			}
		}

		return Layers;
	}

	static int32 Snap(const int32 Value)
	{
		return FMath::GridSnap(Value, GridSnap);
	}

	static bool LayoutExecSpineWithInputStacks(const TArray<UEdGraphNode*>& Nodes, int32 MinX, int32 MinY)
	{
		TArray<UEdGraphNode*> ExecNodeList;
		TSet<UEdGraphNode*> ExecNodes;
		for (UEdGraphNode* Node : Nodes)
		{
			if (HasExecPin(Node))
			{
				ExecNodeList.Add(Node);
				ExecNodes.Add(Node);
			}
		}

		if (ExecNodeList.IsEmpty())
		{
			return false;
		}

		TMap<UEdGraphNode*, FExecPlacement> ExecPlacements = ComputeExecPlacements(ExecNodeList);
		TMap<UEdGraphNode*, int32> InputDepths;
		TMap<UEdGraphNode*, TArray<UEdGraphNode*>> OwnerToInputNodes = AssignInputStacks(ExecNodeList, Nodes, ExecNodes, ExecPlacements, InputDepths);

		TMap<int32, int32> RowStackDepth;
		for (const TPair<UEdGraphNode*, TArray<UEdGraphNode*>>& OwnerStack : OwnerToInputNodes)
		{
			const FExecPlacement Placement = ExecPlacements.FindRef(OwnerStack.Key);
			TMap<int32, int32> DepthCounts;
			for (UEdGraphNode* InputNode : OwnerStack.Value)
			{
				DepthCounts.FindOrAdd(InputDepths.FindRef(InputNode))++;
			}

			int32 MaxDepthCount = 0;
			for (const TPair<int32, int32>& DepthCount : DepthCounts)
			{
				MaxDepthCount = FMath::Max(MaxDepthCount, DepthCount.Value);
			}

			int32& ExistingMaxDepth = RowStackDepth.FindOrAdd(Placement.Row);
			ExistingMaxDepth = FMath::Max(ExistingMaxDepth, MaxDepthCount);
		}

		TArray<int32> Rows;
		for (const TPair<UEdGraphNode*, FExecPlacement>& Placement : ExecPlacements)
		{
			Rows.AddUnique(Placement.Value.Row);
		}
		Rows.Sort();

		TMap<int32, int32> RowBaseY;
		int32 RunningY = MinY;
		for (int32 Row : Rows)
		{
			RowBaseY.Add(Row, RunningY);
			RunningY += ExecRowSpacingY + RowStackDepth.FindRef(Row) * InputStackSpacingY;
		}

		for (UEdGraphNode* ExecNode : ExecNodeList)
		{
			const FExecPlacement Placement = ExecPlacements.FindRef(ExecNode);
			ExecNode->Modify();
			ExecNode->NodePosX = Snap(MinX + Placement.Column * ExecSpacingX);
			ExecNode->NodePosY = Snap(RowBaseY.FindRef(Placement.Row));
		}

		TSet<UEdGraphNode*> StackedInputNodes;
		for (const TPair<UEdGraphNode*, TArray<UEdGraphNode*>>& OwnerStack : OwnerToInputNodes)
		{
			const FExecPlacement OwnerPlacement = ExecPlacements.FindRef(OwnerStack.Key);
			const int32 OwnerX = Snap(MinX + OwnerPlacement.Column * ExecSpacingX);
			const int32 OwnerY = RowBaseY.FindRef(OwnerPlacement.Row);

			TMap<int32, int32> DepthCounts;
			TArray<UEdGraphNode*> InputNodes = OwnerStack.Value;
			InputNodes.Sort([&InputDepths](const UEdGraphNode& Left, const UEdGraphNode& Right)
			{
				const int32 LeftDepth = InputDepths.FindRef(const_cast<UEdGraphNode*>(&Left));
				const int32 RightDepth = InputDepths.FindRef(const_cast<UEdGraphNode*>(&Right));
				return LeftDepth == RightDepth ? Left.NodePosX < Right.NodePosX : LeftDepth < RightDepth;
			});

			for (UEdGraphNode* InputNode : InputNodes)
			{
				const int32 Depth = InputDepths.FindRef(InputNode);
				const int32 IndexAtDepth = DepthCounts.FindOrAdd(Depth)++;

				InputNode->Modify();
				InputNode->NodePosX = Snap(OwnerX - InputStackColumnBackX - Depth * InputStackDepthIndentX);
				InputNode->NodePosY = Snap(OwnerY + InputStackStartY + IndexAtDepth * InputStackSpacingY);
				StackedInputNodes.Add(InputNode);
			}
		}

		TArray<UEdGraphNode*> LooseNodes;
		for (UEdGraphNode* Node : Nodes)
		{
			if (!ExecNodes.Contains(Node) && !StackedInputNodes.Contains(Node))
			{
				LooseNodes.Add(Node);
			}
		}

		if (!LooseNodes.IsEmpty())
		{
			LooseNodes.Sort([](const UEdGraphNode& Left, const UEdGraphNode& Right)
			{
				return Left.NodePosY == Right.NodePosY ? Left.NodePosX < Right.NodePosX : Left.NodePosY < Right.NodePosY;
			});

			const int32 LooseStartY = Snap(RunningY + InputStackSpacingY);
			for (int32 Index = 0; Index < LooseNodes.Num(); ++Index)
			{
				LooseNodes[Index]->Modify();
				LooseNodes[Index]->NodePosX = Snap(MinX + Index * InputStackSpacingX);
				LooseNodes[Index]->NodePosY = LooseStartY;
			}
		}

		return true;
	}

	static void ExpandCommentSelection(TArray<UEdGraphNode*>& LayoutNodes, const TArray<UEdGraphNode_Comment*>& CommentNodes, const UEdGraph* Graph)
	{
		if (!LayoutNodes.IsEmpty() || CommentNodes.IsEmpty() || !Graph)
		{
			return;
		}

		for (const UEdGraphNode_Comment* Comment : CommentNodes)
		{
			for (UObject* ObjectUnderComment : Comment->GetNodesUnderComment())
			{
				UEdGraphNode* NodeUnderComment = Cast<UEdGraphNode>(ObjectUnderComment);
				if (IsRealGraphNode(NodeUnderComment, Graph) && !IsCommentNode(NodeUnderComment))
				{
					LayoutNodes.AddUnique(NodeUnderComment);
				}
			}

			for (const TObjectPtr<UEdGraphNode>& GraphNodeObject : Graph->Nodes)
			{
				UEdGraphNode* GraphNode = GraphNodeObject.Get();
				if (IsRealGraphNode(GraphNode, Graph) && !IsCommentNode(GraphNode) && IsNodeInsideComment(GraphNode, Comment))
				{
					LayoutNodes.AddUnique(GraphNode);
				}
			}
		}
	}

	static TArray<FCommentWrapInfo> BuildCommentWrapInfos(const TArray<UEdGraphNode_Comment*>& CommentNodes, const TArray<UEdGraphNode*>& LayoutNodes)
	{
		TArray<FCommentWrapInfo> WrapInfos;
		for (UEdGraphNode_Comment* Comment : CommentNodes)
		{
			FCommentWrapInfo WrapInfo;
			WrapInfo.Comment = Comment;
			for (UEdGraphNode* Node : LayoutNodes)
			{
				if (!Node || Node == Comment)
				{
					continue;
				}

				bool bBelongsToComment = IsNodeInsideComment(Node, Comment);
				if (!bBelongsToComment)
				{
					for (UObject* ObjectUnderComment : Comment->GetNodesUnderComment())
					{
						if (ObjectUnderComment == Node)
						{
							bBelongsToComment = true;
							break;
						}
					}
				}

				if (bBelongsToComment)
				{
					WrapInfo.ContainedNodes.Add(Node);
				}
			}

			if (WrapInfo.ContainedNodes.IsEmpty() && CommentNodes.Num() == 1)
			{
				WrapInfo.ContainedNodes = LayoutNodes;
			}

			if (!WrapInfo.ContainedNodes.IsEmpty())
			{
				WrapInfos.Add(WrapInfo);
			}
		}

		return WrapInfos;
	}

	static void WrapSelectedComments(const TArray<FCommentWrapInfo>& WrapInfos)
	{
		for (const FCommentWrapInfo& WrapInfo : WrapInfos)
		{
			if (!WrapInfo.Comment || WrapInfo.ContainedNodes.IsEmpty())
			{
				continue;
			}

			int32 MinX = WrapInfo.ContainedNodes[0]->NodePosX;
			int32 MinY = WrapInfo.ContainedNodes[0]->NodePosY;
			int32 MaxX = WrapInfo.ContainedNodes[0]->NodePosX + GetEstimatedNodeWidth(WrapInfo.ContainedNodes[0]);
			int32 MaxY = WrapInfo.ContainedNodes[0]->NodePosY + GetEstimatedNodeHeight(WrapInfo.ContainedNodes[0]);
			for (const UEdGraphNode* Node : WrapInfo.ContainedNodes)
			{
				MinX = FMath::Min(MinX, Node->NodePosX);
				MinY = FMath::Min(MinY, Node->NodePosY);
				MaxX = FMath::Max(MaxX, Node->NodePosX + GetEstimatedNodeWidth(Node));
				MaxY = FMath::Max(MaxY, Node->NodePosY + GetEstimatedNodeHeight(Node));
			}

			WrapInfo.Comment->Modify();
			WrapInfo.Comment->SetBounds(FSlateRect(
				Snap(MinX - CommentPadding),
				Snap(MinY - CommentPadding),
				Snap(MaxX + CommentPadding),
				Snap(MaxY + CommentPadding)));
		}
	}
}

void FBlueprintAutoLayoutModule::StartupModule()
{
	FGraphEditorModule& GraphEditorModule = FModuleManager::LoadModuleChecked<FGraphEditorModule>("GraphEditor");
	GraphContextMenuExtenderHandle = GraphEditorModule.GetAllGraphEditorContextMenuExtender().Add(
		FGraphEditorModule::FGraphEditorMenuExtender_SelectedNode::CreateRaw(this, &FBlueprintAutoLayoutModule::ExtendGraphContextMenu));
}

void FBlueprintAutoLayoutModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("GraphEditor") && GraphContextMenuExtenderHandle != INDEX_NONE)
	{
		FGraphEditorModule& GraphEditorModule = FModuleManager::GetModuleChecked<FGraphEditorModule>("GraphEditor");
		GraphEditorModule.GetAllGraphEditorContextMenuExtender().RemoveAt(GraphContextMenuExtenderHandle);
		GraphContextMenuExtenderHandle = INDEX_NONE;
	}
}

TSharedRef<FExtender> FBlueprintAutoLayoutModule::ExtendGraphContextMenu(const TSharedRef<FUICommandList> CommandList, const UEdGraph* Graph, const UEdGraphNode* Node, const UEdGraphPin* Pin, bool bIsDebugging)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();
	if (!bIsDebugging && Graph && Node && !Pin)
	{
		Extender->AddMenuExtension(
			"EdGraphSchemaOrganization",
			EExtensionHook::After,
			CommandList,
			FMenuExtensionDelegate::CreateRaw(this, &FBlueprintAutoLayoutModule::AddGraphContextMenuEntry, TWeakObjectPtr<const UEdGraph>(Graph), TWeakObjectPtr<const UEdGraphNode>(Node)));
	}

	return Extender;
}

void FBlueprintAutoLayoutModule::AddGraphContextMenuEntry(FMenuBuilder& MenuBuilder, TWeakObjectPtr<const UEdGraph> Graph, TWeakObjectPtr<const UEdGraphNode> Node)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("OrganizeSelectedNodes", "Organize Selected Nodes"),
		LOCTEXT("OrganizeSelectedNodesTooltip", "Arranges selected Blueprint nodes as an exec spine with compact input stacks underneath each exec node."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FBlueprintAutoLayoutModule::OrganizeSelection, Graph, Node)));
}

void FBlueprintAutoLayoutModule::OrganizeSelection(TWeakObjectPtr<const UEdGraph> Graph, TWeakObjectPtr<const UEdGraphNode> ContextNode) const
{
	const UEdGraph* ConstGraph = Graph.Get();
	if (!ConstGraph)
	{
		return;
	}

	UEdGraph* MutableGraph = const_cast<UEdGraph*>(ConstGraph);
	TArray<UEdGraphNode*> Nodes = BlueprintAutoLayout::GetNodesToOrganize(ConstGraph, ContextNode.Get());
	if (Nodes.IsEmpty())
	{
		return;
	}

	TArray<UEdGraphNode*> LayoutNodes;
	TArray<UEdGraphNode_Comment*> CommentNodes;
	for (UEdGraphNode* Node : Nodes)
	{
		if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(Node))
		{
			CommentNodes.Add(CommentNode);
		}
		else
		{
			LayoutNodes.Add(Node);
		}
	}

	BlueprintAutoLayout::ExpandCommentSelection(LayoutNodes, CommentNodes, ConstGraph);
	if (LayoutNodes.IsEmpty())
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("OrganizeBlueprintGraphNodesTransaction", "Organize Blueprint Graph Nodes"));
	MutableGraph->Modify();
	const TArray<BlueprintAutoLayout::FCommentWrapInfo> CommentWrapInfos = BlueprintAutoLayout::BuildCommentWrapInfos(CommentNodes, LayoutNodes);

	int32 MinX = LayoutNodes[0]->NodePosX;
	int32 MinY = LayoutNodes[0]->NodePosY;
	for (const UEdGraphNode* Node : LayoutNodes)
	{
		MinX = FMath::Min(MinX, Node->NodePosX);
		MinY = FMath::Min(MinY, Node->NodePosY);
	}

	if (!BlueprintAutoLayout::LayoutExecSpineWithInputStacks(LayoutNodes, MinX, MinY))
	{
		TMap<UEdGraphNode*, int32> Layers = BlueprintAutoLayout::ComputeLayers(LayoutNodes);
		LayoutNodes.Sort([&Layers](const UEdGraphNode& Left, const UEdGraphNode& Right)
		{
			const int32 LeftLayer = Layers.FindRef(const_cast<UEdGraphNode*>(&Left));
			const int32 RightLayer = Layers.FindRef(const_cast<UEdGraphNode*>(&Right));
			if (LeftLayer != RightLayer)
			{
				return LeftLayer < RightLayer;
			}

			if (Left.NodePosY != Right.NodePosY)
			{
				return Left.NodePosY < Right.NodePosY;
			}

			return Left.NodePosX < Right.NodePosX;
		});

		TMap<int32, int32> LayerCounts;
		for (UEdGraphNode* Node : LayoutNodes)
		{
			const int32 Layer = Layers.FindRef(Node);
			const int32 Row = LayerCounts.FindOrAdd(Layer)++;

			Node->Modify();
			Node->NodePosX = BlueprintAutoLayout::Snap(MinX + Layer * BlueprintAutoLayout::HorizontalSpacing);
			Node->NodePosY = BlueprintAutoLayout::Snap(MinY + Row * BlueprintAutoLayout::VerticalSpacing);
		}
	}
	BlueprintAutoLayout::WrapSelectedComments(CommentWrapInfos);

	MutableGraph->NotifyGraphChanged();
	MutableGraph->MarkPackageDirty();

	if (TSharedPtr<SGraphEditor> GraphEditor = SGraphEditor::FindGraphEditorForGraph(ConstGraph))
	{
		GraphEditor->NotifyGraphChanged();
		GraphEditor->ZoomToFit(true);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlueprintAutoLayoutModule, BlueprintAutoLayout)
