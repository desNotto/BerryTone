#pragma once
// Berry Tone DSP. Everything here is zero-lookahead (IIR filters + envelope
// followers + sample-by-sample nonlinearities), so the plugin adds 0 samples
// of latency. Real-time safe: no allocation, no locks.

#include <algorithm>
#include <cmath>

namespace berry
{
constexpr int kMaxCh = 2;
constexpr double kPi = 3.14159265358979323846;

inline float dbToLin (float db)  { return std::exp (db * 0.11512925f); }
inline float linToDb (float lin) { return 8.685889638f * std::log (std::max (lin, 1.0e-9f)); }

//==============================================================================
// Attack/release peak follower
struct Env
{
    float a = 0.f, r = 0.f, y = 0.f;
    void set (double sr, float attMs, float relMs)
    {
        a = (float) std::exp (-1.0 / (0.001 * attMs * sr));
        r = (float) std::exp (-1.0 / (0.001 * relMs * sr));
    }
    inline float process (float x)
    {
        const float c = x > y ? a : r;
        y = x + c * (y - x);
        return y;
    }
    void reset() { y = 0.f; }
};

//==============================================================================
// RBJ biquad, transposed direct form II, double precision state
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1[kMaxCh] {}, z2[kMaxCh] {};

    void reset() { for (int i = 0; i < kMaxCh; ++i) z1[i] = z2[i] = 0.0; }

    inline float process (float xf, int ch)
    {
        const double x = xf;
        const double y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return (float) y;
    }

    void setLowpass (double sr, double f, double q)
    {
        f = fc (sr, f);
        const double w = 2.0 * kPi * f / sr, c = std::cos (w), al = std::sin (w) / (2.0 * q);
        coeffs ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }

    void setHighpass (double sr, double f, double q)
    {
        f = fc (sr, f);
        const double w = 2.0 * kPi * f / sr, c = std::cos (w), al = std::sin (w) / (2.0 * q);
        coeffs ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }

    void setLowShelf (double sr, double f, double gainDb)
    {
        f = fc (sr, f);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2.0 * kPi * f / sr, c = std::cos (w), s = std::sin (w);
        const double sq = 2.0 * std::sqrt (A) * (s / std::sqrt (2.0));
        coeffs (A * ((A + 1) - (A - 1) * c + sq),
                2 * A * ((A - 1) - (A + 1) * c),
                A * ((A + 1) - (A - 1) * c - sq),
                (A + 1) + (A - 1) * c + sq,
                -2 * ((A - 1) + (A + 1) * c),
                (A + 1) + (A - 1) * c - sq);
    }

    void setHighShelf (double sr, double f, double gainDb)
    {
        f = fc (sr, f);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2.0 * kPi * f / sr, c = std::cos (w), s = std::sin (w);
        const double sq = 2.0 * std::sqrt (A) * (s / std::sqrt (2.0));
        coeffs (A * ((A + 1) + (A - 1) * c + sq),
                -2 * A * ((A - 1) + (A + 1) * c),
                A * ((A + 1) + (A - 1) * c - sq),
                (A + 1) - (A - 1) * c + sq,
                2 * ((A - 1) - (A + 1) * c),
                (A + 1) - (A - 1) * c - sq);
    }

private:
    static double fc (double sr, double f) { return std::min (std::max (f, 5.0), sr * 0.45); }
    void coeffs (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
};

//==============================================================================
// 1) LOW FREQ TIGHTENER
// Level-dependent low-band squeeze (kills boomy sustained lows) followed by a
// 2nd-order high-pass. The low band is split subtractively so the recombination
// is exact when "Tight" = 0.
struct Tightener
{
    double sr = 48000;
    Biquad hp, xo;
    Env env;
    float amt = 0.f;

    void prepare (double s)
    {
        sr = s;
        xo.setLowpass (sr, 160.0, 0.7071);
        env.set (sr, 2.f, 60.f);
        hp.reset(); xo.reset(); env.reset();
        set (60.f, 0.f);
    }
    void set (float hpFreq, float amount)
    {
        hp.setHighpass (sr, hpFreq, 0.7071);
        amt = amount;
    }
    void process (float* x, int n)
    {
        float low[kMaxCh] {};
        float det = 0.f;
        for (int c = 0; c < n; ++c) { low[c] = xo.process (x[c], c); det = std::max (det, std::fabs (low[c])); }
        const float e = env.process (det);
        const float g = 1.f / (1.f + amt * 8.f * e);
        for (int c = 0; c < n; ++c)
            x[c] = hp.process (x[c] - low[c] + low[c] * g, c);
    }
};

//==============================================================================
// 2) HORIZON GATE
// Zero-latency downward gate: high-passed sidechain, hysteresis, hold,
// adjustable release and depth. No lookahead, so the open time is ~0.4 ms.
struct HorizonGate
{
    double sr = 48000;
    Biquad scHp;
    Env det;
    float openThr = 1e-3f, closeThr = 7e-4f, floorGain = 1e-3f;
    float atkC = 0.f, relC = 0.f, gain = 1.f;
    int hold = 0, holdCnt = 0;
    bool open = false;

    void prepare (double s)
    {
        sr = s;
        scHp.setHighpass (sr, 110.0, 0.7071);
        det.set (sr, 0.15f, 20.f);
        atkC = (float) (1.0 - std::exp (-1.0 / (0.0004 * sr)));
        hold = (int) (0.012 * sr);
        scHp.reset(); det.reset();
        gain = 1.f; holdCnt = 0; open = false;
        set (-55.f, 80.f, -70.f);
    }
    void set (float threshDb, float relMs, float depthDb)
    {
        openThr = dbToLin (threshDb);
        closeThr = dbToLin (threshDb - 4.f);
        floorGain = dbToLin (depthDb);
        relC = (float) (1.0 - std::exp (-1.0 / (0.001 * relMs * sr)));
    }
    void process (float* x, int n)
    {
        float d = 0.f;
        for (int c = 0; c < n; ++c) d = std::max (d, std::fabs (scHp.process (x[c], c)));
        const float e = det.process (d);

        if (e > openThr)                 { open = true; holdCnt = hold; }
        else if (open && e < closeThr)   { if (holdCnt > 0) --holdCnt; else open = false; }

        const float target = open ? 1.f : floorGain;
        gain += (target - gain) * (target > gain ? atkC : relC);
        for (int c = 0; c < n; ++c) x[c] *= gain;
    }
};

//==============================================================================
// 3) CHUG ENHANCER
// Transient shaper. A band-limited detector (90 Hz - 3 kHz, so pick noise and
// sub rumble don't trigger it) compares a fast and slow envelope.
//  Punch: + emphasises the attack of palm mutes, - softens it.
//  Body : + extends the note body, - cuts it short (staccato chug).
struct ChugEnhancer
{
    double sr = 48000;
    Biquad dHp, dLp;
    Env fast, slow;
    float punch = 0.f, body = 0.f, gain = 1.f, upC = 0.f, dnC = 0.f;

    void prepare (double s)
    {
        sr = s;
        dHp.setHighpass (sr, 90.0, 0.7071);
        dLp.setLowpass (sr, 3000.0, 0.7071);
        upC = (float) (1.0 - std::exp (-1.0 / (0.0006 * sr)));
        dnC = (float) (1.0 - std::exp (-1.0 / (0.012 * sr)));
        dHp.reset(); dLp.reset(); fast.reset(); slow.reset(); gain = 1.f;
        set (0.f, 0.f, 1.f);
    }
    void set (float p, float b, float speed)
    {
        punch = p; body = b;
        speed = std::max (speed, 0.1f);
        fast.set (sr, 0.8f / speed, 14.f / speed);
        slow.set (sr, 18.f / speed, 140.f / speed);
    }
    void process (float* x, int n)
    {
        float d = 0.f;
        for (int c = 0; c < n; ++c) d = std::max (d, std::fabs (dLp.process (dHp.process (x[c], c), c)));
        const float f = fast.process (d), s = slow.process (d);

        float db = 0.f;
        if (s > 3e-4f) db = std::clamp (linToDb (f / s), -18.f, 18.f);

        float gdb = db > 0.f ? punch * db * 0.8f : -body * db * 0.6f;
        gdb = std::clamp (gdb, -12.f, 12.f);

        const float target = dbToLin (gdb);
        gain += (target - gain) * (target > gain ? upC : dnC);
        for (int c = 0; c < n; ++c) x[c] *= gain;
    }
};

//==============================================================================
// 4) LOW OCTAVE BLENDER
// Classic analog-style octave-down: low-passed tracker -> Schmitt trigger ->
// flip-flop (divide by 2) -> amplitude-follow -> rounded by a tone filter.
// Best on single notes / palm-muted riffs. Zero latency.
struct LowOctaveBlender
{
    double sr = 48000;
    Biquad trackLp, toneLp, dc;
    Env env[kMaxCh];
    float ff[kMaxCh] { 1.f, 1.f };
    bool pos[kMaxCh] { false, false };
    float level = 0.f;

    void prepare (double s)
    {
        sr = s;
        for (auto& e : env) { e.set (sr, 1.f, 35.f); e.reset(); }
        trackLp.reset(); toneLp.reset(); dc.reset();
        dc.setHighpass (sr, 25.0, 0.7071);
        for (int c = 0; c < kMaxCh; ++c) { ff[c] = 1.f; pos[c] = false; }
        set (0.f, 180.f, 300.f);
    }
    void set (float blend, float toneHz, float trackHz)
    {
        level = blend * 2.2f;
        toneLp.setLowpass (sr, toneHz, 0.7071);
        trackLp.setLowpass (sr, trackHz, 0.9);
    }
    void process (float* x, int n)
    {
        for (int c = 0; c < n; ++c)
        {
            const float l = trackLp.process (x[c], c);
            const float e = env[c].process (std::fabs (l));
            const float th = std::max (0.2f * e, 2e-4f);

            if (! pos[c]) { if (l > th)  { pos[c] = true; ff[c] = -ff[c]; } }
            else if (l < -th)            { pos[c] = false; }

            float sub = toneLp.process (ff[c] * e * 1.4f, c);
            sub = dc.process (sub, c);
            x[c] += level * sub;
        }
    }
};

//==============================================================================
// 5) IMPEDANCE CURVE
// Models the pickup-inductance / cable-capacitance / amp-input-load resonance:
// a 2nd-order low-pass whose peak is adjustable. "Load" = how much of the
// curve is applied.
struct ImpedanceCurve
{
    double sr = 48000;
    Biquad lp;
    float mix = 1.f;

    void prepare (double s) { sr = s; lp.reset(); set (4500.f, 0.4f, 0.7f); }
    void set (float freq, float peak, float load)
    {
        lp.setLowpass (sr, freq, 0.5 + peak * 4.5);
        mix = load;
    }
    void process (float* x, int n)
    {
        for (int c = 0; c < n; ++c)
        {
            const float w = lp.process (x[c], c);
            x[c] += mix * (w - x[c]);
        }
    }
};

//==============================================================================
// 6) TONE TILT
// Complementary low/high shelves around an adjustable pivot.
struct ToneTilt
{
    double sr = 48000;
    Biquad lo, hi;

    void prepare (double s) { sr = s; lo.reset(); hi.reset(); set (700.f, 0.f); }
    void set (float pivot, float tiltDb)
    {
        lo.setLowShelf  (sr, pivot, -tiltDb);
        hi.setHighShelf (sr, pivot,  tiltDb);
    }
    void process (float* x, int n)
    {
        for (int c = 0; c < n; ++c) x[c] = hi.process (lo.process (x[c], c), c);
    }
};

//==============================================================================
// 7) SOUND DESTROYER
// Drive (tanh + wavefold) -> sample-rate reduction -> bit crush -> mix.
// Deliberately not oversampled: aliasing is the point, and it keeps latency 0.
struct SoundDestroyer
{
    double sr = 48000;
    float g = 1.f, trim = 1.f, fold = 0.f, mix = 0.f, ratio = 1.f, levels = 32768.f;
    bool crushOn = false;
    float ph[kMaxCh] {}, held[kMaxCh] {};

    void prepare (double s)
    {
        sr = s;
        for (int c = 0; c < kMaxCh; ++c) { ph[c] = 1.f; held[c] = 0.f; }
        set (0.3f, 0.2f, 0.3f);
    }
    void set (float drive, float crush, float m)
    {
        g = 1.f + drive * drive * 40.f;
        trim = 1.f / std::pow (g, 0.45f);
        fold = std::clamp ((drive - 0.4f) / 0.6f, 0.f, 1.f) * 0.6f;
        crushOn = crush > 0.002f;
        ratio = 1.f + crush * crush * 30.f;
        levels = std::pow (2.f, 16.f - crush * 13.f - 1.f);
        mix = m;
    }
    void process (float* x, int n)
    {
        for (int c = 0; c < n; ++c)
        {
            const float d = x[c] * g;
            const float s = std::tanh (d);
            float y = s + fold * (std::sin (d * 0.8f) - s);
            y *= trim;

            if (crushOn)
            {
                ph[c] += 1.f / ratio;
                if (ph[c] >= 1.f) { ph[c] -= 1.f; held[c] = y; }
                y = std::floor (held[c] * levels + 0.5f) / levels;
            }
            x[c] += mix * (y - x[c]);
        }
    }
};

} // namespace berry
