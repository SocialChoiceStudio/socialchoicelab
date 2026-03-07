#' StreamManager: reproducible pseudo-random number streams
#'
#' Wraps the \code{SCS_StreamManager} C handle. All PRNG calls go through
#' named streams derived from a single master seed, enabling full
#' reproducibility. Register stream names before drawing from them.
#'
#' @examples
#' \dontrun{
#'   sm <- StreamManager$new(master_seed = 42, stream_names = "sim")
#'   sm$uniform_real("sim", 0, 1)
#'   sm$reset_all(42)
#' }
#' @importFrom R6 R6Class
#' @export
StreamManager <- R6::R6Class(
  "StreamManager",
  private = list(
    ptr = NULL,
    # Called automatically by R's GC; also invoked by the C-level finalizer.
    finalize = function() {
      .Call("r_scs_stream_manager_destroy", private$ptr,
            PACKAGE = "socialchoicelab")
    }
  ),
  public = list(

    #' @description Create a new StreamManager.
    #' @param master_seed Integer or double. Master seed (values up to 2^53
    #'   are represented exactly).
    #' @param stream_names Optional character vector. Stream names to register
    #'   immediately after creation.
    initialize = function(master_seed, stream_names = NULL) {
      private$ptr <- .Call("r_scs_stream_manager_create",
                           as.double(master_seed),
                           PACKAGE = "socialchoicelab")
      if (!is.null(stream_names)) {
        .Call("r_scs_register_streams", private$ptr,
              as.character(stream_names),
              PACKAGE = "socialchoicelab")
      }
      invisible(self)
    },

    #' @description Register allowed stream names. Only registered names may
    #'   be used with PRNG draw functions.
    #' @param stream_names Character vector of stream names.
    #' @return \code{self} invisibly.
    register = function(stream_names) {
      .Call("r_scs_register_streams", private$ptr, as.character(stream_names),
      PACKAGE = "socialchoicelab")
      invisible(self)
    },

    #' @description Reset all streams with a new master seed.
    #' @param master_seed New master seed.
    #' @return \code{self} invisibly.
    reset_all = function(master_seed) {
      .Call("r_scs_reset_all", private$ptr, as.double(master_seed),
      PACKAGE = "socialchoicelab")
      invisible(self)
    },

    #' @description Reset a single named stream to a new seed.
    #' @param stream_name Character. The stream to reset.
    #' @param seed New seed for this stream.
    #' @return \code{self} invisibly.
    reset_stream = function(stream_name, seed) {
      .Call("r_scs_reset_stream", private$ptr, as.character(stream_name),
            as.double(seed),
      PACKAGE = "socialchoicelab")
      invisible(self)
    },

    #' @description Skip (discard) \code{n} values in a named stream.
    #' @param stream_name Character.
    #' @param n Number of values to skip.
    #' @return \code{self} invisibly.
    skip = function(stream_name, n) {
      .Call("r_scs_skip", private$ptr, as.character(stream_name),
            as.double(n),
      PACKAGE = "socialchoicelab")
      invisible(self)
    },

    #' @description Draw a uniform real in \code{[min, max)}.
    #' @param stream_name Character.
    #' @param min,max Range boundaries.
    #' @return Scalar double.
    uniform_real = function(stream_name, min = 0.0, max = 1.0) {
      .Call("r_scs_uniform_real", private$ptr, as.character(stream_name),
            as.double(min), as.double(max),
      PACKAGE = "socialchoicelab")
    },

    #' @description Draw a normal variate N(mean, sd^2).
    #' @param stream_name Character.
    #' @param mean,sd Distribution parameters.
    #' @return Scalar double.
    normal = function(stream_name, mean = 0.0, sd = 1.0) {
      .Call("r_scs_normal", private$ptr, as.character(stream_name),
            as.double(mean), as.double(sd),
      PACKAGE = "socialchoicelab")
    },

    #' @description Draw a Bernoulli variate.
    #' @param stream_name Character.
    #' @param prob Probability of \code{TRUE}.
    #' @return Logical scalar.
    bernoulli = function(stream_name, prob) {
      .Call("r_scs_bernoulli", private$ptr, as.character(stream_name),
            as.double(prob),
      PACKAGE = "socialchoicelab")
    },

    #' @description Draw a uniform integer in \code{[min, max]}.
    #' @param stream_name Character.
    #' @param min,max Integer range (inclusive).
    #' @return Scalar double (to accommodate values beyond R's 32-bit integer).
    uniform_int = function(stream_name, min, max) {
      .Call("r_scs_uniform_int", private$ptr, as.character(stream_name),
            as.double(min), as.double(max),
      PACKAGE = "socialchoicelab")
    },

    #' @description Draw a uniform choice index in \code{[0, n)} (0-based).
    #' @param stream_name Character.
    #' @param n Number of choices.
    #' @return Integer scalar (0-based index).
    uniform_choice = function(stream_name, n) {
      .Call("r_scs_uniform_choice", private$ptr, as.character(stream_name),
            as.double(n),
      PACKAGE = "socialchoicelab")
    }
  )
)
