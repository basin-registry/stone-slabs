# Linear Algebra

```stone
import zeros, eye, dot, norm, solve, det, inv from linalg
```

Constructors: `zeros(n)` (1D), `zeros(m, n)` (2D matrix), `ones(n)`, `ones(m, n)`, `eye(n)` (identity)

Basic: `dot`, `outer`, `norm`, `trace`, `diag`, `cross`, `normalize`

Matrix ops: `matmul` (or `@`), `transpose`, `det`, `inv`, `solve`

Stacking: `hcat`, `vcat`, `kron`, `blkdiag`

Decompositions: `qr` (returns `{Q, R}`), `lu` (returns `{L, U, P}`)

Eigenvalues: `eigvals` (may return complex numbers)

Matrix functions: `expm` (matrix exponential), `charpoly` (characteristic polynomial)
