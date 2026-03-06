# Vector Math

2D and 3D vector operations on array-based vectors `[x, y]` and `[x, y, z]`.

```stone
import vec2_add, vec2_dot, vec2_normalize from vec
import vec3_cross, vec3_normalize from vec
```

Or import from submodules with short names:

```stone
import add, dot, magnitude, normalize from vec/vec2
import cross, normalize from vec/vec3
```

## vec2 Operations

`add`, `sub`, `mul` (scalar), `dot`, `cross` (scalar result), `magnitude`, `normalize`, `distance`, `angle`, `rotate`, `lerp`, `project`, `reflect`, `perpendicular`

## vec3 Operations

`add`, `sub`, `mul` (scalar), `dot`, `cross` (vector result), `magnitude`, `normalize`, `distance`, `angle`, `lerp`, `project`, `reflect`, `rotate_x`, `rotate_y`, `rotate_z`
