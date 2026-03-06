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

## API Reference

| Function | Description |
|----------|-------------|
| `complex(re: num, im: num)` | create complex from real and imaginary parts |
| `complex(re: num)` | create complex from real part (im = 0) |
| `polar(r: num, theta: num)` | create complex from polar coordinates |
| `to_complex(x: num)` | convert num to Complex. Used in promotion syntax: `fn add(a: Complex \| to_complex(a), ...)` |
| `conj(z: Complex)` | complex conjugate (a + bi -> a - bi) |
| `abs(z: Complex)` | complex magnitude |
| `arg(z: Complex)` | complex argument (phase angle) |
| `add(a: Complex, b: Complex)` | Addition: supports Complex+Complex, num+Complex, Complex+num |
| `sub(a: Complex, b: Complex)` | Subtraction |
| `mul(a: Complex, b: Complex)` | Multiplication: (a+bi)(c+di) = (ac-bd) + (ad+bc)i |
| `div(a: Complex, b: Complex)` | Division: (a+bi)/(c+di) = [(ac+bd) + (bc-ad)i] / (c^2+d^2) |
| `neg(a: Complex)` | Negation |
| `exp(z: Complex)` | exp(z) = e^z = e^(a+bi) = e^a * (cos(b) + i*sin(b)) |
| `log(z: Complex)` | log(z) = ln\|z\| + i*arg(z) |
| `sqrt(z: Complex)` | sqrt(z) = sqrt(\|z\|) * e^(i*arg(z)/2) |
| `pow(a: Complex, b: Complex)` | pow(a, b) = e^(b * log(a)) |
| `sin(z: Complex)` | sin(z) = sin(a)cosh(b) + i*cos(a)sinh(b) |
| `cos(z: Complex)` | cos(z) = cos(a)cosh(b) - i*sin(a)sinh(b) |
| `tan(z: Complex)` | tan(z) = sin(z) / cos(z) |
| `sinh(z: Complex)` | sinh(z) = sinh(a)cos(b) + i*cosh(a)sin(b) |
| `cosh(z: Complex)` | cosh(z) = cosh(a)cos(b) + i*sinh(a)sin(b) |
| `tanh(z: Complex)` | tanh(z) = sinh(z) / cosh(z) |
| `is_real(z: Complex)` | check if imaginary part is zero |
| `is_imag(z: Complex)` | check if real part is zero |
| `eq(a: Complex, b: Complex)` | equality comparison |
| `neq(a: Complex, b: Complex)` | inequality comparison |

**Primitives:** `real`, `imag`, `czeros`, `ceye`, `is_complex`, `is_complex_array`

**Constants:** `Complex`
