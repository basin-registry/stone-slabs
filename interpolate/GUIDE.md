# Interpolation

Aliased as `interp`.

```stone
import interp1_linear, lookup1d from interpolate
```

Prelookup: `prelookup(breakpoints, x)` returns `{index, fraction}`

1D interpolation: `interp1_linear`, `interp1_nearest`, `interp1_linear_arr` (for arrays), `interp1(x, y, xi, method)` (method: "linear" or "nearest")

2D interpolation: `interp2_linear`, `interp2_nearest`, `interp2(x, y, z, xi, yi, method)` (bilinear, method: "linear" or "nearest")

Lookup tables: `lookup1d(table, breakpoints, input)`, `lookup2d(table, row_bp, col_bp, row_in, col_in)`

## API Reference

| Function | Description |
|----------|-------------|
| `prelookup(breakpoints, x)` | Find index and interpolation fraction. Returns { index, fraction } |
| `interp1_linear(x, y, xi)` | 1D linear interpolation (scalar query) |
| `interp1_linear_arr(x, y, xi_arr)` | 1D linear interpolation for array of query points |
| `interp1_nearest(x, y, xi)` | 1D nearest neighbor interpolation (scalar query) |
| `interp1_nearest_arr(x, y, xi_arr)` | 1D nearest interpolation for array of query points |
| `lookup1d(table, breakpoints, input)` | 1D lookup table with linear interpolation |
| `lookup2d(table, row_bp, col_bp, row_in, col_in)` | 2D lookup table with bilinear interpolation |
| `interp2_linear(x, y, z, xi, yi)` | 2D bilinear interpolation |
| `interp2_nearest(x, y, z, xi, yi)` | 2D nearest neighbor interpolation |
| `interp1(x, y, xi, method)` | 1D interpolation with method selection ("linear" or "nearest") |
| `interp2(x, y, z, xi, yi, method)` | 2D interpolation with method selection |
