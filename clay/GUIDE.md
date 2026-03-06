# Clay - Parametric Solid Modeling

Clay is Stone's immutable CAD modeling library. It provides 3D solid primitives, boolean operations, profile-based extrusion, a constraint-based sketch system, and an intent-based query system for selecting geometry.

All operations are immutable: they return new Features, never modify inputs.

```stone
import box, cylinder, subtract, fillet, edges, faces from clay
import XY_PLANE, sketch, extrude, rect, circle from clay
```

## Core Types

| Type | Description |
|------|-------------|
| `Feature` | Result of CAD operations. Contains a `body` (solid geometry) and `selections` (named face/edge queries) |
| `Body` | Opaque handle to solid geometry in the kernel |
| `Profile` | Closed 2D region ready for extrusion |
| `Sketch` | 2D drawing on a plane, with geometry entities and constraints |
| `Query` | First-class selection value (faces, edges, or vertices) |
| `Plane` | Defined by `origin`, `normal`, and `x_dir` vectors |

**Standard planes:** `XY_PLANE`, `XZ_PLANE`, `YZ_PLANE`

## 3D Primitives

All primitives return a `Feature` and accept a single record argument:

```stone
import box, cylinder, sphere, cone, torus, cube from clay

base = box({size = [100, 50, 10]})
pin = cylinder({radius = 5, height = 20, center = [25, 0, 5]})
ball = sphere({radius = 10, center = [0, 0, 15]})
tip = cone({radius1 = 8, radius2 = 0, height = 15})
ring = torus({major_radius = 20, minor_radius = 3})
block = cube(10)  // shorthand for box with equal sides
```

**box:** `{ size = [w, h, d], center = [x,y,z]? }`
**cylinder:** `{ radius, height, center = [x,y,z]?, axis = [x,y,z]? }`
**sphere:** `{ radius, center = [x,y,z]? }`
**cone:** `{ radius1, radius2 = 0, height, center = [x,y,z]?, axis = [x,y,z]? }`
**torus:** `{ major_radius, minor_radius, center = [x,y,z]?, axis = [x,y,z]? }`
**cube:** `cube(size)` — shorthand for box with equal dimensions

## Boolean Operations

Combine or cut solids:

```stone
base = box({size = [100, 50, 10]})
hole = cylinder({radius = 5, height = 12, center = [25, 0, 5]})

result = subtract(base, hole)   // cut hole from base
combined = union(base, hole)    // merge both solids
common = intersect(base, hole)  // keep only overlap
```

- `union(target, tool)` — combine two solids
- `subtract(target, tool)` — cut tool from target (alias: `cut`)
- `intersect(target, tool)` — keep only common volume
- `add(target, tool)` — alias for union

Pipe syntax: `result = base |> subtract(hole)`

## Feature Operations

```stone
import box, fillet, chamfer, shell, edges, faces from clay

base = box({size = [100, 50, 10]})
rounded = fillet(base, {edges = edges(base), radius = 2})
chamfered = chamfer(base, {edges = edges(base), distance = 1.5})
hollowed = shell(base, {faces = faces(base) |> facing_up, thickness = 2})
```

- `fillet(target, {edges, radius})` — round selected edges
- `chamfer(target, {edges, distance, angle = 45})` — angled cut on edges
- `shell(target, {faces, thickness})` — hollow solid, removing specified faces

## 2D Profiles

Create closed 2D regions for extrusion. Profile booleans work the same as 3D:

```stone
plate = rect({width = 100, height = 60})
hole = circle({radius = 5, center = [40, 20]})
profile = plate |> subtract(hole)
```

**rect:** `{ width, height, center = [x,y]? }`
**circle:** `{ radius, center = [x,y]? }`
**ellipse:** `{ rx, ry, center = [x,y]?, rotation = 0 }`
**polygon:** `{ points = [[x,y], ...] }` or `{ sides, radius, center = [x,y]? }`
**slot:** `{ start = [x,y], end = [x,y], width }`

## Sketch + Extrude

```stone
profile = rect({width = 100, height = 60})
    |> subtract(circle({radius = 5, center = [40, 20]}))

feature = sketch(XY_PLANE, profile) |> extrude(10)
```

Full extrude spec: `extrude(base, { profile, distance, direction = [0,0,1], symmetric = true, draft_angle = 5, operation = "cut" })`

## Revolve

```stone
profile = rect({width = 10, height = 30, center = [20, 15]})
vase = revolve(sketch(XZ_PLANE, profile) |> extrude(1), {axis_direction = [0, 0, 1], angle = 360})
```

## Transforms

All return new Features:

- `translate(target, [dx, dy, dz])` — move geometry (alias: `move`)
- `rotate(target, {axis, angle, center?})` — rotate in degrees
- `rotate_x/y/z(target, angle, center?)` — axis shortcuts
- `scale(target, {factor, center?})` — uniform or per-axis scale
- `mirror(target, {plane, keep_original?})` — mirror across plane
- `mirror_x/y/z(target, keep_original?)` — axis plane shortcuts

### Patterns

```stone
row = linear_pattern(pin, {direction = [1, 0, 0], count = 5, spacing = 15})
ring = circular_pattern(pin, {axis = [0, 0, 1], count = 8, angle = 360})
```

## Query System

Queries select faces, edges, or vertices. They describe *how* to select geometry, making models robust to changes.

```stone
top = faces(base) |> facing_up
outer = edges(base) |> convex_edges
rounded = fillet(base, {edges = outer, radius = 2})
```

Constructors: `faces(feature)`, `edges(feature)`, `vertices(feature)`, `edges_of(face_query)`, `faces_of(edge_query)`

Filters: `facing(query, direction)`, `facing_up`/`facing_down`, `at_z(query, z)`, `planar_faces`, `cylindrical_faces`, `convex_edges`, `concave_edges`, `outer_edges`, `inner_edges`, `along_axis(query, axis)`

Set ops: `combine(a, b)`, `overlap(a, b)`, `exclude(a, b)`

Utilities: `count(query)`, `is_empty(query)`, `first(query)`, `last(query)`

## Constraint-Based Sketching

```stone
sk = sketch(XY_PLANE)
    |> add_rect({width = 100, height = 60, center = [0, 0]})
    |> add_circle({radius = 10, center = [30, 0]})
    |> solve

feature = profile(sk) |> extrude(10)
```

Sketch ops: `sketch(plane?)`, `sketch_on(face_query)`, `add_rect`/`add_circle`/`add_line`/`add_arc`/`add_polygon`, `constrain`, `constrain_all`, `solve`, `close`, `profile(sketch)`, `profiles(sketch)`

Constraints: `coincident`, `parallel`, `perpendicular`, `horizontal`, `vertical`, `fixed`, `equal`, `tangent`, `concentric`, `distance`, `angle`

## Visualization

```stone
import get_brep from clay
import canvas3d, show from display

show(canvas3d({ axes = true, objects = [get_brep(result)] }))
```

Use draw3d `group()` for render-time transforms (free) instead of clay `rotate()`/`translate()` (expensive CSG). Build each part once with `get_brep()`, then use groups to position and animate.
