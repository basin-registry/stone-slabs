# Interpolation

```stone
import interp1_linear, lookup1d from interpolate
```

Prelookup: `prelookup(breakpoints, x)` returns `{index, fraction}`

1D interpolation: `interp1_linear`, `interp1_nearest`, `interp1_linear_arr` (for arrays), `interp1(x, y, xi, method)` (method: "linear" or "nearest")

2D interpolation: `interp2_linear`, `interp2_nearest`, `interp2(x, y, z, xi, yi, method)` (bilinear, method: "linear" or "nearest")

Lookup tables: `lookup1d(table, breakpoints, input)`, `lookup2d(table, row_bp, col_bp, row_in, col_in)`
