# Foundation Layer References

Core C++ foundation components for SocialChoiceLab.

## Core Components

### Geometry / numeric types
- **Eigen vectors** (Vector2d, Vector3d, VectorXd) for points and numeric operations.
- Distance and preference code use `Eigen::MatrixBase`-derived types.

### core::kernels
- CGAL kernel policies
- Numeric kernels for performance
- Precision vs speed trade-offs

### core::linalg
- Eigen linear algebra operations
- Matrix operations and decompositions
- Vector operations

### core::rng
- Seeded PRNG implementation
- Multiple independent streams
- Reproducible random number generation

### core::serialization
- Protobuf for data exchange
- Configuration serialization
- Cross-language data sharing

## Key References Needed

- [ ] CGAL documentation and tutorials
- [ ] Eigen quick reference guide
- [ ] C++17/20 best practices
- [ ] Memory management patterns
- [ ] Random number generation theory
- [ ] Protobuf C++ integration guide

