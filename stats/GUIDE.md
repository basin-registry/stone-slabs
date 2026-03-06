# Statistics

```stone
import mean, std, corr from stats
```

Central tendency: `mean`, `median`

Dispersion: `variance(xs, ddof)`, `std(xs, ddof)` (ddof=0 for population, ddof=1 for sample)

Relationships: `cov(xs, ys)`, `corr(xs, ys)` (Pearson correlation, -1 to 1)

Quantiles: `quantile(xs, q)`, `percentile(xs, p)`, `iqr(xs)` (interquartile range)

## API Reference

| Function | Description |
|----------|-------------|
| `mean(xs)` | Arithmetic mean (average) of array elements |
| `median(xs)` | Median value (middle element of sorted array) |
| `variance(xs)` | Population variance (single-argument version, ddof=0) |
| `variance(xs, ddof)` | Full variance with ddof parameter |
| `std(xs)` | Population std (single-argument version, ddof=0) |
| `std(xs, ddof)` | Full std with ddof parameter |
| `cov(xs, ys)` | Covariance between two arrays (sample covariance) |
| `corr(xs, ys)` | Pearson correlation coefficient |
| `quantile(xs, q)` | Quantile (q-th fractile, q in [0,1]) Uses linear interpolation |
| `percentile(xs, p)` | Percentile (p-th percentile, p in [0,100]) |
| `iqr(xs)` | Interquartile range (Q3 - Q1) |
