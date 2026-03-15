# Builtin Devices

VACASK provides several built-in devices that are always available without a
`load` directive. These cover independent and dependent sources as well as
inductive coupling.

## Independent sources

### Voltage source (`vsource`)

```text
name (p n) vsource [parameters]
```

Terminals: positive (`p`), negative (`n`). Current flows from `p` through the
source to `n`.

### Current source (`isource`)

```text
name (p n) isource [parameters]
```

Terminals: positive (`p`), negative (`n`). Positive current flows from `p` to
`n` through the external circuit.

### Common parameters

Both `vsource` and `isource` accept the following parameters.

**DC value:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `dc` | real | `0` | DC value (V or A). |
| `type` | identifier | `"dc"` | Source waveform type (see below). |

**AC excitation (for small-signal analyses):**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `mag` | real | `0` | AC magnitude. |
| `phase` | real | `0` | AC phase in degrees. |

**Multiplier:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `$mfactor` | real | `1` | Number of parallel instances. |

### Source waveform types

Set `type` to one of the following identifiers to use a time-varying waveform
in transient analysis.

#### `sine` — Sinusoidal

$$v(t) = \text{sinedc} + \text{ampl} \cdot \sin(2\pi f (t - \text{delay}) + \varphi) \cdot e^{-\theta(t-\text{delay})}$$

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `sinedc` | real | `0` | DC offset. |
| `ampl` | real | `1` | Amplitude. |
| `freq` | real | `1e3` | Frequency in Hz. |
| `sinephase` | real | `0` | Phase in degrees. |
| `theta` | real | `0` | Damping factor (1/s). 0 = no damping. |
| `delay` | real | `0` | Delay in seconds. |

#### `pulse` — Pulse

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `val0` | real | `0` | Initial value. |
| `val1` | real | `1` | Pulsed value. |
| `delay` | real | `0` | Initial delay. |
| `rise` | real | `1e-9` | Rise time. Must be > 0. |
| `fall` | real | `0` | Fall time. ≤ 0 gives a step transition. |
| `width` | real | `0` | Pulse width. |
| `period` | real | `0` | Period. ≤ 0 = single pulse. If > 0: must exceed rise + fall + width. |

#### `exp` — Exponential

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `val0` | real | — | Initial value. |
| `val1` | real | — | Peak value. |
| `delay` | real | `0` | Delay. |
| `td2` | real | — | Second delay. Must be > 0. |
| `tau1` | real | `0` | Rise time constant. |
| `tau2` | real | `0` | Fall time constant. |

#### `pwl` — Piecewise Linear

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `wave` | real vector | — | Alternating time-value pairs: `[t0, v0, t1, v1, ...]`. Must have an even number of elements. |
| `offset` | real | `0` | DC offset added to all values. |
| `scale` | real | `1` | Scale factor for values. |
| `stretch` | real | `1` | Time stretch factor. |
| `pwlperiod` | real | `0` | Period for repeating the waveform. ≤ 0 = no repeat. |

#### `am` — Amplitude Modulated

$$v(t) = \text{sinedc} + \text{ampl}\sin(2\pi f(t-d)+\varphi)\,(1 + m\sin(2\pi f_m(t-d)+\varphi_m))$$

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `sinedc` | real | `0` | DC offset. |
| `ampl` | real | `1` | Carrier amplitude. |
| `freq` | real | `1e3` | Carrier frequency (Hz). |
| `sinephase` | real | `0` | Carrier phase (degrees). |
| `modfreq` | real | `1e3` | Modulation frequency (Hz). |
| `modphase` | real | `0` | Modulation phase. |
| `modindex` | real | `0.5` | Modulation index. |
| `delay` | real | `0` | Delay (s). |

#### `fm` — Frequency Modulated

$$v(t) = \text{sinedc} + \text{ampl}\sin(2\pi f(t-d)+\varphi + m\sin(2\pi f_m(t-d)+\varphi_m))$$

Parameters are identical to `am` except that `modindex` represents the
frequency deviation in Hz.

## Linear controlled sources

### VCCS — Voltage-controlled current source

```text
name (p n cp cn) vccs gain=value
```

Terminals: output positive (`p`), output negative (`n`), control positive
(`cp`), control negative (`cn`).

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `gain` | real | `1` | Transconductance (S = A/V). |
| `$mfactor` | real | `1` | Number of parallel instances. |

### VCVS — Voltage-controlled voltage source

```text
name (p n cp cn) vcvs gain=value
```

Same terminal assignment as VCCS.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `gain` | real | `1` | Voltage gain (V/V). |
| `$mfactor` | real | `1` | Number of parallel instances. |

### CCCS — Current-controlled current source

```text
name (p n) cccs gain=value ctlinst=instance
```

Terminals: output positive (`p`), output negative (`n`). The controlling
current flows through the instance specified by `ctlinst`.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `gain` | real | `1` | Current gain (A/A). |
| `ctlinst` | identifier | — | Name of the controlling instance. |
| `ctlnode` | identifier | `"flow(br)"` | Internal node of the controlling instance. |
| `$mfactor` | real | `1` | Number of parallel instances. |

### CCVS — Current-controlled voltage source

```text
name (p n) ccvs gain=value ctlinst=instance
```

Same terminal and parameter structure as CCCS.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `gain` | real | `1` | Transimpedance (V/A = Ω). |
| `ctlinst` | identifier | — | Name of the controlling instance. |
| `ctlnode` | identifier | `"flow(br)"` | Internal node of the controlling instance. |
| `$mfactor` | real | `1` | Number of parallel instances. |

## Inductive coupling

### Mutual inductance (`mutual`)

```text
name mutual ind1=L1 ind2=L2 k=value
```

Couples two inductor instances with mutual inductance coefficient $k$.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `k` | real | `0` | Coupling coefficient, 0 ≤ k ≤ 1. |
| `ind1` | identifier | — | First inductor instance name. |
| `ind2` | identifier | — | Second inductor instance name. |
