# 2D Drawing

Shape constructors for canvas2d rendering. All take a single record argument.

```stone
import circle, rect, ellipse, line, arc, polygon, polyline, path, text, group from draw2d
```

`circle({ center = [x, y], radius = num })`

`rect({ center = [x, y], width = num, height = num, rotation = num? })`

`ellipse({ center = [x, y], rx = num, ry = num, rotation = num? })`

`line({ start = [x, y], end = [x, y] })`

`arc({ center = [x, y], radius = num, start_angle = num, end_angle = num })`

`polygon({ points = [[x, y], ...], closed = bool? })`

`polyline({ points = [[x, y], ...] })`

`text({ position = [x, y], text = string, font_size = num? })`

`path({ commands = [...] })` where commands are `move_to(x, y)`, `line_to(x, y)`, `quad_to(cx, cy, x, y)`, `cubic_to(c1x, c1y, c2x, c2y, x, y)`, `arc_to(rx, ry, rotation, large_arc, sweep, x, y)`, `close_path`.

## Style

```stone
circle({
    center = [0, 0], radius = 50,
    style = { fill = "red", stroke = "slate", stroke_width = 2, fill_opacity = 0.8 }
})
```

Properties: `fill`, `fill_opacity`, `stroke`, `stroke_width`, `stroke_opacity`, `stroke_dash` ([5, 3]), `stroke_cap` ("butt"/"round"/"square"), `stroke_join` ("miter"/"round"/"bevel").

## Transforms

`translate(shape, [dx, dy])`, `rotate(shape, angle, pivot?)`, `scale(shape, factor, pivot?)`, `mirror_x(shape, x?)`, `mirror_y(shape, y?)`.

## Groups

```stone
group({
    objects = [shape1, shape2],
    position = [x, y],
    rotation = angle,
    scale = num,
    anchor = [x, y]
})
```

Groups nest. Each child's transform is relative to its parent.

## API Reference

**Shapes:**

| Function | Description |
|----------|-------------|
| `circle(p: Circle)` | Create circle from params record |
| `rect(p: Rect)` | Create rectangle from params record |
| `ellipse(p: Ellipse)` | Create ellipse from params record |
| `line(p: Line)` | Create line from params record |
| `arc(p: Arc)` | Create arc from params record |
| `polygon(p: Polygon)` | Create polygon from params record |
| `polyline(p: Polygon)` | Create polyline (open polygon) from params record |
| `text(p: Text)` | Create text from params record |
| `group(p: Group2D)` | Create group from params record |

**Path Commands:**

| Function | Description |
|----------|-------------|
| `move_to(x: num, y: num)` | Move to point |
| `line_to(x: num, y: num)` | Line to point |
| `quad_to(cx, cy, x, y)` | Quadratic bezier curve |
| `cubic_to(c1x, c1y, c2x, c2y, x, y)` | Cubic bezier curve |
| `arc_to(rx, ry, rotation, large_arc, sweep, x, y)` | Arc to point |
| `close_path` | Close path |
| `path(commands: array<PathCmd, 1>)` | Create path from commands |

**Queries & Transforms:**

| Function | Description |
|----------|-------------|
| `bounds(shape)` | Bounding box |
| `area(shape)` | Area |
| `perimeter(shape)` | Perimeter |
| `center(shape)` | Get center point |
| `contains(shape, pt)` | Point containment test |
| `translate(shape, offset)` | Move center |
| `rotate(shape, angle, pivot = [0, 0])` | Rotate around pivot |
| `scale(shape, factor, pivot = [0, 0])` | Scale from pivot |
| `mirror_x(shape, x = 0)` | Mirror across vertical line |
| `mirror_y(shape, y = 0)` | Mirror across horizontal line |

**Types:** `Bounds2`, `Style2D`, `Circle`, `Rect`, `Ellipse`, `Line`, `Arc`, `Polygon`, `PathCmd`, `Path`, `Text`, `Group2D`, `Shape2D`
