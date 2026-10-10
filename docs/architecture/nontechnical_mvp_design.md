# Nontechnical MVP: first user workflow

**Status:** Draft design, not yet built. Scopes step 1 of [nontechnical-user-release plan](../../.cursor/plans/nontechnical-user-release_61acb40c.plan.md).

**Purpose:** Define the single workflow the first hosted application must support, so the application service layer (step 3) and the Shiny app (step 5) have one unambiguous target instead of trying to expose the whole package surface at once.

---

## The one workflow

**"Explore a spatial voting scenario and export a figure."**

1. The user opens the app in a browser. No sign-in, no file upload required.
2. The user picks one of the bundled scenarios (see list below) from a dropdown, or enters a small number of voters and alternatives manually (2D coordinates, typed into a simple grid — no code).
3. The user chooses a decision rule: simple majority or a supermajority fraction (`k`). Weighted voting, veto players, and non-Euclidean distance are **not** in the MVP (see "Explicitly out of scope").
4. The app draws the spatial plot: voter ideal points, alternatives, the status quo, and the winset for the status quo, using the existing canvas stack (`plot_spatial_voting`, `layer_winset`, `layer_preferred_regions`).
5. The user can toggle the Pareto set (convex hull) and indifference curve overlays on or off.
6. The user clicks "Export" and downloads a self-contained HTML file of the current figure (same artifact `save_plot()` already produces).

That is the entire MVP. No competition simulation, no experiment sweeps, no saved projects, no accounts.

## Why this workflow first

- It uses only functionality that is already implemented and tested: `load_scenario`, `plot_spatial_voting`, `layer_winset`, `layer_preferred_regions`, `layer_ic`, `layer_convex_hull`, `save_plot`.
- It avoids every feature flagged as approximate, deferred, or unverified (see `release-safety` work below), so the first release makes no claim it cannot support.
- It gives a demonstrably complete, correct, exportable result in one sitting, which is the right size for a user pilot (plan step 6).

## Bundled scenarios shown in the dropdown

Pull directly from the existing bundled library at
[python/src/socialchoicelab/data/scenarios/](/Users/robiragan/Dropbox/SocialChoiceStudioDev/socialchoicelab/python/src/socialchoicelab/data/scenarios/)
(33 scenarios, e.g. `laing_olmsted_bear`, `condorcet_cycle_three`, `fiorina_plott_a_p_s_r1978_series1`). Use `list_scenarios()` to populate the dropdown rather than hardcoding names, so the MVP stays in sync as the scenario library grows.

## Explicitly out of scope for the MVP

| Feature | Why excluded now |
|---|---|
| Candidate competition simulation | Larger parameter surface; save for a second workflow once the first is validated with users. |
| Non-Euclidean distance configs | The overlay guard (release-safety work) is not yet built; showing these risks mislabeled overlays. |
| Yolk / Heart overlays | Documented as approximate (LP-yolk, grid-approximation Heart); excluded until relabeled or replaced. |
| CSV import of custom data | Typed 2D grid entry covers the "no custom data" pilot; file import adds validation and error-message surface that is better scoped after user feedback. |
| Saved projects / ModelConfig | Deferred on the roadmap; no serialization format exists yet. |
| Accounts / multi-user state | Each session is stateless; a session's inputs live only in the browser tab. |

## Interface sketch (for the application-layer and Shiny steps)

```
[ Scenario: [dropdown ▾] ]  or  [ Enter voters/alternatives manually ]
[ Decision rule: (•) Simple majority   ( ) Supermajority  [k: __] ]
[ Overlays: [ ] Pareto set   [ ] Indifference curves   [ ] Preferred regions ]

              <canvas plot>

[ Export HTML ]
```

## Acceptance criteria for this step

- A written, reviewed description of the one workflow exists (this document).
- Every function referenced above is already implemented, tested, and not flagged as approximate.
- The out-of-scope table gives the application-layer step (step 3) a concrete boundary: build only what this workflow needs first.
