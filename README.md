# socialchoicelab

Part of the **SocialChoiceStudio** project.  
Core C++ engine with R and Python bindings for simulating and analyzing spatial and general social choice models.

## Features

**Implemented now:** Distance functions (Eigen-based; Minkowski, Euclidean, Manhattan, Chebyshev), loss and utility functions (linear, quadratic, Gaussian, threshold), PRNG and StreamManager for reproducible randomness, and unit tests (GTest).

**Planned:** Exact geometry via CGAL (2D/3D), voting rules and aggregation properties, outcome concepts (Copeland, Yolk, Heart, etc.), R and Python interfaces, optional GUI (SocialChoiceStudio).

## Documentation

See the [Living Design Document](docs/architecture/Design_Document.md) for full architecture details, or [docs/README.md](docs/README.md) for the complete documentation index.

## License

socialchoicelab is licensed under the GNU General Public License v3 (GPL-3.0).  
See the [LICENSE](LICENSE) file for details.
