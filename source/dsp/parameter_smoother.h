#pragma once
// ParameterSmoother: sample-accurate linear-ramp smoother.
// When a VST3 parameter is modulated by DAW automation or an LFO, applying
// a step change per block produces audible zipper noise. This class ramps
// the value linearly sample-by-sample toward the target, keeping the
// per-sample step below maxStepPerSample so the change is imperceptible.
//
// Implementation: KISS design. Stores current_, target_, and maxStep_.
// next() advances current_ by at most maxStep_ toward target_ and returns
// the new current_. If maxStep_ <= 0 the value jumps instantly to target_.
// reset() snaps current_ to a value without ramping (target_ is unchanged).
//
namespace netsdr {

class ParameterSmoother {
public:
    // Constructor. maxStepPerSample is the max absolute change per sample.
    // A value <= 0 means "no limit" (jump instantly to target).
    explicit ParameterSmoother(double maxStepPerSample = 0.0);

    // Snap current value to `value` without ramp (e.g. on init / after
    // setupProcessing). Does not change the target.
    void reset(double value);

    // Set the value to ramp toward. Does not change current_ immediately.
    void setTarget(double target);

    // Return the current value, then advance one sample toward the target
    // by at most maxStepPerSample. When current_ reaches target_, it stays
    // there indefinitely.
    //
    // Behaviour:
    // - If maxStep_ <= 0: current_ jumps directly to target_ and that value
    //   is returned (instant snap, no ramp).
    // - If maxStep_ > 0: the value _before_ the advance is returned, then
    //   current_ is moved toward target_ by at most maxStep_. This is the
    //  "return current, then advance" pattern documented in the implementation notes.
    double next();

    // Current sample value (after the most recent next() advance, or the
    // value set by reset()/setTarget()).
    double value() const;

    // True when current_ exactly equals target_.
    bool isSettled() const;

private:
    double current_ = 0.0;
    double target_  = 0.0;
    double maxStep_ = 0.0;
};

} // namespace netsdr