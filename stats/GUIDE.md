# Statistics

```stone
import mean, std, corr from stats
```

Central tendency: `mean`, `median`

Dispersion: `variance(xs, ddof)`, `std(xs, ddof)` (ddof=0 for population, ddof=1 for sample)

Relationships: `cov(xs, ys)`, `corr(xs, ys)` (Pearson correlation, -1 to 1)

Quantiles: `quantile(xs, q)`, `percentile(xs, p)`, `iqr(xs)` (interquartile range)
