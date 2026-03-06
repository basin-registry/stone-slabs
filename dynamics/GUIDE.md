# Dynamics

```stone
import fft, ifft from dynamics/fft
import tf, ss, step, bode from dynamics
```

## ODE Solvers (dynamics/ode)

`euler`, `rk4`, `rk45` — each accepts `f(t, y)`, initial state `y0`, and time array `t`. Work with both scalar and array state.

## Polynomial (dynamics/poly)

`polyval`, `polymul`, `deconv`, `roots`, `poly_from_roots`, `polyadd`, `polysub`, `polyder`, `polyint`, `polyfit`, `polystr`

Coefficients are arrays ordered highest power first: `[a_n, ..., a_1, a_0]`.

## FFT (dynamics/fft)

`fft`, `ifft`, `fftfreq`, `rfftfreq`, `fftshift`, `ifftshift`, `spectrum`, `psd`, `phase_spectrum`, `fftconvolve`

## System Representations (dynamics/systems)

Constructors: `tf(num, den)`, `dtf(num, den, Ts)`, `ss(A, B, C, D)`, `dss(A, B, C, D, Ts)`, `zpk(z, p, k)`, `dzpk(z, p, k, Ts)`

Analysis: `poles`, `zeros`, `dcgain`, `order`, `is_stable`, `is_continuous`, `is_discrete`

Conversions: `tf2ss`, `ss2tf`, `zpk2tf`, `tf2zpk`

Interconnection: `series`, `parallel`, `feedback`

## Time-Domain Analysis (dynamics/analysis)

`step`, `impulse`, `lsim`, `initial`, `step_info`

All return `{t, y}` records. `step_info` returns `{rise_time, settling_time, overshoot, peak, peak_time, steady_state}`.

## Frequency-Domain Analysis (dynamics/frequency)

`freqresp`, `evalfr`, `bode`, `nyquist`, `margin`, `bandwidth`

`bode` returns `{omega, mag, phase, mag_db}`. `margin` returns `{gm, pm, wcg, wcp}`.

## Discrete-Time (dynamics/discrete)

`c2d(sys, Ts, method)`, `d2c(sys, method)`, `dlsim`, `dstep`, `dimpulse`

Methods for c2d: "zoh", "tustin", "forward", "backward".

## Control Design (dynamics/control)

`pid(Kp, Ki, Kd)`, `pid2(Kp, Ki, Kd, Tf)` — PID controllers as transfer functions

Controllability/observability: `ctrb`, `obsv`, `is_controllable`, `is_observable`

Pole placement: `place`, `acker`, `lqr`, `rlocus`
