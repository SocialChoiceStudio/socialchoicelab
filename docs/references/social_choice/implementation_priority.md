# Implementation Priority Guide

Based on your comprehensive social choice reference library, here's a prioritized implementation roadmap for SocialChoiceLab.

## Phase 1: Foundation + Core Geometry (Milestone 1)

### Essential Geometric Concepts
**Priority: HIGH** - These are fundamental to spatial voting models

1. **Yolk Computation** (Feld, Grofman, Miller)
   - Minimal covering circle for majority rule
   - Core of the uncovered set
   - Essential for 2D spatial analysis

2. **Uncovered Set (Heart)**
   - Set of alternatives not covered by any other
   - Fundamental outcome concept
   - Requires exact geometric computations

3. **Convex Hull Operations**
   - Voter ideal points
   - Candidate positions
   - Policy space boundaries

### Distance Functions
**Priority: HIGH** - Core to preference modeling

1. **Euclidean Distance** (most common in spatial models)
2. **Manhattan Distance** (L1 norm)
3. **Minkowski Distance** (generalized Lp norm)

## Phase 2: Voting Rules & Aggregation

### Core Voting Rules
**Priority: HIGH** - Essential for any social choice platform

1. **Plurality** (simplest, most common)
2. **Borda Count** (scoring rule)
3. **Condorcet Methods** (pairwise comparisons)
4. **Approval Voting** (binary choices)

### Aggregation Properties
**Priority: MEDIUM** - Important for analysis

1. **Transitivity** (Condorcet consistency)
2. **Pareto Efficiency**
3. **Independence of Irrelevant Alternatives (IIA)**

## Phase 3: Advanced Spatial Analysis

### Multi-dimensional Computations
**Priority: MEDIUM** - Extends beyond 2D

1. **3D Spatial Models** (Schofield's work)
2. **N-dimensional Preference Spaces**
3. **High-dimensional Convex Hulls**

### Electoral Competition
**Priority: MEDIUM** - Simulation engines

1. **Centripetal vs Centrifugal Forces** (Cox, Calvo-Hellwig)
2. **Party Convergence Dynamics** (Gallego-Schofield)
3. **Strategic Voting Models** (Patty-Snyder-Ting)

## Phase 4: Advanced Applications

### Multi-Winner Voting
**Priority: LOW** - Specialized applications

1. **Approval-based Multi-Winner** (Lackner-Skowron)
2. **Proportional Representation**

### Frontier Research
**Priority: LOW** - Experimental features

1. **Fuzzy Preferences** (Chapter 20)
2. **Agenda Setting Models**
3. **Voting by Evaluation**

## Implementation Strategy

### Start with Exact 2D Geometry
- Use CGAL EPEC for robust computations
- Focus on Yolk and Uncovered Set algorithms
- Build solid foundation for spatial analysis

### Modular Design
- Each voting rule as separate service
- Pluggable distance functions
- Configurable aggregation properties

### Performance Considerations
- Exact geometry where correctness matters
- Numeric approximations for large-scale simulations
- Caching for expensive geometric computations

## Key Papers for Implementation

### Must-Read for Phase 1:
1. **Feld, Grofman, Miller - Yolk paper** (exact algorithms)
2. **Handbook Chapter 27 - Geometry of Voting** (comprehensive reference)
3. **Patty - Majority Rule in 2D** (implementation details)

### Reference for Phase 2:
1. **Handbook Chapter 4 - Voting Procedures** (complete survey)
2. **Miller, Shepsle - Spatial Model** (theoretical foundation)

### Advanced Phase 3+:
1. **Schofield - Spatial Modeling** (multi-dimensional)
2. **Cox - Centripetal/Centrifugal** (electoral competition)
3. **Frontier papers** (cutting-edge research)
