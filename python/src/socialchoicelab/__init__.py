# socialchoicelab — spatial social choice analysis.
#
# Public API is populated across phases B4–B5.

from socialchoicelab._loader import _lib, _ffi  # noqa: F401  eagerly loads libscs_api

# B4.2 — Exception hierarchy
from socialchoicelab._error import (  # noqa: F401
    SCSError,
    SCSInvalidArgumentError,
    SCSInternalError,
    SCSBufferTooSmallError,
    SCSOutOfMemoryError,
)

# B4.1 — Configuration types
from socialchoicelab._types import (  # noqa: F401
    DistanceConfig,
    LossConfig,
    make_dist_config,
    make_loss_config,
)

# B4.3 — StreamManager
from socialchoicelab._stream_manager import StreamManager  # noqa: F401

# B4.4 — Winset
from socialchoicelab._winset import (  # noqa: F401
    Winset,
    winset_2d,
    weighted_winset_2d,
    winset_with_veto_2d,
)

# B4.5 — Profile
from socialchoicelab._profile import (  # noqa: F401
    Profile,
    profile_build_spatial,
    profile_from_utility_matrix,
    profile_impartial_culture,
    profile_uniform_spatial,
    profile_gaussian_spatial,
)

__version__ = "0.1.0"
