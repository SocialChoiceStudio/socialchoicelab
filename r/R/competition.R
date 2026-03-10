#' CompetitionTrace: one competition simulation trace
#'
#' Wraps the \code{SCS_CompetitionTrace} C handle. Create traces with
#' \code{\link{competition_run}}.
#'
#' @export
CompetitionTrace <- R6::R6Class(
  "CompetitionTrace",
  private = list(
    ptr = NULL,
    finalize = function() {
      .Call("r_scs_competition_trace_destroy", private$ptr,
            PACKAGE = "socialchoicelab")
    }
  ),
  public = list(
    #' @description Internal. Use \code{\link{competition_run}} instead.
    #' @param ptr External pointer returned by a C factory function.
    initialize = function(ptr) {
      private$ptr <- ptr
      invisible(self)
    },

    #' @description Return basic trace dimensions.
    #' @return Named list with \code{n_rounds}, \code{n_competitors}, and \code{n_dims}.
    dims = function() {
      .Call("r_scs_competition_trace_dims", private$ptr,
            PACKAGE = "socialchoicelab")
    },

    #' @description Return the trace termination status.
    #' @return Named list with \code{terminated_early} and \code{reason}.
    termination = function() {
      out <- .Call("r_scs_competition_trace_termination", private$ptr,
                   PACKAGE = "socialchoicelab")
      out$reason <- .competition_trace_reason(out$reason)
      out
    },

    #' @description Return competitor positions for a given round.
    #' @param round 1-based round index.
    #' @return Numeric matrix with one row per competitor and one column per dimension.
    round_positions = function(round) {
      d <- self$dims()
      round <- as.integer(round)
      if (round < 1L || round > d$n_rounds) {
        stop("round_positions: round ", round, " is out of range [1, ",
             d$n_rounds, "].")
      }
      .Call("r_scs_competition_trace_round_positions", private$ptr, round - 1L,
            PACKAGE = "socialchoicelab")
    },

    #' @description Return final competitor positions after the last move.
    #' @return Numeric matrix with one row per competitor and one column per dimension.
    final_positions = function() {
      .Call("r_scs_competition_trace_final_positions", private$ptr,
            PACKAGE = "socialchoicelab")
    },

    #' @description Return vote shares for a given round.
    #' @param round 1-based round index.
    #' @return Named numeric vector of vote shares by competitor.
    round_vote_shares = function(round) {
      d <- self$dims()
      round <- as.integer(round)
      if (round < 1L || round > d$n_rounds) {
        stop("round_vote_shares: round ", round, " is out of range [1, ",
             d$n_rounds, "].")
      }
      .Call("r_scs_competition_trace_round_vote_shares", private$ptr, round - 1L,
            PACKAGE = "socialchoicelab")
    },

    #' @description Return seat shares for a given round.
    #' @param round 1-based round index.
    #' @return Named numeric vector of seat shares by competitor.
    round_seat_shares = function(round) {
      d <- self$dims()
      round <- as.integer(round)
      if (round < 1L || round > d$n_rounds) {
        stop("round_seat_shares: round ", round, " is out of range [1, ",
             d$n_rounds, "].")
      }
      .Call("r_scs_competition_trace_round_seat_shares", private$ptr, round - 1L,
            PACKAGE = "socialchoicelab")
    },

    #' @description Return integer vote totals for a given round.
    #' @param round 1-based round index.
    #' @return Named integer vector of vote totals by competitor.
    round_vote_totals = function(round) {
      d <- self$dims()
      round <- as.integer(round)
      if (round < 1L || round > d$n_rounds) {
        stop("round_vote_totals: round ", round, " is out of range [1, ",
             d$n_rounds, "].")
      }
      .Call("r_scs_competition_trace_round_vote_totals", private$ptr, round - 1L,
            PACKAGE = "socialchoicelab")
    },

    #' @description Return integer seat totals for a given round.
    #' @param round 1-based round index.
    #' @return Named integer vector of seat totals by competitor.
    round_seat_totals = function(round) {
      d <- self$dims()
      round <- as.integer(round)
      if (round < 1L || round > d$n_rounds) {
        stop("round_seat_totals: round ", round, " is out of range [1, ",
             d$n_rounds, "].")
      }
      .Call("r_scs_competition_trace_round_seat_totals", private$ptr, round - 1L,
            PACKAGE = "socialchoicelab")
    },

    #' @description Return final vote shares.
    #' @return Named numeric vector of vote shares by competitor.
    final_vote_shares = function() {
      .Call("r_scs_competition_trace_final_vote_shares", private$ptr,
            PACKAGE = "socialchoicelab")
    },

    #' @description Return final seat shares.
    #' @return Named numeric vector of seat shares by competitor.
    final_seat_shares = function() {
      .Call("r_scs_competition_trace_final_seat_shares", private$ptr,
            PACKAGE = "socialchoicelab")
    }
  )
)

#' CompetitionExperiment: replicated competition runs
#'
#' Wraps the \code{SCS_CompetitionExperiment} C handle. Create experiments with
#' \code{\link{competition_run_experiment}}.
#'
#' @export
CompetitionExperiment <- R6::R6Class(
  "CompetitionExperiment",
  private = list(
    ptr = NULL,
    finalize = function() {
      .Call("r_scs_competition_experiment_destroy", private$ptr,
            PACKAGE = "socialchoicelab")
    }
  ),
  public = list(
    #' @description Internal. Use \code{\link{competition_run_experiment}} instead.
    #' @param ptr External pointer returned by a C factory function.
    initialize = function(ptr) {
      private$ptr <- ptr
      invisible(self)
    },

    #' @description Return basic experiment dimensions.
    #' @return Named list with \code{n_runs}, \code{n_competitors}, and \code{n_dims}.
    dims = function() {
      .Call("r_scs_competition_experiment_dims", private$ptr,
            PACKAGE = "socialchoicelab")
    },

    #' @description Return summary statistics across experiment runs.
    #' @return Named list with summary metrics such as mean rounds and early termination rate.
    summary = function() {
      .Call("r_scs_competition_experiment_summary", private$ptr,
            PACKAGE = "socialchoicelab")
    },

    #' @description Return mean final vote shares across runs.
    #' @return Named numeric vector of mean final vote shares by competitor.
    mean_final_vote_shares = function() {
      .Call("r_scs_competition_experiment_mean_final_vote_shares", private$ptr,
            PACKAGE = "socialchoicelab")
    },

    #' @description Return mean final seat shares across runs.
    #' @return Named numeric vector of mean final seat shares by competitor.
    mean_final_seat_shares = function() {
      .Call("r_scs_competition_experiment_mean_final_seat_shares", private$ptr,
            PACKAGE = "socialchoicelab")
    },

    #' @description Return the realized round count for each run.
    #' @return Integer vector of round counts.
    run_round_counts = function() {
      .Call("r_scs_competition_experiment_run_round_counts", private$ptr,
            PACKAGE = "socialchoicelab")
    },

    #' @description Return the termination reason for each run.
    #' @return Character vector of termination reason labels.
    run_termination_reasons = function() {
      codes <- .Call("r_scs_competition_experiment_run_termination_reasons", private$ptr,
                     PACKAGE = "socialchoicelab")
      vapply(codes, .competition_trace_reason, character(1))
    },

    #' @description Return whether each run terminated early.
    #' @return Logical vector.
    run_terminated_early_flags = function() {
      .Call("r_scs_competition_experiment_run_terminated_early_flags", private$ptr,
            PACKAGE = "socialchoicelab")
    },

    #' @description Return final vote shares for each run.
    #' @return Numeric matrix with rows as runs and columns as competitors.
    run_final_vote_shares = function() {
      .Call("r_scs_competition_experiment_run_final_vote_shares", private$ptr,
            PACKAGE = "socialchoicelab")
    },

    #' @description Return final seat shares for each run.
    #' @return Numeric matrix with rows as runs and columns as competitors.
    run_final_seat_shares = function() {
      .Call("r_scs_competition_experiment_run_final_seat_shares", private$ptr,
            PACKAGE = "socialchoicelab")
    },

    #' @description Return final competitor positions for each run.
    #' @return Numeric 3D array with dimensions run x competitor x dimension.
    run_final_positions = function() {
      .Call("r_scs_competition_experiment_run_final_positions", private$ptr,
            PACKAGE = "socialchoicelab")
    }
  )
)

#' Run a spatial candidate competition simulation
#'
#' @param competitor_positions Numeric vector of length
#'   \code{n_competitors * n_dims}, row-major by competitor.
#' @param strategy_kinds Character vector. Entries are \code{"sticker"},
#'   \code{"hunter"}, \code{"aggregator"}, or \code{"predator"}.
#' @param voter_ideals Numeric vector of length \code{n_voters * n_dims}.
#' @param competitor_headings Optional numeric vector matching
#'   \code{competitor_positions}, or \code{NULL}.
#' @param dist_config Distance configuration from \code{\link{make_dist_config}}.
#' @param engine_config Competition engine configuration from
#'   \code{\link{make_competition_engine_config}}.
#' @param stream_manager Optional \code{\link{StreamManager}} for stochastic
#'   strategies and step policies.
#' @return A \code{\link{CompetitionTrace}} object.
#' @export
competition_run <- function(competitor_positions,
                            strategy_kinds,
                            voter_ideals,
                            competitor_headings = NULL,
                            dist_config = make_dist_config(n_dims = 1L),
                            engine_config = make_competition_engine_config(),
                            stream_manager = NULL) {
  strategy_ints <- vapply(strategy_kinds, function(kind) {
    switch(kind,
      sticker = 0L,
      hunter = 1L,
      aggregator = 2L,
      predator = 3L,
      stop(
        "competition_run: unknown strategy kind '", kind, "'. ",
        "Choose from: sticker, hunter, aggregator, predator."
      )
    )
  }, integer(1))
  CompetitionTrace$new(.Call(
    "r_scs_competition_run",
    as.double(competitor_positions),
    if (is.null(competitor_headings)) NULL else as.double(competitor_headings),
    as.integer(strategy_ints),
    as.double(voter_ideals),
    dist_config,
    engine_config,
    .sm_ptr_optional(stream_manager),
    PACKAGE = "socialchoicelab"
  ))
}

#' Run replicated competition simulations
#'
#' @param competitor_positions,strategy_kinds,voter_ideals,competitor_headings
#'   See \code{\link{competition_run}}.
#' @param dist_config,engine_config See \code{\link{competition_run}}.
#' @param master_seed Numeric scalar used to derive per-run seeds.
#' @param num_runs Integer number of replications.
#' @param retain_traces Logical. Whether to retain full per-run traces inside
#'   the C++ experiment result.
#' @return A \code{\link{CompetitionExperiment}} object.
#' @export
competition_run_experiment <- function(
    competitor_positions,
    strategy_kinds,
    voter_ideals,
    competitor_headings = NULL,
    dist_config = make_dist_config(n_dims = 1L),
    engine_config = make_competition_engine_config(),
    master_seed = 1,
    num_runs = 10L,
    retain_traces = FALSE) {
  strategy_ints <- vapply(strategy_kinds, function(kind) {
    switch(kind,
      sticker = 0L,
      hunter = 1L,
      aggregator = 2L,
      predator = 3L,
      stop(
        "competition_run_experiment: unknown strategy kind '", kind, "'. ",
        "Choose from: sticker, hunter, aggregator, predator."
      )
    )
  }, integer(1))
  CompetitionExperiment$new(.Call(
    "r_scs_competition_run_experiment",
    as.double(competitor_positions),
    if (is.null(competitor_headings)) NULL else as.double(competitor_headings),
    as.integer(strategy_ints),
    as.double(voter_ideals),
    dist_config,
    engine_config,
    as.double(master_seed),
    as.integer(num_runs),
    as.logical(retain_traces),
    PACKAGE = "socialchoicelab"
  ))
}
