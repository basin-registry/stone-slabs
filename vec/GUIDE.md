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

## API Reference

### vec2

```stone
import ... from vec2
```

| Function | Description |
|----------|-------------|
| `add(a, b)` | Vector addition |
| `sub(a, b)` | Vector subtraction |
| `mul(v, s)` | Scalar multiplication |
| `div(v, s)` | Scalar division |
| `neg(v)` | Negation |
| `mul_comp(a, b)` | Component-wise multiplication |
| `div_comp(a, b)` | Component-wise division |
| `dot(a, b)` | Dot product |
| `cross(a, b)` | 2D cross product (returns scalar: z-component of 3D cross) |
| `magnitude(v)` | Vector length (magnitude) |
| `len_sq(v)` | Squared length |
| `dist(a, b)` | Distance between two points |
| `dist_sq(a, b)` | Squared distance |
| `normalize(v)` | Normalize to unit vector |
| `set_len(v, length)` | Set vector to specific length |
| `angle(v)` | Angle of vector from positive X axis (radians) |
| `angle_between(a, b)` | Signed angle between two vectors (radians) |
| `rotate(v, ang)` | Rotate vector by angle (radians) |
| `perp(v)` | Perpendicular vector (90 counter-clockwise) |
| `perp_cw(v)` | Perpendicular vector (90 clockwise) |
| `lerp(a, b, t)` | Linear interpolation between two vectors |
| `reflect(v, normal)` | Reflect vector off surface with given normal |
| `project(v, onto)` | Project vector onto another vector |
| `reject(v, basis)` | Reject vector from another (perpendicular component) |
| `eq(a, b, epsilon = 1e-10)` | Approximate equality with epsilon |
| `is_zero(v, epsilon = 1e-10)` | Check if vector is zero (within epsilon) |
| `min_comp(v)` | Min component |
| `max_comp(v)` | Max component |
| `min_vec(a, b)` | Component-wise min of two vectors |
| `max_vec(a, b)` | Component-wise max of two vectors |

**Constants:** `ZERO`, `ONE`, `UNIT_X`, `UNIT_Y`

### vec3

```stone
import ... from vec3
```

| Function | Description |
|----------|-------------|
| `add(a, b)` | Vector addition |
| `sub(a, b)` | Vector subtraction |
| `mul(v, s)` | Scalar multiplication |
| `div(v, s)` | Scalar division |
| `neg(v)` | Negation |
| `mul_comp(a, b)` | Component-wise multiplication |
| `div_comp(a, b)` | Component-wise division |
| `dot(a, b)` | Dot product |
| `cross(a, b)` | Cross product |
| `magnitude(v)` | Vector length (magnitude) |
| `len_sq(v)` | Squared length |
| `dist(a, b)` | Distance between two points |
| `dist_sq(a, b)` | Squared distance |
| `normalize(v)` | Normalize to unit vector |
| `set_len(v, length)` | Set vector to specific length |
| `rotate_x(v, angle)` | Rotate around X axis |
| `rotate_y(v, angle)` | Rotate around Y axis |
| `rotate_z(v, angle)` | Rotate around Z axis |
| `rotate_around(v, axis, angle)` | Rotate around arbitrary axis (Rodrigues' rotation formula) |
| `lerp(a, b, t)` | Linear interpolation between two vectors |
| `reflect(v, normal)` | Reflect vector off surface with given normal |
| `project(v, onto)` | Project vector onto another vector |
| `reject(v, basis)` | Reject vector from another (perpendicular component) |
| `project_on_plane(pt, plane_pt, normal)` | Project point onto plane defined by point and normal |
| `eq(a, b, epsilon = 1e-10)` | Approximate equality with epsilon |
| `is_zero(v, epsilon = 1e-10)` | Check if vector is zero (within epsilon) |
| `min_comp(v)` | Min component |
| `max_comp(v)` | Max component |
| `min_vec(a, b)` | Component-wise min of two vectors |
| `max_vec(a, b)` | Component-wise max of two vectors |

**Constants:** `ZERO`, `ONE`, `UNIT_X`, `UNIT_Y`, `UNIT_Z`
