# Reference Materials for SocialChoiceLab

This directory contains reference materials organized by layer and topic to support development of the SocialChoiceLab project.

## Directory Structure

### `/foundation/`
Core C++ foundation layer references:
- C++ best practices and standards
- CGAL documentation and examples
- Eigen linear algebra library guides
- Random number generation (PRNG) references
- Serialization (Protobuf) documentation

### `/geometry/`
Geometry Services layer references:
- CGAL EPEC (Exact Predicates, Exact Constructions) documentation
- 2D/3D geometric algorithms
- Convex hull algorithms
- Half-space intersection methods
- kd-tree implementations
- Exact vs numeric geometry trade-offs

### `/social_choice/`
Social choice theory and voting methods:
- Spatial voting models
- Voting rules and aggregation properties
- Outcome concepts (Copeland, Yolk, Heart, etc.)
- Preference modeling and utility functions
- Research papers and theoretical foundations

### `/development/`
Development guides and best practices:
- R package development (cpp11)
- Python package development (pybind11)
- C API design patterns
- Build system configuration (CMake, Makevars)
- Testing strategies
- Documentation standards

## Usage

When working on specific layers, refer to the relevant subdirectory for:
- Implementation guidance
- Algorithm references
- Best practices
- Example code
- Theoretical foundations

## Adding New References

1. Place files in the appropriate subdirectory
2. Update this README if adding new categories
3. Use descriptive filenames with dates/versions
4. Include brief descriptions in commit messages

