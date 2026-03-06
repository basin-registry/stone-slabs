# Display

Declarative display system. All displays are first-class values you create, configure, and explicitly `show()`.

```stone
import canvas2d, canvas3d, panel, show, vary, reveal from display
```

## show()

Displays anything: canvases, graphs, panels. Multiple calls create separate terminals.

```stone
show(canvas2d({ title = "My Canvas", objects = [...] }))
```

## canvas2d

```stone
show(canvas2d({
    title = "My Canvas",
    origin = "center",
    width = 800, height = 600,
    objects = [circle({ center = [0, 0], radius = 50 })]
}))
```

Fields: `title`, `width` (800), `height` (600), `background`, `origin` ("top-left", "center", "bottom-left"), `pixel_ratio`, `domains`.

## canvas3d

```stone
show(canvas3d({
    title = "3D Scene",
    camera = { position = [8, 6, 4], target = [0, 0, 0] },
    grid = { plane = "xz", size = 20 },
    axes = true,
    objects = [box({ center = [0, 0, 0], size = [2, 2, 2] })]
}))
```

Coordinate system: Z is up. Fields: `title`, `background`, `camera` ({position, target, fov}), `grid` ({plane, size, divisions}), `axes`, `antialias`, `domains`.

## panel

Groups multiple displays with shared animation domains:

```stone
show(panel({
    title = "Synced Views",
    layout = "row",
    displays = [
        canvas2d({ title = "2D", objects = [...] }),
        canvas3d({ title = "3D", objects = [...] })
    ],
    domains = { t = { values = t_vals, playback = { autoplay = true, timestep = 200 } } }
}))
```

## Color Palette

Theme-adaptive named colors: `"blue"`, `"red"`, `"green"`, `"orange"`, `"purple"`, `"teal"`, `"pink"`, `"amber"`, `"indigo"`, `"cyan"`, `"rose"`, `"slate"`, `"black"`, `"white"`, `"foreground"`, `"background"`. Hex like `"#e74c3c"` also works.

## Animation (vary, reveal)

Domains create sliders or playback controls that drive values through shapes.

```stone
show(canvas3d({
    domains = {
        t = { values = range(0, 100), label = "Time",
              playback = { timestep = 50, loop = true, autoplay = true } }
    },
    objects = [...]
}))
```

### vary()

`vary(data, "domain_name")` selects from an array by the domain's current index.

```stone
// Animate position
circle({ center = vary([[0, 0], [50, 50], [100, 0]], "t"), radius = 20 })

// Multi-domain
vary(data, {t = 0, scenario = 1})
```

### reveal()

`reveal(data, "domain_name")` shows elements 0 through the domain's current index, for progressive build-up.

```stone
// Progressively reveal points
{ type = "pointcloud", points = reveal(pts, "draw"), point_size = 3 }
```
