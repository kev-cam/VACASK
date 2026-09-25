# Periodic Small-Signal Analysis (pac)

The `pac` analysis computes the small-signal frequency-domain response of a circuit
operating in a periodic steady state. It first finds the periodic steady-state operating
point with the shooting method, then sweeps a small-signal input frequency and solves the
linearized conversion matrix at each frequency point. It is the shooting-based counterpart
of the harmonic-balance-based [hbac](cmd-analysis-hbac.md) analysis and is suited for
circuits with a single fundamental frequency, including strongly nonlinear and switching circuits.

## Syntax

```text
analysis name pac [parameters]
```

## How it works

1. VACASK finds the periodic steady state with the shooting Newton method.
   See [Periodic Steady-State Analysis](cmd-analysis-pss.md) for details.
2. One more period is integrated and the resistive and reactive Jacobians $G(t)$ and $C(t)$
   are recorded at equally spaced time points. The number of points is the period divided by
   the maximal timestep of the shooting transient, rounded up to a power of 2.
3. An FFT turns them into Fourier coefficients $G_k$ and $C_k$, $k=0,\ldots,\text{truncharm}$.
   Because $G(t)$ and $C(t)$ are real, the coefficients for negative $k$ are the complex
   conjugates of those for $\lvert k \rvert$.
4. At each small-signal input frequency $f$ VACASK forms the conversion matrix.
   Each subblock $(n,m)$ couples unknown $m$ to circuit equation $n$ across the harmonics:

   $$H^{(nm)}_{kl}(f) = [G_{k-l}]_{nm} + j\,(\omega + k\,\omega_0)\,[C_{k-l}]_{nm}, \qquad \omega_0 = \frac{2\pi}{T}, \qquad \omega = 2 \pi f$$

   where $n$, $m$ are node indices, $k$, $l = -\text{truncharm},\ldots,\text{truncharm}$ are
   harmonic (sideband) indices, and $T$ is the period. Coefficients with $\lvert k-l \rvert > \text{truncharm}$
   are treated as 0. The matrix is Toeplitz in the harmonic indices because
   there is a single fundamental and no intermodulation.
5. It solves $H(f)\,X = U$, where $U$ is assembled from the `spur`, `smag`, and `sphase`
   parameters of independent sources.
6. The selected output harmonics of $X$ are written to the output file.
7. Steps 4-6 are repeated across the frequency sweep.

The response at harmonic $h$ is observed at the frequency $f + h/T$.

## Small-signal excitation

Each independent source injects excitation at the harmonics listed in its `spur` parameter,
with magnitudes and phases given by `smag` and `sphase`. Sources with no `spur` entries
contribute no excitation. In `pac` each `spur` entry is either a signed integer harmonic
number $h$ (in the range $-\text{truncharm},\ldots,\text{truncharm}$) or a real frequency (Hz)
that equals $h/T$ for such an $h$. Harmonic 0 is the excitation at the offset frequency $f$
itself. See [Small-Signal Excitation](dev-builtin-src.md#quasiperiodic-small-signal-excitation)
in the Independent Sources reference for the parameter details.

The `mag` and `phase` parameters of independent sources are used by AC and DC incremental
analyses and have no effect in `pac`.

## Parameters

Sweep parameters (`from`, `to`, `step`, `mode`, `points`, `values`) follow the same convention
as in [AC Small-Signal Analysis](cmd-analysis-ac.md). The sweep variable is the offset frequency $f$.

The following parameters are specific to `pac`:

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `truncharm` | integer | `10` | Number of harmonics kept beyond DC. The conversion matrix has $2\,\text{truncharm}+1$ harmonics per unknown, and the Jacobian's Fourier series is truncated beyond this order. Must be >=0. It must be smaller than half the number of time points of the recorded period, see [Choosing truncharm](#choosing-truncharm). |
| `outharm` | integer or integer vector | `{}` | Output harmonic(s) to observe, given as signed integer harmonic numbers in the range $-\text{truncharm},\ldots,\text{truncharm}$. Default `{}` selects all harmonics. A frequency cannot be used because the period is not known before the PSS is solved. The selection cannot change between the points of a parameter sweep. |
| `write` | boolean | `1` | Write the small-signal results to a file. |
| `solver` | string | `""` | Linear solver for the complex conversion-matrix solve, overriding the `qpsmsigsolver` option. See [Linear Solver Selection](cmd-options-solver.md). |

`pac` exposes the following parameters of the [periodic steady-state analysis](cmd-analysis-pss.md)
with the same meaning: `tper`, `oscillator`, `tstab`, `stabstep`, `icmode`, `ic`, `nodeset`,
`maxharm`, `maxacfreq`, `store`, and `writestab`. The remaining PSS parameters are renamed
or have different defaults:

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `psssolve` | boolean | `1` | Solve the PSS. Set to `0` to linearize at the stored PSS solution named by `ic` without solving. `ic` must then be a string naming a stored PSS solution, and the period is taken from it. |
| `writepss` | boolean | `0` | Write the PSS waveform (one period) to `<analysis>.pss.*`. `writestab` additionally writes the stabilization transient to `<analysis>.pss.tran.*`. |
| `pssopsolver` | string | `""` | Linear solver for the operating point, stabilization transient, and shooting iterations, overriding the `tdsolver` option. It is the PSS `solver` parameter. In `pac`, `solver` refers to the conversion-matrix solver. |

### Choosing truncharm

The recorded period must resolve the harmonics that are kept: `truncharm` must be smaller than
half the number of recorded time points. The number of points is set by the maximum
timestep of the shooting transient, which is limited by the `pss_minpts` option
(see [Periodic Steady-State Options](cmd-options-pss.md)) and by `maxharm` and `maxacfreq`.
Set `maxharm` at least to `truncharm` if you use larger values than the default allows;
otherwise the analysis stops with an error.

The result is meaningful only when `truncharm` is large enough to represent the harmonic
content of $G(t)$ and $C(t)$. This is usually a small number for smooth circuits.
For circuits with hard switching, such as switched-capacitor circuits with steep clock
edges, the Jacobians are close to rectangular pulse trains whose coefficients decay slowly,
and the results may change considerably with `truncharm` until it becomes large enough
(hundreds of harmonics). Increase `truncharm` until the results no longer change.
The cost of the analysis grows quickly with `truncharm` because every nonzero Jacobian entry
becomes a dense $(2\,\text{truncharm}+1) \times (2\,\text{truncharm}+1)$ block.

## Save directives

The following small-signal save directives are supported:

| Directive | Description |
|-----------|-------------|
| `default` | Save all small-signal node phasors and branch flows (default behavior). |
| `full` | Save all small-signal unknowns (even those belonging to collapsed nodes). |
| `dv(node)` | Save the small-signal phasor at the given node. |
| `di(instance)` | Save the small-signal current phasor through the given instance. Only instances that introduce a current variable in the MNA system are valid (e.g. voltage sources, inductors). |

The following PSS save directives are also supported. They apply to the PSS waveform and are written to
`<analysis>.pss.*` when `writepss=1` (and to `<analysis>.pss.tran.*` for the stabilization transient
when `writestab=1`).

| Directive | Description |
|-----------|-------------|
| `pssdefault` | Save all PSS node values and branch flows. |
| `pssfull` | Save all PSS unknowns (even those belonging to collapsed nodes). |
| `v(node)` | Save the PSS waveform at the given node. |
| `i(instance)` | Save the PSS branch flow through the given instance. Only instances that introduce a current variable in the MNA system are valid (e.g. voltage sources, inductors). |
| `p(instance,outvar)` | Save the output variable `outvar` of the given instance along the PSS waveform. |

## Output

- A file `<analysis>.*` containing the small-signal results at each frequency point.
- If `writepss=1`, an additional `<analysis>.pss.*` file containing one period of the PSS waveform.
- If `writestab=1`, an additional `<analysis>.pss.tran.*` file containing the stabilization transient.

| Variable | Description |
|----------|-------------|
| `frequency` | Small-signal offset frequency sweep variable (Hz). Always present. |
| `node;h` | Complex small-signal phasor at `node` for output harmonic `h` (signed integer). One variable per saved node per output harmonic. |
| `instance:flow(br);h` | Complex small-signal branch flow phasor for output harmonic `h`. |

The frequency at which the response is observed for offset frequency $f$ and output harmonic $h$
is $f + h/T$.

## Examples

**Small-signal response of a nonlinear circuit with a 1 kHz large signal, all harmonics up to 3 observed:**

```text
Single tone PAC

load "nl.va"

model vsource vsource
model nl nl

// 1 kHz large signal (magnitude 1, cosine) and small-signal excitation at harmonic 0
v1 (1 0) vsource dc=0 type="sine" sinedc=0.0 ampl=1 freq=1k tdphase=90 spur={0} smag=[1]
nl1 (1 0) nl a=3 c=5

control
  analysis pac1 pac tper=1m tstab=20m outharm={} values=[10.0] truncharm=3 maxharm=10
endc

embed "nl.va" <<<FILE
`include "constants.vams"
`include "disciplines.vams"

module nl(n1,n2);
    inout n1, n2;
    electrical n1,n2;

    parameter real a = 1;
    parameter real c = 1;

    analog begin
        I(n1,n2) <+ pow(a*V(n1,n2), 2) + ddt(pow(c*V(n1,n2), 3));
    end
endmodule
>>>FILE
```

**Frequency sweep, observe only the output at harmonic 0:**

```text
analysis pac1 pac tper=10u truncharm=60 outharm=0 from=10 to=50k mode="dec" points=5
```

**Reuse a stored PSS solution:**

```text
analysis pss1 pss tper=1m tstab=20m store="psssol"
// Linearize at the stored solution, the period is taken from it
analysis pac1 pac ic="psssol" psssolve=0 truncharm=3 values=[10.0, 100.0]
```

## Options

- [Periodic Steady-State Options](cmd-options-pss.md)
- [Small-Signal Analysis Options](cmd-options-smsig.md) (`smsig_debug`)
- [Transient Analysis Options](cmd-options-tran.md)
- [Newton-Raphson Solver](cmd-options-nr.md)
- [Linear Solver Selection](cmd-options-solver.md)
