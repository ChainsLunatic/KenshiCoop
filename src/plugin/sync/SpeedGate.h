// SpeedGate - the ONE policy that decides game-speed / pause AUTHORITY in a
// co-op session, factored out of Replicator::syncSpeed so the unit layer
// (prototest) can lock it directly.
//
// WHY THIS EXISTS (host-only speed/pause, 2026-07-21):
// The original speed layer was a CONSENSUS model: both peers captured their
// local speed/pause "vote" and the host applied effective = min(hostReq,
// joinReq) (capped at 1x while either squad fights). That let a JOIN pause or
// slow the WHOLE session for everyone - a join pressing space (joinReq == 0)
// dragged the min() to 0 and froze the host too. The design goal changed:
// ONLY the host may change game speed or pause.
//
// The fix has two halves, and this header owns the pure decisions behind them:
//   1. effectiveHostSpeed(): the host's broadcast effective is now derived from
//      the HOST's own request ALONE - the join's request no longer lowers it.
//      The automatic combat cap (force 1x while a tracked squad fights) is a
//      balance rule, NOT a player changing speed, so it is preserved and still
//      honours the join's reported combat bit.
//   2. shouldDenySpeedInput(): a non-host that actually acted on speed/pause is
//      told "no" - its local sim is snapped back to the host's effective by the
//      replicator's continuous enforcement, and this predicate gates the
//      "only the host can change game speed" toast so the join gets feedback
//      instead of a silent revert.
//
// Pure inline C++03, zero game/logger/wire dependency (matches LoadGate.h /
// ChangeGate.h / EngineCaps.h). Speeds are the engine's frame-speed multiplier;
// 0 (<= EPS) means paused, the same wire convention the SpeedPacket uses.

#ifndef KENSHICOOP_SPEED_GATE_H
#define KENSHICOOP_SPEED_GATE_H

namespace coop {
namespace sync {

// The effective speed the HOST applies locally and broadcasts as PKT_SPEED_SET.
//   hostReq : the host's own current request (multiplier; 0 == paused;
//             < 0 == not sampled yet -> treated as 1x normal speed).
//   combat  : the arbitration combat flag (own OR peer squad fighting). It caps
//             the speed to 1x but NEVER force-unpauses: a pause is 0, already
//             below 1, so the cap leaves it untouched.
// Host-only authority: unlike the old consensus path this NEVER folds in the
// join's request, so a join can no longer pause or slow the shared session.
inline float effectiveHostSpeed(float hostReq, bool combat) {
    float eff = (hostReq >= 0.0f) ? hostReq : 1.0f; // no request yet -> normal
    if (combat && eff > 1.0f) eff = 1.0f;           // combat cap (pause survives)
    return eff;
}

// Should this client's local speed/pause action be DENIED (not authoritative)?
// True only for a non-host that actually acted this tick. The host is always
// authoritative (never denied); a join that merely received a SET or did
// nothing this tick did not "act" and gets no denial. Drives the informational
// toast; the actual revert is the replicator's continuous enforcement.
inline bool shouldDenySpeedInput(bool isHost, bool userActed) {
    return !isHost && userActed;
}

} // namespace sync
} // namespace coop

#endif // KENSHICOOP_SPEED_GATE_H
