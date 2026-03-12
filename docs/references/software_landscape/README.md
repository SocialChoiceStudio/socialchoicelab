# Social Choice Software and Website Landscape

This note is a working catalog of external software, web tools, data
libraries, and community resource hubs relevant to computational social choice.
It is intended as a comparison baseline for `socialchoicelab`, not as an
exhaustive literature review.

Initial seed sources:

- [COMSOC community tools directory](https://comsoc-community.org/tools)
- [COMSOC Zenodo community](https://zenodo.org/communities/comsoc/records?q=&l=list&p=1&s=10&sort=newest)
- [PrefSampling documentation](https://comsoc-community.github.io/prefsampling/)
- [VoteKit documentation](https://votekit.readthedocs.io/en/latest/)

## How to read this document

The entries below are grouped by what they are most useful for:

- software libraries for simulation, aggregation, and analysis;
- data libraries and interchange tooling;
- web apps and public-facing result explorers;
- community hubs that help track the broader ecosystem.

For each entry, the important question is not just “what does it do?”, but
also “how does it overlap with or differ from SocialChoiceLab?”

## 1. Community directories and archival hubs

### COMSOC community tools page

Link: [comsoc-community.org/tools](https://comsoc-community.org/tools)

Why it matters:

- This is the best single starting point for discovering software produced by
  the computational social choice community.
- It cuts across theory areas: voting rules, participatory budgeting,
  apportionment, sampling, datasets, and teaching/demo tools.
- It is useful as a “who else exists?” index even when the underlying tools are
  heterogeneous in maturity and scope.

How it helps SocialChoiceLab:

- good source for competitor/complement scans;
- good place to check naming conventions and feature coverage;
- good way to avoid reinventing existing niche tools.

### COMSOC Zenodo community

Link: [zenodo.org/communities/comsoc](https://zenodo.org/communities/comsoc/records?q=&l=list&p=1&s=10&sort=newest)

Why it matters:

- This is an archival layer rather than a polished software index.
- It collects software releases, datasets, replication packages, and related
  research artifacts from the community.
- It is especially useful for discovering code/data objects that may not have a
  strong standalone website.

How it helps SocialChoiceLab:

- useful for citation and software archaeology;
- useful for finding replication material, example datasets, and adjacent
  projects;
- good place to monitor how comparable projects package and publish releases.

## 2. Libraries and toolkits most relevant to SocialChoiceLab

### VoteKit

Link: [votekit.readthedocs.io](https://votekit.readthedocs.io/en/latest/)

Core role:

- General-purpose Python toolkit for computational social choice research.

What it appears strongest at:

- election objects and ballot/profile handling;
- tabulation workflows;
- exploratory research and teaching use in Python;
- a relatively broad “Swiss army knife” positioning.

Why it matters for comparison:

- This is one of the clearest modern Python comparators to `socialchoicelab`.
- It is more obviously election-workflow oriented than geometry oriented.
- It helps define the baseline for what users may expect from a Python social
  choice toolkit.

Relationship to SocialChoiceLab:

- overlap: profile/election handling, voting rules, research tooling;
- likely difference: SocialChoiceLab’s stronger emphasis on spatial geometry,
  exact/near-exact 2D social-choice objects, and cross-language parity with R.

### PrefSampling

Link: [comsoc-community.github.io/prefsampling](https://comsoc-community.github.io/prefsampling/)

Core role:

- Preference-profile and election-instance sampling library.

What it appears strongest at:

- generating synthetic preference data under a range of culture models;
- supporting simulation studies where profile generation is the main task;
- serving as infrastructure for downstream experiments rather than end-user
  visualization.

Why it matters for comparison:

- It is directly relevant to any future probabilistic or Monte Carlo
  social-choice layer in SocialChoiceLab.
- It is a reminder that profile generation itself is a substantial software
  niche, distinct from aggregation and geometry.

Relationship to SocialChoiceLab:

- overlap: stochastic generation of preference data;
- likely difference: PrefSampling is more focused on sampling breadth, while
  SocialChoiceLab currently centers more on geometry, aggregation, and
  competition.

### abcvoting

Link: [abcvoting.readthedocs.io](https://abcvoting.readthedocs.io/)

Core role:

- Python library for approval-based committee voting rules.

What it appears strongest at:

- approval-based multiwinner rules;
- computing winning committees and related committee properties;
- serving the specific approval-based committee voting literature.

Why it matters for comparison:

- It is a strong example of a focused, theory-specific library rather than a
  broad “everything in social choice” package.
- It shows how deep a specialized tool can go within one narrow rule family.

Relationship to SocialChoiceLab:

- overlap: approval-related aggregation concepts;
- likely difference: SocialChoiceLab is broader and more spatially oriented,
  while `abcvoting` goes much deeper into the ABC rule family.

### votelib

Links:

- [votelib.readthedocs.io](https://votelib.readthedocs.io/)
- [PyPI project page](https://pypi.org/project/votelib/)

Core role:

- Python package for evaluating election results under many real-world systems.

What it appears strongest at:

- broad rule coverage;
- election evaluation under practical institutional variants;
- reusable evaluator/converter architecture.

Why it matters for comparison:

- It represents the “many election systems, less geometry” end of the design
  space.
- It is useful as a comparison point for election-rule breadth and API design.

Relationship to SocialChoiceLab:

- overlap: voting-rule computation and election evaluation;
- likely difference: votelib is oriented toward evaluating electoral systems in
  practice, while SocialChoiceLab adds stronger spatial-model and competition
  structure.

### pref\_voting

Links:

- [pref-voting.readthedocs.io](https://pref-voting.readthedocs.io/)
- [PyPI project page](https://pypi.org/project/pref-voting/)

Core role:

- Python package for elections, profiles, margin graphs, and voting methods.

What it appears strongest at:

- reasoning about elections and margin-graph style objects;
- single-winner and related preference-voting workflows;
- a combination of package docs, examples, and web-app support.

Why it matters for comparison:

- It is close to the “theory + software” niche that SocialChoiceLab occupies.
- Margin-graph support makes it especially relevant for pairwise and tournament
  based analysis.

Relationship to SocialChoiceLab:

- overlap: profiles, pairwise logic, voting methods;
- likely difference: SocialChoiceLab currently emphasizes spatial geometry and
  deterministic multi-language bindings more strongly.

### SVVAMP

Links:

- [SVVAMP documentation](https://svvamp.readthedocs.io/)
- [PyPI project page](https://pypi.org/project/svvamp/)

Core role:

- Simulator of voting algorithms in manipulating populations.

What it appears strongest at:

- manipulation-related analysis;
- probabilistic population models;
- IIA and manipulation concepts in a simulation setting.

Why it matters for comparison:

- It is relevant to the “axiomatic/probabilistic simulation” direction that
  SocialChoiceLab has discussed but not fully implemented.
- It shows what a more manipulation-focused simulation package looks like.

Relationship to SocialChoiceLab:

- overlap: profile models, rule analysis, normative criteria;
- likely difference: SVVAMP appears more directly focused on manipulation and
  voting paradox diagnostics than on spatial social-choice geometry.

### Mapel / Mapof-Elections

Links:

- [mapel-core on PyPI](https://pypi.org/project/mapel-core/)
- [Mapof-Elections](https://science-for-democracy.github.io/mapof-elections/index.html)

Core role:

- Mapping and visualizing election instances via distances between elections.

What it appears strongest at:

- meta-analysis of election instances;
- “maps of elections” and instance-space exploration;
- simulation and experiment design around election similarity.

Why it matters for comparison:

- It occupies a different but adjacent niche: not mainly “compute the social
  choice object,” but “organize and visualize spaces of instances.”
- It may be useful inspiration if SocialChoiceLab later develops election-space
  comparison or scenario atlases.

Relationship to SocialChoiceLab:

- mostly complementary rather than directly overlapping.

## 3. Data libraries and interchange tooling

### PrefLib and PrefLib-Tools

Link: [PrefLib-Tools documentation](https://preflib.github.io/preflibtools/)

Core role:

- common repository and tooling ecosystem for structured preference data.

What it appears strongest at:

- data interchange and standard file formats;
- importing/exporting ordinal and related preference instances;
- basic analytic checks over stored preference data.

Why it matters for comparison:

- PrefLib is the closest thing to shared infrastructure for preference-data
  exchange in this ecosystem.
- Interoperability with PrefLib-style data is likely important if
  SocialChoiceLab wants to connect to existing datasets and benchmarks.

Relationship to SocialChoiceLab:

- more complementary than competitive;
- good candidate for import/export interoperability rather than replacement.

### prefio

Link: [prefio documentation](https://fleverest.github.io/prefio/index.html)

Core role:

- R-side preference-data structures and PrefLib interoperability.

What it appears strongest at:

- tidy handling of preference data in R;
- importing from and exporting to PrefLib formats;
- bridging tabular R workflows with preference datasets.

Why it matters for comparison:

- This is especially relevant if SocialChoiceLab’s R side later needs better
  dataset interchange rather than just algorithm bindings.

Relationship to SocialChoiceLab:

- complementary;
- suggests a possible R interoperability path instead of building all
  preference-data IO from scratch.

### Pabulib

Link: [pabulib.org/about](https://pabulib.org/about)

Core role:

- open participatory budgeting data library built around the `.pb` format.

What it appears strongest at:

- collecting PB datasets;
- standardizing PB instance representation;
- supporting empirical and computational PB research.

Why it matters for comparison:

- It is a major data resource for participatory budgeting, which is closely
  connected to multiwinner and apportionment-style social choice research.
- It is a good model of how a community data library can complement analysis
  code.

Relationship to SocialChoiceLab:

- complementary;
- relevant if SocialChoiceLab expands further into PB or multiwinner public
  budgeting workflows.

### pabutools

Link: [comsoc-community.github.io/pabutools](https://comsoc-community.github.io/pabutools/)

Core role:

- software toolkit for participatory budgeting instances and PB analytics.

What it appears strongest at:

- handling PB instances;
- computing PB outcomes and running analytic routines;
- close integration with Pabulib data.

Why it matters for comparison:

- Together, Pabulib + pabutools represent a strong PB-specific ecosystem.
- This is a good example of a successful domain-specific split between data
  library and analysis library.

Relationship to SocialChoiceLab:

- complementary;
- relevant if SocialChoiceLab wants PB support, but not a direct replacement
  for its spatial-geometry focus.

## 4. Web tools and public-facing result explorers

### pref.tools

Link: [pref.tools vote app](https://pref.tools/vote/about)

Core role:

- public-facing web applications for voting-rule exploration and computation.

Why it matters:

- It shows a different dissemination model: web-first interfaces built on top
  of existing social-choice code.
- The vote app explicitly notes that it draws on software such as
  `pref_voting`, `abcvoting`, and related computation tools.

Relationship to SocialChoiceLab:

- not a like-for-like library competitor;
- more a model for how research software can be surfaced to broader audiences.

### Method of Equal Shares online computation tool

Link: [equalshares.net computation tool](https://equalshares.net/implementation/computation/)

Core role:

- browser-based computation tool for a specific participatory budgeting rule.

Why it matters:

- It is a concrete example of theory-specific computation exposed directly to
  users in the browser.
- It demonstrates that narrowly scoped web tools can coexist with deeper
  back-end libraries.

Relationship to SocialChoiceLab:

- useful as a product/design reference if SocialChoiceLab develops theory-led
  explainer or calculator pages.

## 5. Practical comparison notes for SocialChoiceLab

Across the landscape above, a few broad patterns stand out:

### Most external tools are narrower than SocialChoiceLab

Many projects go deep in one area:

- approval-based committee voting (`abcvoting`);
- participatory budgeting (`pabutools`, Pabulib, Equal Shares);
- profile sampling (`PrefSampling`);
- election-system evaluation (`votelib`);
- manipulation or axiomatic diagnostics (`SVVAMP`);
- election-instance mapping (`Mapel` / Mapof-Elections).

`socialchoicelab` is unusual in trying to combine:

- spatial geometry;
- ordinal aggregation;
- dynamic electoral competition;
- cross-language reproducibility via a common core.

### SocialChoiceLab is strongest where geometry matters

The most distinctive comparative niche appears to be:

- winsets, veto-restricted winsets, uncovered sets, heart/core approximations,
  yolk, and related spatial objects;
- integration of those concepts with dynamic competition traces;
- parity across C++ core, R, and Python.

### SocialChoiceLab is still weaker in ecosystem/data integration

Compared with the surrounding ecosystem, obvious gaps include:

- preference-data interchange and standard formats;
- public external dataset integration;
- broader probabilistic/axiomatic simulation tooling;
- public-facing browser tools aimed at non-programmer users.

## 6. Obvious next additions to this catalog

This document is only a starting sweep. High-value next additions would be:

- a dedicated section on participatory budgeting software and datasets;
- a section on web demos and educational explainers;
- an interoperability section: PrefLib, `.pb`, CSV/JSON profile formats;
- a more systematic comparison table covering license, language, docs quality,
  data support, and theory coverage.

## 7. Working conclusion

The current ecosystem is not empty at all. There are already good tools for:

- sampling;
- approval-based committee rules;
- election tabulation;
- manipulation analysis;
- participatory budgeting;
- preference-data storage and interchange;
- web-based public exploration.

That is useful for SocialChoiceLab in two ways:

- it sharpens where SocialChoiceLab is genuinely distinctive;
- it shows where interoperability or selective borrowing would be more valuable
  than trying to build every adjacent capability in-house.
