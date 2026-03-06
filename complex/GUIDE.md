# Complex Numbers

```stone
import complex, polar, conj, abs, arg from complex
```

Complex numbers are records: `{re: num, im: num}`

Constructors: `complex(re, im)`, `polar(r, theta)`, `to_complex`

Arrays: `czeros`, `ceye`

Accessors: `z.re`, `z.im`, `conj`, `abs` (magnitude), `arg` (phase)

Arithmetic: `+`, `-`, `*`, `/` (operators work), or explicit `add`, `sub`, `mul`, `div`, `neg`

Transcendental: `exp`, `log`, `sqrt`, `pow`, `sin`, `cos`, `tan`, `sinh`, `cosh`, `tanh`

Type promotion: mix `num` and complex freely (e.g., `complex(1,2) + 5` works)
