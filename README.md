# BlueprintAutoLayout

BlueprintAutoLayout is a lightweight Unreal Engine editor plugin that adds a single smart right-click command for organizing Blueprint graph selections.

The goal is not to make every graph mathematically tidy. The goal is to make Blueprint graphs feel more like a careful human arranged them: compact exec flow, readable input preparation, and fewer huge horizontal sprawls.

## What It Does

- Adds **Organize Selected Nodes** to Blueprint graph node right-click menus.
- Builds a compact left-to-right exec spine from selected white exec pins.
- Places direct data/input nodes in the previous column, gently stacked by the owning node's input pin order.
- Pushes deeper input dependencies farther left so data flows into the owner without widening the main exec chain.
- Moves branch/secondary exec paths onto lower rows.
- Reserves vertical row space for input stacks before placing lower exec rows.
- Keeps a generic layered fallback for selections without Blueprint exec pins.
- Supports undo through Unreal's transaction system.

## Usage

1. Enable the plugin in your Unreal project.
2. Open a Blueprint graph.
3. Select the nodes you want to organize.
4. Right-click a selected graph node.
5. Choose **Organize Selected Nodes**.

The command is intentionally one smart mode with fallbacks, rather than a menu full of layout variants.

## Layout Methodology

BlueprintAutoLayout treats Blueprint graphs as two related structures:

- **Exec flow**: the primary horizontal story of the graph.
- **Input preparation**: the supporting data chains that feed each exec node.

The organizer first finds exec nodes in the selection and lays them out as a spine. For each exec node, it walks upstream through non-exec input pins and assigns those input nodes to the exec node they feed.

Direct input nodes are placed in the previous column, stacked vertically in pin order. If those input nodes have their own dependencies, those dependencies step farther left. This creates a compact shape like:

```text
                  [Exec Owner]
    [Input A]  -->     |
    [Input B]  -->     |

[Prep] -> [Input C] -->|
```

For branches and secondary exec outputs, the first path continues on the current row and additional paths are placed on lower rows. Row spacing accounts for the tallest input stack in that row.

## Installation

Copy the plugin folder into your Unreal project:

```text
YourProject/
  Plugins/
    BlueprintAutoLayout/
      BlueprintAutoLayout.uplugin
      Source/
```

Then regenerate project files if needed and build your editor target.

For this repository, the plugin lives at:

```text
Plugins/BlueprintAutoLayout
```

## Requirements

- Unreal Engine 5.7 was used during development.
- The plugin is editor-only.
- No runtime module or packaged-game footprint.
- No external layout library dependency.

## Current Limitations

- Node sizing is estimated for some graph node types when Unreal does not expose cached Slate bounds.
- Comment boxes are supported only when selected; selected comments re-wrap around the organized nodes they contain.
- Very unusual graphs with cycles, latent control-flow tricks, or partially selected dependency chains may fall back to a simpler readable arrangement.

## Development Notes

The implementation is deliberately small and native:

- `FGraphEditorModule` context menu extender for the right-click action.
- K2 exec pin detection through `UEdGraphSchema_K2::PC_Exec`.
- Scoped transactions for undo.
- A smart layout path followed by a generic fallback path.

The most important tuning constants are near the top of:

```text
Plugins/BlueprintAutoLayout/Source/BlueprintAutoLayout/Private/BlueprintAutoLayoutModule.cpp
```

## Status

Experimental, but usable. The plugin is meant to evolve by trying it on real messy Blueprint graphs and tuning the spacing/ownership rules until the results feel natural.
