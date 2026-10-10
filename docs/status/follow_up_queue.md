# Follow-up queue

**Session-sized tasks.** Pick one. Priority and sequencing stay in [ROADMAP.md](ROADMAP.md).

**Rules**

- Do **not** treat this as a second roadmap. When an item ships, delete it here.
- When an item grows past one sitting, stop and put the rest in `ROADMAP.md`.

**Index:** [docs/README.md](../README.md).

---

## Next sitting

- [ ] Commit and push the uncommitted nontechnical-user-release work (see `.cursor/HANDOFF.md`; suggested split: service layer / overlay guard / scenario data fix / hosted app). Also push the 3 earlier unpushed commits.
- [ ] Confirm whether deleting `.cursor/rules/Review-Rules-At-Start.mdc` and `Run-All-Local-Checks-Before-Push.mdc` was intentional.
- [ ] Build-verify `app/Dockerfile`.
- [ ] Decide: hosting provider for the web app.
- [ ] Decide: when and how to run the user pilot.
- [ ] Decide: how to handle the 31 scenarios with no status quo point (let users click one, or add real ones from the literature). See `docs/architecture/hosted_mvp_design.md` § "Expanding scenario coverage".

## Not a short task

Leave these for a planned block: yolk algorithm replacement, the Heart boundary, Characteristics of Voting Rules, per-run position re-randomization, PrefLib import, correcting the paper draft's Fishburn sentence, and giving the finished-but-unreleased work its own release number.
