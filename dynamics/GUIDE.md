# Dynamics

```stone
import fft, ifft from dynamics/fft
import tf, ss, step, bode from dynamics
```

## ODE Solvers (dynamics/ode)

`euler`, `rk4`, `rk45` -- each accepts `f(t, y)`, initial state `y0`, and time array `t`. Work with both scalar and array state.

```stone
import ... from dynamics/ode
```

| Function | Description |
|----------|-------------|
| `euler(f, y0, t)` | Euler method (scalar and array versions) |
| `rk4(f, y0, t)` | 4th-order Runge-Kutta (scalar and array versions) |
| `rk45(f, y0, tspan)` | Adaptive Runge-Kutta-Fehlberg (scalar and array versions) |

## Polynomial (dynamics/poly)

Coefficients are arrays ordered highest power first: `[a_n, ..., a_1, a_0]`.

```stone
import ... from dynamics/poly
```

| Function | Description |
|----------|-------------|
| `polyval(p, x)` | Evaluate polynomial at scalar, complex, or array of points |
| `conv(p1, p2)` | Polynomial multiplication (discrete convolution) |
| `polymul(p1, p2)` | Alias for conv |
| `polyadd(p1, p2)` | Add two polynomials |
| `polysub(p1, p2)` | Subtract two polynomials (p1 - p2) |
| `deconv(p1, p2)` | Polynomial division. Returns { q, r } |
| `polyder(p)` | Derivative of polynomial |
| `polyint(p, c)` | Integral of polynomial with constant term c |
| `roots(p)` | Find roots using companion matrix eigenvalues |
| `poly_from_roots(r)` | Create monic polynomial from roots |
| `polyfit(x, y, n)` | Fit polynomial of degree n to data using least squares |
| `polystr(p)` | String representation of polynomial |

## FFT (dynamics/fft)

```stone
import ... from dynamics/fft
```

| Function | Description |
|----------|-------------|
| `fft(x)` | Fast Fourier Transform (Cooley-Tukey radix-2) |
| `ifft(X)` | Inverse FFT |
| `fftfreq(n, d)` | DFT sample frequencies |
| `rfftfreq(n, d)` | Real FFT sample frequencies (non-negative only) |
| `fftshift(x)` | Shift zero-frequency component to center |
| `ifftshift(x)` | Inverse of fftshift |
| `spectrum(x, fs)` | Magnitude spectrum (one-sided). Returns { freq, mag } |
| `psd(x, fs)` | Power Spectral Density. Returns { freq, power } |
| `phase_spectrum(x)` | Phase spectrum (unwrapped, radians) |
| `fftconvolve(a, b)` | Fast convolution using FFT |

## System Representations (dynamics/systems)

Constructors: `tf(num, den)`, `dtf(num, den, Ts)`, `ss(A, B, C, D)`, `dss(A, B, C, D, Ts)`, `zpk(z, p, k)`, `dzpk(z, p, k, Ts)`

```stone
import ... from dynamics/systems
```

| Function | Description |
|----------|-------------|
| `ss(A, B, C, D)` | Continuous-time state-space system |
| `dss(A, B, C, D, Ts)` | Discrete-time state-space system |
| `tf(num, den)` | Continuous-time transfer function |
| `dtf(num, den, Ts)` | Discrete-time transfer function |
| `zpk(z, p, k)` | Continuous-time zero-pole-gain system |
| `dzpk(z, p, k, Ts)` | Discrete-time zero-pole-gain system |
| `order(sys)` | System order (number of states) |
| `is_continuous(sys)` | Check if continuous-time |
| `is_discrete(sys)` | Check if discrete-time |
| `poles(sys)` | System poles |
| `zeros(sys)` | System zeros |
| `is_stable(sys)` | Stability check |
| `dcgain(sys)` | DC gain (steady-state) |
| `tf2ss(sys)` | Transfer function to state-space |
| `ss2tf(sys)` | State-space to transfer function |
| `zpk2tf(sys)` | Zero-pole-gain to transfer function |
| `tf2zpk(sys)` | Transfer function to zero-pole-gain |
| `series(sys1, sys2)` | Series connection |
| `parallel(sys1, sys2)` | Parallel connection |
| `feedback(sys1, sys2, sign)` | Feedback connection |

## Time-Domain Analysis (dynamics/analysis)

All return `{t, y}` records. `step_info` returns `{rise_time, settling_time, overshoot, peak, peak_time, steady_state}`.

```stone
import ... from dynamics/analysis
```

| Function | Description |
|----------|-------------|
| `step(sys, t)` | Unit step response. Returns { t, y } |
| `impulse(sys, t)` | Impulse response. Returns { t, y } |
| `lsim(sys, u, t, x0)` | Simulate with arbitrary input. Returns { t, y, x } |
| `initial(sys, x0, t)` | Unforced response from initial state. Returns { t, y } |
| `step_info(sys, t)` | Step response characteristics. Returns { rise_time, settling_time, overshoot, peak, peak_time, steady_state } |

## Frequency-Domain Analysis (dynamics/frequency)

`bode` returns `{omega, mag, phase, mag_db}`. `margin` returns `{gm, pm, wcg, wcp}`.

```stone
import ... from dynamics/frequency
```

| Function | Description |
|----------|-------------|
| `evalfr(sys, s)` | Evaluate system at complex frequency |
| `freqresp(sys, omega)` | Frequency response. Returns { omega, H } |
| `bode(sys, omega)` | Bode plot data. Returns { omega, mag, phase, mag_db } |
| `nyquist(sys, omega)` | Nyquist plot data. Returns { omega, real, imag } |
| `margin(sys)` | Gain and phase margins. Returns { gm, pm, wcg, wcp } |
| `bandwidth(sys)` | -3dB bandwidth |

## Discrete-Time (dynamics/discrete)

Methods for c2d: "zoh", "tustin", "forward", "backward".

```stone
import ... from dynamics/discrete
```

| Function | Description |
|----------|-------------|
| `c2d(sys, Ts, method)` | Continuous to discrete conversion. Methods: "zoh", "tustin", "forward", "backward" |
| `d2c(sys, method)` | Discrete to continuous conversion |
| `dlsim(sys, u, x0)` | Simulate discrete system. Returns { y, x } |
| `dstep(sys, n)` | Discrete step response |
| `dimpulse(sys, n)` | Discrete impulse response |

## Control Design (dynamics/control)

`pid(Kp, Ki, Kd)`, `pid2(Kp, Ki, Kd, Tf)` -- PID controllers as transfer functions

Controllability/observability: `ctrb`, `obsv`, `is_controllable`, `is_observable`

Pole placement: `place`, `acker`, `lqr`, `rlocus`

```stone
import ... from dynamics/control
```

| Function | Description |
|----------|-------------|
| `pid(Kp, Ki, Kd)` | PID controller transfer function |
| `pid2(Kp, Ki, Kd, Tf)` | PID with derivative filter |
| `ctrb(A, B)` | Controllability matrix |
| `is_controllable(A, B)` | Check controllability |
| `obsv(A, C_mat)` | Observability matrix |
| `is_observable(A, C_mat)` | Check observability |
| `place(A, B, p)` | Pole placement using Ackermann's formula |
| `acker(A, B, p)` | Ackermann's formula for pole placement |
| `lqr(A, B, Q, R)` | Linear Quadratic Regulator. Returns { K, S, E } |
| `rlocus(sys, K_vals)` | Root locus. Returns { K, roots } |
