"""Competition engine and experiment bindings."""

from __future__ import annotations

from dataclasses import dataclass, field

import numpy as np

from socialchoicelab._error import SCSError, _check, new_err_buf
from socialchoicelab._loader import _ffi, _lib
from socialchoicelab._types import DistanceConfig, _c_double_array, _c_int_array, _to_cffi_dist

__all__ = [
    "CompetitionStepConfig",
    "CompetitionTerminationConfig",
    "CompetitionEngineConfig",
    "CompetitionTrace",
    "CompetitionExperiment",
    "competition_run",
    "competition_run_experiment",
]

_ERR = 512

_STRATEGY_INT = {
    "sticker": 0,
    "hunter": 1,
    "aggregator": 2,
    "predator": 3,
}

_STEP_POLICY_INT = {
    "fixed": 0,
    "random_uniform": 1,
    "share_delta_proportional": 2,
}

_SEAT_RULE_INT = {
    "plurality_top_k": 0,
    "hare_largest_remainder": 1,
}

_BOUNDARY_POLICY_INT = {
    "project_to_box": 0,
    "stay_put": 1,
    "reflect": 2,
}

_OBJECTIVE_INT = {
    "vote_share": 0,
    "seat_share": 1,
}

_TERM_REASON_STR = {
    0: "max_rounds",
    1: "converged",
    2: "cycle_detected",
    3: "no_improvement_window",
}


@dataclass
class CompetitionStepConfig:
    kind: str = "fixed"
    fixed_step_size: float = 1.0
    min_step_size: float = 0.0
    max_step_size: float = 1.0
    proportionality_constant: float = 1.0


@dataclass
class CompetitionTerminationConfig:
    stop_on_convergence: bool = False
    position_epsilon: float = 1e-8
    stop_on_cycle: bool = False
    cycle_memory: int = 50
    signature_resolution: float = 1e-6
    stop_on_no_improvement: bool = False
    no_improvement_window: int = 10
    objective_epsilon: float = 1e-8


@dataclass
class CompetitionEngineConfig:
    seat_count: int = 1
    seat_rule: str = "plurality_top_k"
    step_config: CompetitionStepConfig = field(default_factory=CompetitionStepConfig)
    boundary_policy: str = "project_to_box"
    objective_kind: str = "vote_share"
    max_rounds: int = 10
    termination: CompetitionTerminationConfig = field(default_factory=CompetitionTerminationConfig)


def _to_cffi_step_config(cfg: CompetitionStepConfig):
    kind = cfg.kind.lower()
    if kind not in _STEP_POLICY_INT:
        raise ValueError(f"Unknown competition step kind {cfg.kind!r}.")
    out = _ffi.new("SCS_CompetitionStepConfig *")
    out.kind = _STEP_POLICY_INT[kind]
    out.fixed_step_size = float(cfg.fixed_step_size)
    out.min_step_size = float(cfg.min_step_size)
    out.max_step_size = float(cfg.max_step_size)
    out.proportionality_constant = float(cfg.proportionality_constant)
    return out


def _to_cffi_term_config(cfg: CompetitionTerminationConfig):
    out = _ffi.new("SCS_CompetitionTerminationConfig *")
    out.stop_on_convergence = int(bool(cfg.stop_on_convergence))
    out.position_epsilon = float(cfg.position_epsilon)
    out.stop_on_cycle = int(bool(cfg.stop_on_cycle))
    out.cycle_memory = int(cfg.cycle_memory)
    out.signature_resolution = float(cfg.signature_resolution)
    out.stop_on_no_improvement = int(bool(cfg.stop_on_no_improvement))
    out.no_improvement_window = int(cfg.no_improvement_window)
    out.objective_epsilon = float(cfg.objective_epsilon)
    return out


def _to_cffi_engine_config(cfg: CompetitionEngineConfig):
    seat_rule = cfg.seat_rule.lower()
    if seat_rule not in _SEAT_RULE_INT:
        raise ValueError(f"Unknown competition seat rule {cfg.seat_rule!r}.")
    boundary_policy = cfg.boundary_policy.lower()
    if boundary_policy not in _BOUNDARY_POLICY_INT:
        raise ValueError(f"Unknown competition boundary policy {cfg.boundary_policy!r}.")
    objective_kind = cfg.objective_kind.lower()
    if objective_kind not in _OBJECTIVE_INT:
        raise ValueError(f"Unknown competition objective kind {cfg.objective_kind!r}.")
    out = _ffi.new("SCS_CompetitionEngineConfig *")
    step = _to_cffi_step_config(cfg.step_config)
    term = _to_cffi_term_config(cfg.termination)
    out.seat_count = int(cfg.seat_count)
    out.seat_rule = _SEAT_RULE_INT[seat_rule]
    out.step_config = step[0]
    out.boundary_policy = _BOUNDARY_POLICY_INT[boundary_policy]
    out.objective_kind = _OBJECTIVE_INT[objective_kind]
    out.max_rounds = int(cfg.max_rounds)
    out.termination = term[0]
    return out, [step, term]


def _strategy_buffer(strategy_kinds) -> tuple:
    ints = []
    for kind in strategy_kinds:
        key = str(kind).lower()
        if key not in _STRATEGY_INT:
            raise ValueError(f"Unknown competition strategy kind {kind!r}.")
        ints.append(_STRATEGY_INT[key])
    return _c_int_array(ints)


def _as_position_array(values, n_dims: int | None = None) -> tuple[np.ndarray, int, int]:
    arr = np.asarray(values, dtype=np.float64)
    if arr.ndim == 1:
        if n_dims is None:
            raise ValueError("Flat position arrays require an explicit n_dims.")
        if len(arr) % n_dims != 0:
            raise ValueError("Position array length must be divisible by n_dims.")
        flat = arr
        n_points = len(arr) // n_dims
        return flat, n_points, n_dims
    if arr.ndim == 2:
        return arr.ravel(), arr.shape[0], arr.shape[1]
    raise ValueError("Positions must be a 1D flat array or a 2D array.")


class CompetitionTrace:
    def __init__(self, ptr) -> None:
        if ptr == _ffi.NULL:
            raise SCSError("Received a null SCS_CompetitionTrace pointer.")
        self._ptr = ptr
        self._destroyed = False

    def __del__(self) -> None:
        self._destroy()

    def __enter__(self) -> "CompetitionTrace":
        return self

    def __exit__(self, *_) -> None:
        self._destroy()

    def _destroy(self) -> None:
        if not self._destroyed and self._ptr != _ffi.NULL:
            _lib.scs_competition_trace_destroy(self._ptr)
            self._ptr = _ffi.NULL
            self._destroyed = True

    def dims(self) -> tuple[int, int, int]:
        out_rounds = _ffi.new("int *")
        out_competitors = _ffi.new("int *")
        out_dims = _ffi.new("int *")
        err = new_err_buf()
        _check(
            _lib.scs_competition_trace_dims(
                self._ptr, out_rounds, out_competitors, out_dims, err, _ERR
            ),
            err,
        )
        return int(out_rounds[0]), int(out_competitors[0]), int(out_dims[0])

    def termination(self) -> dict:
        out_early = _ffi.new("int *")
        out_reason = _ffi.new("SCS_CompetitionTerminationReason *")
        err = new_err_buf()
        _check(
            _lib.scs_competition_trace_termination(
                self._ptr, out_early, out_reason, err, _ERR
            ),
            err,
        )
        reason = int(_ffi.cast("int", out_reason[0]))
        return {
            "terminated_early": bool(out_early[0]),
            "reason": _TERM_REASON_STR.get(reason, f"unknown({reason})"),
        }

    def round_positions(self, round_index: int) -> np.ndarray:
        n_rounds, n_competitors, n_dims = self.dims()
        if round_index < 0 or round_index >= n_rounds:
            raise IndexError(
                f"round_positions: round index {round_index} is out of range [0, {n_rounds - 1}]."
            )
        buf = _ffi.new("double[]", n_competitors * n_dims)
        err = new_err_buf()
        _check(
            _lib.scs_competition_trace_round_positions(
                self._ptr, int(round_index), buf, n_competitors * n_dims, err, _ERR
            ),
            err,
        )
        return np.frombuffer(_ffi.buffer(buf, (n_competitors * n_dims) * 8), dtype=np.float64).reshape(
            n_competitors, n_dims
        ).copy()

    def final_positions(self) -> np.ndarray:
        _, n_competitors, n_dims = self.dims()
        buf = _ffi.new("double[]", n_competitors * n_dims)
        err = new_err_buf()
        _check(
            _lib.scs_competition_trace_final_positions(
                self._ptr, buf, n_competitors * n_dims, err, _ERR
            ),
            err,
        )
        return np.frombuffer(_ffi.buffer(buf, (n_competitors * n_dims) * 8), dtype=np.float64).reshape(
            n_competitors, n_dims
        ).copy()

    def round_vote_shares(self, round_index: int) -> np.ndarray:
        n_rounds, n_competitors, _ = self.dims()
        if round_index < 0 or round_index >= n_rounds:
            raise IndexError(
                f"round_vote_shares: round index {round_index} is out of range [0, {n_rounds - 1}]."
            )
        buf = _ffi.new("double[]", n_competitors)
        err = new_err_buf()
        _check(
            _lib.scs_competition_trace_round_vote_shares(
                self._ptr, int(round_index), buf, n_competitors, err, _ERR
            ),
            err,
        )
        return np.frombuffer(_ffi.buffer(buf, n_competitors * 8), dtype=np.float64).copy()

    def round_seat_shares(self, round_index: int) -> np.ndarray:
        n_rounds, n_competitors, _ = self.dims()
        if round_index < 0 or round_index >= n_rounds:
            raise IndexError(
                f"round_seat_shares: round index {round_index} is out of range [0, {n_rounds - 1}]."
            )
        buf = _ffi.new("double[]", n_competitors)
        err = new_err_buf()
        _check(
            _lib.scs_competition_trace_round_seat_shares(
                self._ptr, int(round_index), buf, n_competitors, err, _ERR
            ),
            err,
        )
        return np.frombuffer(_ffi.buffer(buf, n_competitors * 8), dtype=np.float64).copy()

    def round_vote_totals(self, round_index: int) -> np.ndarray:
        n_rounds, n_competitors, _ = self.dims()
        if round_index < 0 or round_index >= n_rounds:
            raise IndexError(
                f"round_vote_totals: round index {round_index} is out of range [0, {n_rounds - 1}]."
            )
        buf = _ffi.new("int[]", n_competitors)
        err = new_err_buf()
        _check(
            _lib.scs_competition_trace_round_vote_totals(
                self._ptr, int(round_index), buf, n_competitors, err, _ERR
            ),
            err,
        )
        return np.frombuffer(_ffi.buffer(buf, n_competitors * 4), dtype=np.int32).copy()

    def round_seat_totals(self, round_index: int) -> np.ndarray:
        n_rounds, n_competitors, _ = self.dims()
        if round_index < 0 or round_index >= n_rounds:
            raise IndexError(
                f"round_seat_totals: round index {round_index} is out of range [0, {n_rounds - 1}]."
            )
        buf = _ffi.new("int[]", n_competitors)
        err = new_err_buf()
        _check(
            _lib.scs_competition_trace_round_seat_totals(
                self._ptr, int(round_index), buf, n_competitors, err, _ERR
            ),
            err,
        )
        return np.frombuffer(_ffi.buffer(buf, n_competitors * 4), dtype=np.int32).copy()

    def final_vote_shares(self) -> np.ndarray:
        _, n_competitors, _ = self.dims()
        buf = _ffi.new("double[]", n_competitors)
        err = new_err_buf()
        _check(
            _lib.scs_competition_trace_final_vote_shares(
                self._ptr, buf, n_competitors, err, _ERR
            ),
            err,
        )
        return np.frombuffer(_ffi.buffer(buf, n_competitors * 8), dtype=np.float64).copy()

    def final_seat_shares(self) -> np.ndarray:
        _, n_competitors, _ = self.dims()
        buf = _ffi.new("double[]", n_competitors)
        err = new_err_buf()
        _check(
            _lib.scs_competition_trace_final_seat_shares(
                self._ptr, buf, n_competitors, err, _ERR
            ),
            err,
        )
        return np.frombuffer(_ffi.buffer(buf, n_competitors * 8), dtype=np.float64).copy()


class CompetitionExperiment:
    def __init__(self, ptr) -> None:
        if ptr == _ffi.NULL:
            raise SCSError("Received a null SCS_CompetitionExperiment pointer.")
        self._ptr = ptr
        self._destroyed = False

    def __del__(self) -> None:
        self._destroy()

    def __enter__(self) -> "CompetitionExperiment":
        return self

    def __exit__(self, *_) -> None:
        self._destroy()

    def _destroy(self) -> None:
        if not self._destroyed and self._ptr != _ffi.NULL:
            _lib.scs_competition_experiment_destroy(self._ptr)
            self._ptr = _ffi.NULL
            self._destroyed = True

    def dims(self) -> tuple[int, int, int]:
        out_runs = _ffi.new("int *")
        out_competitors = _ffi.new("int *")
        out_dims = _ffi.new("int *")
        err = new_err_buf()
        _check(
            _lib.scs_competition_experiment_dims(
                self._ptr, out_runs, out_competitors, out_dims, err, _ERR
            ),
            err,
        )
        return int(out_runs[0]), int(out_competitors[0]), int(out_dims[0])

    def summary(self) -> dict:
        mean_rounds = _ffi.new("double *")
        early_rate = _ffi.new("double *")
        err = new_err_buf()
        _check(
            _lib.scs_competition_experiment_summary(
                self._ptr, mean_rounds, early_rate, err, _ERR
            ),
            err,
        )
        return {
            "mean_rounds": float(mean_rounds[0]),
            "early_termination_rate": float(early_rate[0]),
        }

    def mean_final_vote_shares(self) -> np.ndarray:
        _, n_competitors, _ = self.dims()
        buf = _ffi.new("double[]", n_competitors)
        err = new_err_buf()
        _check(
            _lib.scs_competition_experiment_mean_final_vote_shares(
                self._ptr, buf, n_competitors, err, _ERR
            ),
            err,
        )
        return np.frombuffer(_ffi.buffer(buf, n_competitors * 8), dtype=np.float64).copy()

    def mean_final_seat_shares(self) -> np.ndarray:
        _, n_competitors, _ = self.dims()
        buf = _ffi.new("double[]", n_competitors)
        err = new_err_buf()
        _check(
            _lib.scs_competition_experiment_mean_final_seat_shares(
                self._ptr, buf, n_competitors, err, _ERR
            ),
            err,
        )
        return np.frombuffer(_ffi.buffer(buf, n_competitors * 8), dtype=np.float64).copy()

    def run_round_counts(self) -> np.ndarray:
        n_runs, _, _ = self.dims()
        buf = _ffi.new("int[]", n_runs)
        err = new_err_buf()
        _check(
            _lib.scs_competition_experiment_run_round_counts(
                self._ptr, buf, n_runs, err, _ERR
            ),
            err,
        )
        return np.frombuffer(_ffi.buffer(buf, n_runs * 4), dtype=np.int32).copy()

    def run_termination_reasons(self) -> list[str]:
        n_runs, _, _ = self.dims()
        buf = _ffi.new("SCS_CompetitionTerminationReason[]", n_runs)
        err = new_err_buf()
        _check(
            _lib.scs_competition_experiment_run_termination_reasons(
                self._ptr, buf, n_runs, err, _ERR
            ),
            err,
        )
        return [
            _TERM_REASON_STR.get(int(_ffi.cast("int", buf[i])), f"unknown({int(_ffi.cast('int', buf[i]))})")
            for i in range(n_runs)
        ]

    def run_terminated_early_flags(self) -> np.ndarray:
        n_runs, _, _ = self.dims()
        buf = _ffi.new("int[]", n_runs)
        err = new_err_buf()
        _check(
            _lib.scs_competition_experiment_run_terminated_early_flags(
                self._ptr, buf, n_runs, err, _ERR
            ),
            err,
        )
        return np.frombuffer(_ffi.buffer(buf, n_runs * 4), dtype=np.int32).astype(bool)

    def run_final_vote_shares(self) -> np.ndarray:
        n_runs, n_competitors, _ = self.dims()
        buf = _ffi.new("double[]", n_runs * n_competitors)
        err = new_err_buf()
        _check(
            _lib.scs_competition_experiment_run_final_vote_shares(
                self._ptr, buf, n_runs * n_competitors, err, _ERR
            ),
            err,
        )
        return np.frombuffer(_ffi.buffer(buf, n_runs * n_competitors * 8), dtype=np.float64).reshape(
            n_runs, n_competitors
        ).copy()

    def run_final_seat_shares(self) -> np.ndarray:
        n_runs, n_competitors, _ = self.dims()
        buf = _ffi.new("double[]", n_runs * n_competitors)
        err = new_err_buf()
        _check(
            _lib.scs_competition_experiment_run_final_seat_shares(
                self._ptr, buf, n_runs * n_competitors, err, _ERR
            ),
            err,
        )
        return np.frombuffer(_ffi.buffer(buf, n_runs * n_competitors * 8), dtype=np.float64).reshape(
            n_runs, n_competitors
        ).copy()

    def run_final_positions(self) -> np.ndarray:
        n_runs, n_competitors, n_dims = self.dims()
        buf = _ffi.new("double[]", n_runs * n_competitors * n_dims)
        err = new_err_buf()
        _check(
            _lib.scs_competition_experiment_run_final_positions(
                self._ptr, buf, n_runs * n_competitors * n_dims, err, _ERR
            ),
            err,
        )
        return np.frombuffer(
            _ffi.buffer(buf, n_runs * n_competitors * n_dims * 8), dtype=np.float64
        ).reshape(n_runs, n_competitors, n_dims).copy()


def competition_run(
    competitor_positions,
    strategy_kinds,
    voter_ideals,
    competitor_headings=None,
    dist_config: DistanceConfig | None = None,
    engine_config: CompetitionEngineConfig | None = None,
    stream_manager=None,
) -> CompetitionTrace:
    if dist_config is None:
        dist_config = DistanceConfig(salience_weights=[1.0])
    if engine_config is None:
        engine_config = CompetitionEngineConfig()
    pos_flat, n_competitors, n_dims = _as_position_array(competitor_positions)
    voters_flat, n_voters, voter_dims = _as_position_array(voter_ideals)
    if voter_dims != n_dims:
        raise ValueError("competitor_positions and voter_ideals must have the same dimension.")
    if len(strategy_kinds) != n_competitors:
        raise ValueError("strategy_kinds length must match the number of competitors.")
    pos_buf, pos_ka = _c_double_array(pos_flat)
    strat_buf, strat_ka = _strategy_buffer(strategy_kinds)
    voter_buf, voter_ka = _c_double_array(voters_flat)
    if competitor_headings is None:
        heading_ptr = _ffi.NULL
        heading_ka = None
    else:
        headings_flat, heading_points, heading_dims = _as_position_array(competitor_headings, n_dims=n_dims)
        if heading_points != n_competitors or heading_dims != n_dims:
            raise ValueError("competitor_headings must match competitor_positions shape.")
        heading_ptr, heading_ka = _c_double_array(headings_flat)
    dist_cfg, dist_ka = _to_cffi_dist(dist_config, n_dims=n_dims)
    engine_cfg, engine_ka = _to_cffi_engine_config(engine_config)
    mgr = _ffi.NULL if stream_manager is None else stream_manager._handle()
    err = new_err_buf()
    ptr = _lib.scs_competition_run(
        pos_buf,
        heading_ptr,
        strat_buf,
        int(n_competitors),
        voter_buf,
        int(n_voters),
        int(n_dims),
        dist_cfg,
        engine_cfg,
        mgr,
        err,
        _ERR,
    )
    if ptr == _ffi.NULL:
        from socialchoicelab._error import _ffi_string

        raise SCSError(f"scs_competition_run failed: {_ffi_string(err)}")
    return CompetitionTrace(ptr)


def competition_run_experiment(
    competitor_positions,
    strategy_kinds,
    voter_ideals,
    competitor_headings=None,
    dist_config: DistanceConfig | None = None,
    engine_config: CompetitionEngineConfig | None = None,
    master_seed: int = 1,
    num_runs: int = 10,
    retain_traces: bool = False,
) -> CompetitionExperiment:
    if dist_config is None:
        dist_config = DistanceConfig(salience_weights=[1.0])
    if engine_config is None:
        engine_config = CompetitionEngineConfig()
    pos_flat, n_competitors, n_dims = _as_position_array(competitor_positions)
    voters_flat, n_voters, voter_dims = _as_position_array(voter_ideals)
    if voter_dims != n_dims:
        raise ValueError("competitor_positions and voter_ideals must have the same dimension.")
    if len(strategy_kinds) != n_competitors:
        raise ValueError("strategy_kinds length must match the number of competitors.")
    pos_buf, pos_ka = _c_double_array(pos_flat)
    strat_buf, strat_ka = _strategy_buffer(strategy_kinds)
    voter_buf, voter_ka = _c_double_array(voters_flat)
    if competitor_headings is None:
        heading_ptr = _ffi.NULL
        heading_ka = None
    else:
        headings_flat, heading_points, heading_dims = _as_position_array(competitor_headings, n_dims=n_dims)
        if heading_points != n_competitors or heading_dims != n_dims:
            raise ValueError("competitor_headings must match competitor_positions shape.")
        heading_ptr, heading_ka = _c_double_array(headings_flat)
    dist_cfg, dist_ka = _to_cffi_dist(dist_config, n_dims=n_dims)
    engine_cfg, engine_ka = _to_cffi_engine_config(engine_config)
    err = new_err_buf()
    ptr = _lib.scs_competition_run_experiment(
        pos_buf,
        heading_ptr,
        strat_buf,
        int(n_competitors),
        voter_buf,
        int(n_voters),
        int(n_dims),
        dist_cfg,
        engine_cfg,
        int(master_seed),
        int(num_runs),
        int(bool(retain_traces)),
        err,
        _ERR,
    )
    if ptr == _ffi.NULL:
        from socialchoicelab._error import _ffi_string

        raise SCSError(f"scs_competition_run_experiment failed: {_ffi_string(err)}")
    return CompetitionExperiment(ptr)
