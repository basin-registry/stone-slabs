# Signal Processing

```stone
import hamming, filter, moving_average from signal
```

Windowing: `hamming`, `hanning`, `blackman`, `bartlett`, `rectangular`

Filtering: `filter(b, a, x)` (b=numerator coeffs, a=denominator coeffs, a[0] assumed 1), `moving_average`, `exponential_moving_average`

Resampling: `upsample`, `downsample`

Analysis: `envelope_hilbert`, `rms_envelope`, `unwrap` (phase)

Convolution/Correlation: `conv`, `xcorr`, `autocorr`

Generation: `sinusoid`, `chirp`, `white_noise`

## Events (signal/events)

```stone
import zero_crossing, rising_edge, find_peaks from signal/events
```

Zero crossing: `zero_crossing(x, x_prev)` (returns `{crossed, direction}`), `zero_crossing_time` (interpolated)

Edge detection: `rising_edge`, `falling_edge`, `any_edge` (with threshold)

Signal edge detection: `detect_rising_edges(signal, threshold)`, `detect_falling_edges`, `count_edges` (arrays)

Sample/hold: `sample_hold`, `sample_on_edge`

Trigger logic: `trigger_once`, `monostable` (pulse generator)

Peaks: `find_peaks` (local maxima), `find_valleys` (local minima)

Hysteresis: `hysteresis_compare(x, high_thresh, low_thresh, prev_output)` (thermostat-like)
