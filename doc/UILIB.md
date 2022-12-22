# UI system

This document is a collection of ideas and may not always be accurate/up to date.

Current state of things is very messy, because I tried to just make it compatible to packs.

## Concepts
- Each window has its own drawing context (`Renderer`)
- Widgets may be bound to the drawing context they first drew to
- Widgets are implemented by overriding common methods
- Widgets should not have to know what their parents are
  - we have one problem with tooltips and a dirty work-around
- `addChild` transfers ownership to container
- `removeChild` transfers ownership to caller
- `setSize()` should auto-size all children. Value given may be modified if too small.
- a layout step should generate all `MinSize`s, update actual size and emit `Signal`s when modifing any portion of the tree - this is incomplete

## UI main class
- `createWindow`: creates a window and adds it to the list of event receivers
- `render`: receives/handles/propagates events and calls `render` on all windows

## Widgets
- `Window` inherits `Container`
- `HBox` inherits `Container`: Horizontally oriented children
- `VBox` inherits `Container`: Vertically oriented children
- `Table` or `Grid` inherits `Container`: X x Y (equally sized) children
- `SimpleContainer` inherits `Container`: 1 full-size child (or managed by parent)
- `Tooltip` inherits `SimpleContanier`: styled background and border
- `Group` inherits `VBox`: 1 child + title
- `Dock` inserhits `Container`: children can be docked left, right, top bottom or be floating in the remaining space
- `Container` inherits `Widget`
- `Label` inherits `Widget`
- `LineEdit` inherits `Widget` or `TextEdit`?
- `TextEdit` inherits `Widget`
- `StagedImageButton` ? inherits `Widget`: scrolls through states with left/right mous, e.g. value or enable/disable
- `DoubleStagedImageButton` ? inherits `Widget`: scrolls through one state with left, other with right mouse, e.g. value + enable/disable
- `CountImageButton` ?

## Layout system
This needs a lot of work still.

- Original: not really clear, we try to implement widgets in a compatible manner
- Future: auto-size in both directions, see Concepts and TODO.md
- Hints/Values:
  - `size`: always, actual size that was decided on
  - manual `minSize`: future
  - autom. `minSize`: always (required for docks/hboxes)
  - manual `maxSize`: always (selectively implemented with hacks at the moment)
  - autom. `maxSize`: future (required to transfer manual maxSize to parents)
  - `autoSize`: always, what size (or aspect ratio) the actual content has
  - `preferredSize`: future (lets child tell the size it wants)
  - `hGrow`: future; original has `"h_alignment": "stretch"` though
  - `vGrow`: future; original has `"v_alignment": "stretch"` though
  - `hGravity`: future; original has `"h_alignment"` though
  - `vGravity`: future; original has `"v_alignment"` though

## Common widget stuff
- tooltip: any widget/container
- background: image or color (or none/transparent)
- keepAspect: in relation to autoSize; TODO: in relation to preferredSize?

## Events
Using `class Signal`, we can "subscribe" lambdas to:
- `onMouseEnter (sender, position, Button, Modifiers)`
- `onMouseLeave (sender, position, Button, Modifiers)`
- `onClick (sender, position, Button, Modifiers)`
- `onBeforeChange (sender)`
- `onAfterChange (sender)`

Fixed member functions, only propagating from window to widgets:
- `ParentResized` (for future layout system)

## Checks to Widget mapping
Currently we have a generic UI::Item, which requires conversion of eveything to
stages (individual images).
