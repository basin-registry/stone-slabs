# 3D Drawing

Shape constructors for canvas3d rendering. All take a single record argument.

```stone
import box, sphere, cylinder, cone, torus, mesh, line3d, polyline3d, pointcloud, text, group from draw3d
```

`box({ center = [x,y,z], size = [w,h,d], rotation = [rx,ry,rz]? })`

`sphere({ center = [x,y,z], radius = num })`

`cylinder({ start = [x,y,z], end = [x,y,z], radius = num })` -- uses start/end, NOT center/height

`cone({ start = [x,y,z], end = [x,y,z], radius = num })` -- uses start/end, NOT center/height

`torus({ center = [x,y,z], major_radius = num, minor_radius = num, axis = [x,y,z]? })`

`mesh({ vertices = [[x,y,z], ...], indices = [[a,b,c], ...] })` -- normals auto-computed if omitted. Indices can also be flat: `[a, b, c, d, e, f, ...]`.

`line3d({ start = [x,y,z], end = [x,y,z] })`

`polyline3d({ points = [[x,y,z], ...], closed = bool? })`

`pointcloud({ points = [[x,y,z], ...], point_size = num? })`

`text({ position = [x,y,z], text = string })`

## Material

```stone
sphere({
    center = [0, 0, 0], radius = 1,
    material = { color = "red", metalness = 0.3, roughness = 0.7, opacity = 0.9 }
})
```

Properties: `color`, `opacity`, `metalness` (0-1), `roughness` (0-1), `wireframe`, `emissive`, `flatShading`.

## Groups

```stone
group({
    objects = [shape1, shape2],
    position = [x, y, z],
    rotation = [rx, ry, rz],
    scale = 1,
    anchor = [0, 0, 0]
})
```

Groups nest. Each child group's transform is relative to its parent, creating hierarchies for articulated assemblies.

## API Reference

**Shapes:**

| Function | Description |
|----------|-------------|
| `box(p: Box)` | Create box from params record |
| `cube(size: num)` | Create cube (box with equal dimensions) |
| `sphere(p: Sphere)` | Create sphere from params record |
| `cylinder(p: Cylinder)` | Create cylinder from params record |
| `cone(p: Cone)` | Create cone from params record |
| `torus(p: Torus)` | Create torus from params record |
| `line3d(p: Line3D)` | Create 3D line from params record |
| `polyline3d(p: Polyline3D)` | Create 3D polyline from params record |
| `mesh(p: Mesh)` | Create mesh from params record |
| `pointcloud(p: PointCloud)` | Create pointcloud from params record |
| `group(p: Group3D)` | Create group from params record |

**Queries & Transforms:**

| Function | Description |
|----------|-------------|
| `bounds(shape)` | Axis-aligned bounding box |
| `volume(shape)` | Volume |
| `surface_area(shape)` | Surface area |
| `center(shape)` | Get center point |
| `translate(shape, offset)` | Move center |
| `rotate_x(shape, angle, pivot = [0, 0, 0])` | Rotate around X axis |
| `rotate_y(shape, angle, pivot = [0, 0, 0])` | Rotate around Y axis |
| `rotate_z(shape, angle, pivot = [0, 0, 0])` | Rotate around Z axis |
| `rotate(shape, angles)` | Add to rotation angles |
| `scale(shape, factor, pivot = [0, 0, 0])` | Scale from pivot |

**Types:** `Bounds3`, `Material3D`, `CadMaterial`, `LineMaterial`, `Box`, `Sphere`, `Cylinder`, `Cone`, `Torus`, `Mesh`, `Line3D`, `Polyline3D`, `PointCloud`, `Group3D`, `Shape3D`
