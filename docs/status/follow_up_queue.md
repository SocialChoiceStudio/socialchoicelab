# Follow-up queue

**Session-sized tasks.** Pick one. Priority and sequencing stay in [ROADMAP.md](ROADMAP.md).

**Rules**

- Do **not** treat this as a second roadmap. When an item ships, delete it here.
- When an item grows past one sitting, stop and put the rest in `ROADMAP.md`.

**Index:** [docs/README.md](../README.md).

---

## Next sitting

- [ ] Build-verify `app/Dockerfile`.
- [ ] Decide: hosting provider for the web app.
- [ ] Decide: when and how to run the user pilot.
- [ ] Decide: how to handle the 31 scenarios with no status quo point (let users click one, or add real ones from the literature). See `docs/architecture/hosted_mvp_design.md` § "Expanding scenario coverage".
- [ ] `scripts/check-all.sh` keeps an `SCS_LIB_PATH` that is already set. Run from a worktree with `SCS_LIB_PATH` pointing at the main checkout, it tests R and Python against the main checkout's library, not the worktree's. Make it warn or fail when `SCS_LIB_PATH` is outside the repo being checked.
- [ ] Winset errors differ between bindings: Python raises `scs_winset_2d failed: <message>`, R shows `<message>` only. Make them identical (R/Python parity).
- [ ] The committed R pkgdown site (`r/docs/`) was built at version 0.2.0, so every rebuild changes about 95 pages to 0.3.0. Rebuild and commit it once, on its own.

## Not a short task

Leave these for a planned block: yolk algorithm replacement, the Heart boundary, Characteristics of Voting Rules, per-run position re-randomization, PrefLib import, correcting the paper draft's Fishburn sentence, and giving the finished-but-unreleased work its own release number.
