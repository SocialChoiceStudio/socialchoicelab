import numpy as np

import socialchoicelab as scl


def test_competition_run_trace_exports_round_data():
    competitors = np.array([[0.0], [3.0]])
    voters = np.array([[0.1], [0.2], [2.9]])
    cfg = scl.CompetitionEngineConfig(
        seat_count=1,
        seat_rule="plurality_top_k",
        max_rounds=2,
        step_config=scl.CompetitionStepConfig(kind="fixed", fixed_step_size=1.0),
    )
    with scl.competition_run(
        competitors,
        ["sticker", "sticker"],
        voters,
        dist_config=scl.DistanceConfig(salience_weights=[1.0]),
        engine_config=cfg,
    ) as trace:
        assert trace.dims() == (2, 2, 1)
        term = trace.termination()
        assert term["terminated_early"] is False
        assert term["reason"] == "max_rounds"
        round0 = trace.round_positions(0)
        assert round0.shape == (2, 1)
        assert np.allclose(round0[:, 0], [0.0, 3.0])
        shares = trace.round_vote_shares(0)
        assert np.allclose(shares, [2.0 / 3.0, 1.0 / 3.0])
        assert np.array_equal(trace.round_vote_totals(0), np.array([2, 1], dtype=np.int32))
        assert np.array_equal(trace.round_seat_totals(0), np.array([1, 0], dtype=np.int32))
        assert np.allclose(trace.final_vote_shares(), [2.0 / 3.0, 1.0 / 3.0])
        assert np.allclose(trace.final_seat_shares(), [1.0, 0.0])


def test_competition_run_experiment_exports_summary():
    competitors = np.array([[0.0], [3.0]])
    voters = np.array([[0.1], [0.2], [2.9]])
    cfg = scl.CompetitionEngineConfig(
        seat_count=1,
        seat_rule="plurality_top_k",
        max_rounds=3,
        step_config=scl.CompetitionStepConfig(kind="fixed", fixed_step_size=1.0),
        termination=scl.CompetitionTerminationConfig(
            stop_on_convergence=True,
            position_epsilon=1e-12,
        ),
    )
    with scl.competition_run_experiment(
        competitors,
        ["sticker", "sticker"],
        voters,
        dist_config=scl.DistanceConfig(salience_weights=[1.0]),
        engine_config=cfg,
        master_seed=2026,
        num_runs=3,
    ) as experiment:
        assert experiment.dims() == (3, 2, 1)
        summary = experiment.summary()
        assert summary["mean_rounds"] == 1.0
        assert summary["early_termination_rate"] == 1.0
        assert np.allclose(
            experiment.mean_final_vote_shares(),
            [2.0 / 3.0, 1.0 / 3.0],
        )
        assert np.all(experiment.run_round_counts() == np.array([1, 1, 1], dtype=np.int32))
        assert experiment.run_termination_reasons() == ["converged", "converged", "converged"]
        assert np.array_equal(
            experiment.run_terminated_early_flags(),
            np.array([True, True, True]),
        )
        assert np.allclose(
            experiment.run_final_vote_shares(),
            np.array([[2.0 / 3.0, 1.0 / 3.0]] * 3),
        )
        assert np.allclose(
            experiment.run_final_seat_shares(),
            np.array([[1.0, 0.0]] * 3),
        )
        assert np.allclose(
            experiment.run_final_positions(),
            np.array([[[0.0], [3.0]]] * 3),
        )
