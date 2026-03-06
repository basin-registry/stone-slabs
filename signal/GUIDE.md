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

## API Reference

| Function | Description |
|----------|-------------|
| `hamming(n)` | Hamming window of length n |
| `hanning(n)` | Hanning (Hann) window of length n |
| `blackman(n)` | Blackman window of length n |
| `bartlett(n)` | Bartlett (triangular) window of length n |
| `rectangular(n)` | Rectangular window (all ones) |
| `filter(b, a, x)` | Apply IIR/FIR filter to signal x |
| `moving_average(x, window_size)` | Simple moving average filter |
| `exponential_moving_average(x, alpha)` | Exponential moving average |
| `upsample(x, factor)` | Upsample by inserting zeros |
| `downsample(x, factor)` | Downsample by keeping every nth sample |
| `envelope_hilbert(x)` | Signal envelope using Hilbert transform approximation |
| `rms_envelope(x, window_size)` | RMS envelope with sliding window |
| `unwrap(phase)` | Unwrap phase angles to remove discontinuities |
| `conv(a, b)` | Discrete convolution of two signals |
| `xcorr(a, b)` | Cross-correlation of two signals |
| `autocorr(x)` | Autocorrelation of signal with itself |
| `sinusoid(n, freq, sample_rate, phase)` | Generate sinusoidal signal |
| `chirp(n, f0, f1, sample_rate)` | Generate linear chirp signal |
| `white_noise(n)` | Generate white noise |

### signal/events

Event detection functions are also re-exported from `signal` directly.

| Function | Description |
|----------|-------------|
| `zero_crossing(x, x_prev)` | Detect zero crossing. Returns { crossed, direction } |
| `zero_crossing_time(x, x_prev, t, dt)` | Estimate time of zero crossing. Returns { crossed, time, direction } |
| `rising_edge(x, x_prev, threshold)` | Detect rising edge crossing threshold |
| `falling_edge(x, x_prev, threshold)` | Detect falling edge crossing threshold |
| `any_edge(x, x_prev, threshold)` | Detect any edge crossing threshold. Returns { edge, rising } |
| `detect_rising_edges(signal, threshold)` | Find all rising edges in a signal array |
| `detect_falling_edges(signal, threshold)` | Find all falling edges in a signal array |
| `count_edges(signal, threshold)` | Count total number of threshold crossings |
| `sample_hold(trigger, value, held_value)` | Sample and hold operation |
| `sample_on_edge(signal, data, threshold)` | Sample data values at rising edges |
| `trigger_once(condition, triggered_prev)` | Trigger that fires only once. Returns { triggered, output } |
| `monostable(trigger, duration_samples, counter_prev)` | Monostable multivibrator. Returns { output, counter } |
| `find_peaks(signal)` | Find local maxima in signal |
| `find_valleys(signal)` | Find local minima in signal |
| `hysteresis_compare(x, high_thresh, low_thresh, prev_output)` | Comparator with hysteresis |
