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
