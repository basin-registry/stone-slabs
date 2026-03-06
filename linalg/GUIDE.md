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

## API Reference

| Function | Description |
|----------|-------------|
| `transpose(A)` | matrix transpose |
| `trace(A)` | sum of diagonal elements |
| `cross(a, b)` | cross product of two 3D vectors |
| `normalize(v)` | normalize vector to unit length |
| `matmul(A, B)` | matrix multiplication (wrapper for @ operator) |
| `det(A)` | determinant of a square matrix using Gaussian elimination with partial pivoting |
| `solve(A, b)` | solve linear system Ax = b using Gaussian elimination |
| `inv(A)` | matrix inverse using Gauss-Jordan elimination |
| `hcat(A, B)` | Horizontal concatenation of 2D matrices [A \| B] |
| `vcat(A, B)` | Vertical concatenation of 2D matrices [A; B] |
| `kron(A, B)` | Kronecker product |
| `blkdiag(A, B)` | Block diagonal matrix |
| `qr(A)` | QR decomposition using Modified Gram-Schmidt. Returns { Q, R } |
| `lu(A)` | LU decomposition with partial pivoting. Returns { L, U, P } |
| `eigvals(A)` | Eigenvalues using QR algorithm with Hessenberg reduction |
| `expm(A)` | Matrix exponential using scaling and squaring with Pade approximation |
| `charpoly(A)` | Characteristic polynomial coefficients using Faddeev-LeVerrier |

**Primitives:** `zeros`, `ones`, `eye`, `dot`, `outer`, `norm`, `diag`
