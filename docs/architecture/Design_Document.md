# SocialChoiceStudio – Living Design Document

## Overview
SocialChoiceStudio is a modular, high-performance simulation and analysis platform for **spatial** and **general social choice models**.  
It is designed for reproducible research, educational use, and exploratory simulations.  
The system is built around a **C++ core** with bindings for **R** and **Python**, plus an optional GUI.

## Project Goals
- Model separable and non-separable voter preferences.
- Perform exact geometry for spatial computations using CGAL.
- Provide modular plug-and-play architecture for preference modeling, aggregation, and outcome logic.
- Support reproducible workflows in R and Python, and interactive work in the GUI.

---

## What to Build

### A) Code-first packages
- **Core C++**: pure C++ (CGAL/Eigen), no binding headers.
- **Stable façade**: thin c_api (C ABI) with POD structs/opaque handles; no STL across boundary.
- **R package (socialchoicelab)**: cpp11 adapter that calls c_api.
- **Python package (socialchoicelab)**: pybind11 adapter that calls c_api.
- **Plot helpers**: Plotly (R and Py) with identical APIs, Kaleido for static export.
- **Repro**: ModelConfig JSON/YAML everywhere; export_script(config, lang="R|python").

### B) "GUI-lite"
- **R Shiny modules** and **Shiny for Python apps** call their language package (which calls c_api).

### C) Web app
- **Shiny for Python deployment**; talks to Python package (pybind11 → c_api).

---

## Layered Architecture

### 1. Foundation
- **core::types** – Eigen vectors (Vector2d, Vector3d, VectorXd), CGAL exact types when needed
- **core::kernels** – CGAL kernel policy, numeric kernels
- **core::linalg** – Eigen linear algebra (matrices, vectors, operations)
- **core::rng** – Seeded PRNG, multiple streams
- **core::serialization** – Protobuf for data exchange

### 2. Preference Services
- **distance** – Minkowski (p≥1), Euclidean, Manhattan, Chebyshev, custom
- **loss** – Linear and quadratic loss functions
- **utility** – Applies a loss function to a distance metric to produce utility values
- **indifference** – Level-set construction; stateless service

### 3. Geometry Services
- **geom2d** – CGAL EPEC exact 2D operations
- **geom3d** – CGAL exact 3D where needed
- **geomND** – Numeric N-dimensional algorithms: convex hull, half-space intersection, kd-tree

### 4. Profiles and Aggregation
- **profiles** – Generators, loaders, schema validation
- **directional_voting** – Support for directional preferences (voters can have preferred directions of change)
- **aggregation** – Convert utilities to ranks, scores, approvals

### 5. Voting Rules & Aggregation Properties
- **voting_rules** – Plurality, Borda, Condorcet, etc...
- **aggregation_properties** – Transitivity, Pareto efficiency, IIA, monotonicity, etc...

### 6. Outcome Concepts
- **outcome_concepts** – Copeland, Strong Point, Heart, Yolk, Uncovered sets, k-majority winsets

### 7. Simulation Engines (stateful)
- **adaptive_candidates** – Round-based candidate movement (Sticker, Aggregator, Hunter, Predator)
- **experiment_runner** – Replications, parameter sweeps

### 8. FFI & Interfaces
- **c_api** – Stable surface for all bindings
- **python** – pybind11
- **r** – cpp11
- **web** – gRPC service
- **ios** – C bridge (low priority)

---

## Integration Pattern

### Boundaries
- **Core engine**: header-only exposure limited to your C++ domain types; no R/Python includes.
- **c_api**: extern "C" functions with:
  - Opaque handles: SCS_Handle*, SCS_Profile*, etc.
  - POD config/result structs; lengths + pointers for arrays; error codes + message buffer.
  - No exceptions cross the boundary (catch → return code).

### R binding (cpp11)
- **Files**: r/src/cpp11_bindings.cpp, r/src/init.c (if needed for registration).
- **Marshalling**: use cpp11::sexp, cpp11::as<>, cpp11::writable::list/strings/doubles to translate R objects ↔ POD structs.
- **Errors**: map non-zero c_api codes to cpp11::stop(msg).
- **Build**: src/Makevars{,.win} link against libscs_core (prebuilt or built as part of the R package). Keep CGAL/Eigen out of R package sources; link the core.
- **CRAN-friendly**: cpp11 is header-only and widely used; avoid non-portable compiler flags in Makevars.

### Python binding (pybind11)
- **Files**: python/src/bindings.cpp.
- **Build**: scikit-build-core + CMake; produce wheels linking to libscs_core.
- **Errors**: map c_api errors to Python exceptions; NumPy views for bulk arrays where helpful.

---

## Design Principles
- **Performance first**: Use optimized libraries (Eigen, CGAL) for all geometric and numerical operations.
- **Speed by default**: Eigen vectors for fast numeric operations, CGAL only when exact precision is absolutely necessary.
- **Use "engines" only for stateful or iterative processes** (e.g., adaptive candidates, simulations).
- **Keep stateless math as "services"** (e.g., distance, loss, geometry computations).
- Separation of **distance** and **loss** functions for maximum flexibility in utility modeling.
- Modular boundaries so each part can be swapped or extended.
- Exact geometry (CGAL EPEC) only where correctness is critical and performance can be sacrificed.

---

## Determinism & Geometry
- **ModelConfig** carries master seed + named streams; geometry precision policy (numeric | exact2d) selects between Eigen-only ops and CGAL EPEC backends.

---

## Visualization Contract
- **Plot helpers** (R and Py) consume results + optional ModelConfig.plot; produce Plotly figures; identical theme (theme_scs/style_scs).

---

## Licensing
- **GPL v3** for full compatibility with CGAL EPEC and cpp11 (R binding layer).

---

## Naming
- **Project**: SocialChoiceStudio
- **Core package name**: `socialchoicelab` (R and Python)
- **GUI name**: SocialChoiceStudio (alias SCS)

---

## ModelConfig Structure
- **Master seed**: Global random number generator seed
- **Named streams**: Separate PRNG streams for different components (voters, candidates, etc.)
- **Geometry precision**: `numeric` (Eigen-only) or `exact2d` (CGAL EPEC)
- **Plot settings**: Theme preferences, color schemes, export formats
- **Performance limits**: Memory caps, CPU timeouts, iteration limits
- **Output formats**: JSON/YAML export options, precision settings

---

## Performance & Scalability
- **Memory management**: Smart pointers for large profiles, memory pools for temporary objects
- **Parallelization**: Multi-threading for independent computations (profile generation, voting rule evaluation)
- **Streaming**: Process large profiles in chunks to avoid memory issues
- **Caching**: Cache expensive geometric computations (convex hulls, intersections)


---

## Testing & Quality Assurance
- **Unit tests**: Test each service independently (distance, loss, geometry)
- **Integration tests**: Test complete workflows end-to-end
- **Geometric validation**: Compare against reference implementations (CGAL examples, published results)
- **Performance benchmarks**: Regression testing for computation times
- **Continuous integration**: Automated builds on multiple platforms
- **Test data**: Synthetic profiles, real-world examples, edge cases

---

## Advanced Features (Future Research)

### Empirical Data Integration
- **empirical_profiles** – Functions for generating preference profiles based on real-world data
  - Import from survey data, voting records, or other empirical sources
  - Statistical validation and cleaning of empirical preference data
  - Conversion between different empirical data formats

### Preference Estimation
- **preference_estimation** – Machine learning and statistical methods for estimating voter preferences
  - **loss_function_estimation** – Estimate optimal loss functions from observed behavior
  - **utility_function_estimation** – Recover complete utility functions from partial preference data
  - **parameter_estimation** – Fit model parameters to empirical data
  - **validation_methods** – Cross-validation and goodness-of-fit measures

*Note: These features require significant research and validation before implementation. Priority: Low - implement after core functionality is complete.*
