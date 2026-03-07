# Plot

Data plotting with graph terminals. Import from `plot`.

```stone
import graph2d, graph3d, line, scatter, bar, surface, show from plot
```

## graph2d

```stone
show(graph2d({
    title = "My Plot",
    x_label = "x", y_label = "y",
    plots = [
        line({ x = xs, y = ys }),
        scatter({ x = xs, y = ys })
    ]
}))
```

Configuration: `title`, `x_label`, `y_label`, `x_scale`/`y_scale` ("linear" or "log"), `x_min`, `x_max`, `y_min`, `y_max`, `grid`, `legend`, `domains`.

2D plot constructors:

```stone
line({ x = xs, y = ys })
line({ points = [[0, 1], [1, 2]] })
scatter({ x = xs, y = ys })
bar({ labels = ["A", "B", "C"], values = [10, 20, 30] })
heatmap({ z = data, x = x_vals, y = y_vals })
contour({ z = data })
area({ x = xs, y = ys })
```

Style: `line({ x = xs, y = ys, style = { color = "red", line_width = 3 } })`

## graph3d

```stone
show(graph3d({
    title = "Scene",
    background = "light",
    plots = [surface({ z = z_data, x = x_vals, y = y_vals })]
}))
```

Coordinate system: Z is up. Configuration: `title`, `background` ("light" or "dark"), `axes`, `grid`, `camera_x/y/z`, `target_x/y/z`, `domains`.

3D plot constructors:

```stone
surface({ z = z_data, x = x_vals, y = y_vals })
line({ x = xs, y = ys, z = zs })
scatter({ x = xs, y = ys, z = zs })
sphere({ center = { x = 0, y = 0, z = 0 }, radius = 1.0 })
box({ center = { x = 2, y = 0, z = 0 }, size = { x = 1, y = 1, z = 1 } })
text({ position = { x = 0, y = 0, z = 2 }, text = "Label" })
axes({})
```

## Subplots

Use the `panel()` constructor from `display` to arrange multiple graphs:

```stone
import panel, show from display

show(panel({
    title = "Dashboard",
    layout = "row",
    displays = [
        graph2d({ title = "Left", plots = [...] }),
        graph2d({ title = "Right", plots = [...] })
    ]
}))
```

## API Reference

**Types:**

| Type | Description |
|------|-------------|
| `Point2D` | `{x: num, y: num}` - 2D point record |
| `Point3D` | `{x: num, y: num, z: num}` - 3D point record |

**2D Plot Constructors:**

All 2D constructors accept an optional `style: object` parameter as the last argument.

| Function | Description |
|----------|-------------|
| `line(data: LineData)` | create 2D line plot. `line({x, y, z})` for 3D (auto-detected). `line({points})` from [[x,y], ...] |
| `scatter(data: ScatterData)` | create 2D scatter plot. `scatter({x, y, z})` for 3D. `scatter({points})` from [[x,y], ...] |
| `bar(data: BarData)` | create bar chart |
| `area(data: AreaData)` | `area({x, y})` or `area({points})` |
| `heatmap(data: HeatmapData)` | create heatmap. `heatmap({z, x, y})` with custom axes |
| `contour(data: ContourData)` | create contour plot. `contour({z, x, y})` with custom axes |

**3D Plot Constructors:**

| Function | Description |
|----------|-------------|
| `surface(data: SurfaceData)` | create surface. `surface({z, x, y})` with custom axes |
| `text(data: TextData)` | create 3D text label |
| `axes(data: AxesData)` | `axes({})` or `axes({size})` -- XYZ axis helper |

Re-exports `graph2d`, `graph3d`, `show`, `vary`, `reveal` from `display`.
