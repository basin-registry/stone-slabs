# Nonlinear Operations

```stone
import sign, saturate, dead_zone from nonlinear
```

Basic: `sign`, `heaviside`, `step`

Saturation: `saturate(u, lower, upper)`, `saturation(u, lower, upper)` (returns `{y, saturated}`)

Dead zone: `dead_zone(u, lower, upper)`, `dead_zone_symmetric(u, deadband)`

Rate limiting: `rate_limiter(u, u_prev, dt, rising_rate, falling_rate)`, `rate_limiter_symmetric`

Hysteresis: `backlash`, `relay`, `schmitt_trigger`

Friction: `coulomb_friction`, `viscous_friction`, `coulomb_viscous_friction`, `stribeck_friction`

Quantization: `quantize`, `floor_quantize`, `ceil_quantize`

Angle wrapping: `wrap_angle` (to [-pi, pi]), `wrap_angle_deg` (to [-180, 180]), `wrap_to_2pi`, `wrap_to_360`

Other: `mod_positive`, `ramp`, `ramp_limited`

## API Reference

| Function | Description |
|----------|-------------|
| `sign(x)` | Sign function. Returns -1 for negative, 0 for zero, 1 for positive |
| `heaviside(x)` | Heaviside step function. Returns 0 for x < 0, 0.5 for x == 0, 1 for x > 0 |
| `step(x)` | Unit step function. Returns 0 for x < 0, 1 for x >= 0 |
| `saturate(u, lower, upper)` | Clamp value to [lower, upper] |
| `dead_zone(u, lower, upper)` | Dead zone function. Output is 0 within the dead band, linear outside |
| `dead_zone_symmetric(u, deadband)` | Symmetric dead zone |
| `rate_limiter(u, u_prev, dt, rising_rate, falling_rate)` | Rate limiter |
| `rate_limiter_symmetric(u, u_prev, dt, rate)` | Symmetric rate limiter |
| `backlash(u, u_prev, y_prev, deadband)` | Mechanical backlash model |
| `relay(u, on_thresh, off_thresh, y_prev)` | Relay with hysteresis |
| `schmitt_trigger(u, high_thresh, low_thresh, prev_state)` | Schmitt trigger |
| `coulomb_friction(v, Fc)` | Simple Coulomb friction |
| `coulomb_viscous_friction(v, Fc, b)` | Coulomb + viscous friction |
| `viscous_friction(v, b)` | Pure viscous damping |
| `stribeck_friction(v, Fc, Fs, vs, sigma)` | Stribeck friction model |
| `quantize(x, q)` | Quantize to nearest multiple of q |
| `floor_quantize(x, q)` | Quantize down to multiple of q |
| `ceil_quantize(x, q)` | Quantize up to multiple of q |
| `wrap_angle(theta)` | Wrap angle to [-pi, pi] |
| `wrap_angle_deg(theta)` | Wrap angle to [-180, 180] degrees |
| `wrap_to_2pi(theta)` | Wrap angle to [0, 2*pi] |
| `wrap_to_360(theta)` | Wrap angle to [0, 360] degrees |
| `ramp(x)` | Ramp function (positive part of x) |
| `ramp_limited(x, limit)` | Limited ramp |
