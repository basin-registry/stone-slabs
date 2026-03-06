# Nonlinear Operations

```stone
import sign, saturate, dead_zone from optimize
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
