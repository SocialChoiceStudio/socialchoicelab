# Social Choice Theory Reference Index

This directory contains reference materials for SocialChoiceLab development. **PDF files are stored locally but not tracked in Git** to keep the repository lightweight.

## 📚 Reference Library Organization

### Core Theoretical Foundations
- **Handbook of Computational Social Choice** - Complete reference for computational methods
- **Chapter 1: Impossibility Theorems** - Arrow's theorem and fundamental limitations
- **Chapter 2: Categories of Arrovian Voting** - Classification of voting systems
- **Chapter 3: Domain Restrictions** - When voting systems work well
- **Chapter 4: Voting Procedures** - Complete survey of voting methods
- **Chapter 18: Topological Theories** - Advanced mathematical approaches
- **Chapter 26: Spatial Models of Voting** - Geometric preference modeling
- **Chapter 27: Geometry of Voting** - Essential for SocialChoiceLab implementation

### Spatial Voting & Geometry
- **Miller & Shepsle: Spatial Model of Politics** - Foundational spatial theory
- **Schofield: Spatial Modeling of Selections** - Multi-dimensional models
- **Patty: Majority Rule in 2D** - Implementation details for 2D spatial voting
- **Feld, Grofman, Miller: Yolk** - Core geometric concept for uncovered sets

### Electoral Competition
- **Cox: Centripetal vs Centrifugal Forces** - Party positioning dynamics
- **Calvo & Hellwig: Centripetal/Centrifugal** - Multi-candidate competition
- **Gallego & Schofield: Party Convergence** - When parties move toward center
- **Patty, Snyder, Ting: Strategic Voting** - Voter behavior models
- **Merrill & Adams: Multi-Candidate Competition** - Centrifugal forces

### Applications & Extensions
- **Lackner & Skowron: Multi-Winner Voting** - Approval-based methods
- **Chapter 20: Fuzzy Preferences** - Non-crisp preference modeling
- **Frontier Research Papers** - Cutting-edge developments

### Simulation & Computational Methods
- **Kamwa: Simulations in Preference Aggregation** - Computational approaches
- **Data Simulations for Voting by Evaluation** - New evaluation methods

## 🎯 Implementation Priority

See [Implementation_Priority.md](Implementation_Priority.md) in this directory for the full prioritized roadmap. Summary: Foundation + core geometry (Yolk, uncovered set, distance functions) → voting rules and aggregation → advanced spatial analysis → applications.

## 📁 File Organization

This directory contains reference materials for social choice theory and implementation. PDFs and papers are stored locally; see the sections above for topic organization.

## 💡 Usage Notes

- **PDFs are available locally** for reference during development
- **Key papers for Phase 1**: Yolk paper, Geometry of Voting, Spatial Models
- **Implementation guides**: Patty's 2D work, Schofield's multi-dimensional models
- **Theoretical foundation**: Handbook chapters, Miller & Shepsle spatial theory

## 🔗 Integration with SocialChoiceLab

These references directly inform:
- **Foundation layer**: Geometric types and exact computations
- **Geometry Services**: CGAL EPEC implementations
- **Preference Services**: Distance and utility functions
- **Voting Rules**: Implementation of classic and modern methods
- **Outcome Concepts**: Yolk, uncovered sets, winsets
- **Simulation Engines**: Electoral competition dynamics
