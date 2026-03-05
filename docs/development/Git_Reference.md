# Git Reference — SocialChoiceLab

Solo developer workflow. No GitButler. No branches until the project is mature. Just commits and pushes that work.

---

## One-Time Setup (Do This First)

If this directory is not yet a git repo, run the following once from the project root. If it is already initialized, skip to [Daily Workflow (Manual Alternative)](#daily-workflow-manual-alternative).

```bash
git init
git remote add origin https://github.com/SocialChoiceStudio/socialchoicelab.git
git add -A
git commit -m "Initial commit: project recovery 2026-03-03"
git branch -M main
git push -u origin main
```

After this, `git push` (no flags) works forever because `-u` set the tracking branch.

---

## End-of-Session Script (Recommended)

Instead of the three manual commands, run the end-of-session script:

```bash
./scripts/end-of-session.sh
```

It does four things automatically:
1. Updates `docs/status/where_we_are.md` pointer from `consensus_plan.md`
2. Appends a dated session entry to the Recent Work section
3. Stages and commits everything (`git add -A && git commit`)
4. Pushes to GitHub

You can pass an optional commit message:
```bash
./scripts/end-of-session.sh "Implement Item 28: CI workflow"
```

Or skip the push (commit locally only):
```bash
./scripts/end-of-session.sh --no-push
```

---

## End-of-Milestone Script

When you finish a phase, run:

```bash
./scripts/end-of-milestone.sh phase-3
```

Replace `phase-3` with any tag name (e.g. `v0.1.0`). It will:
1. Run `clang-tidy` on all source files (advisory — you can continue past warnings)
2. Remind you to update `CHANGELOG.md`
3. Call `end-of-session.sh` to commit and push
4. Create an annotated git tag and push it to GitHub

---

## Daily Workflow (Manual Alternative)

If you don't want to use the script, that's it — three commands:

```bash
git add -A
git commit -m "short description of what you did"
git push
```

- `git add -A` stages everything (new files, edits, deletes). The `.gitignore` keeps build artifacts and PDFs out.
- Commit as often or rarely as you want. One commit per item in the Consensus Plan is a good natural rhythm.
- `git push` sends it to GitHub.

---

## Writing Commit Messages

One line is fine. Describe *what changed*, not *how*:

```
Fix loss sign convention in distance_to_utility
Add FNV-1a hash for deterministic stream seeds
Reorganize docs into status/architecture/development
```

If it's a Phase/Item completion, include it:

```
[Item 28] Add CI workflow for build and lint
```

---

## Tagging Milestones

When you finish a phase, tag it so you have a clean recovery point:

```bash
git tag -a phase-0 -m "Phase 0 complete: immediate stabilization"
git push origin phase-0
```

For the current state (Phases 0–2 done), run these once after initial setup:

```bash
git tag -a phase-0 -m "Phase 0 complete"
git tag -a phase-1 -m "Phase 1 complete: reproducibility and test quality"
git tag -a phase-2 -m "Phase 2 complete: documentation truthfulness"
git push origin --tags
```

To see your tags: `git tag`  
To go back to a tag (read-only look): `git checkout phase-1`  
To come back: `git checkout main`

---

## Checking What You Have

```bash
git status          # what's changed since last commit
git log --oneline   # history as a clean list
git diff            # see exact line-level changes before committing
```

---

## The One Situation That Can Go Wrong

**Push is rejected** — this happens if you edited a file on GitHub's web UI or on another machine without pulling first. Git will say something like `rejected ... fetch first`.

Fix:
```bash
git pull --rebase
git push
```

`--rebase` puts your local commits on top of whatever was on GitHub. It's safe for a solo developer because there's nobody else's work to conflict with.

If `git pull --rebase` reports a conflict (rare if you're the only one working):
```bash
# Edit the conflicted file(s), remove the <<<< ==== >>>> markers, save
git add <conflicted-file>
git rebase --continue
git push
```

---

## What NOT to Do

| Command | Why not |
|---------|---------|
| `git push --force` | Destroys history on GitHub. Never. |
| `git rebase -i` (interactive rebase on pushed commits) | Rewrites shared history. Fine locally before pushing; don't do it after. |
| GitButler, Git Tower, or other "smart" Git clients | They add complexity and hidden state. Plain Git is fine for this project. |
| Branching | Not worth it until you have collaborators or a stable API to protect. Phase 3 complete; branching deferred until the project has collaborators or a stable API to protect. Revisit at c_api milestone. |

---

## Undoing Things

**Undo the last commit but keep the changes:**
```bash
git reset HEAD~1
```
Your files are unchanged; the commit is gone. Re-commit when ready.

**Throw away all uncommitted changes (dangerous — irreversible):**
```bash
git restore .
```
(On older Git versions, use `git checkout -- .` instead.)

**See what a specific commit changed:**
```bash
git show <commit-hash>
```
Get the hash from `git log --oneline`.

---

## GitHub Setup Checklist (One Time)

- [ ] Create repo on GitHub (name: `socialchoicelab`, visibility: private or public your call)
- [ ] Do the one-time setup commands above
- [ ] Verify push worked: `git log --oneline origin/main`
- [ ] Add the three phase tags above
- [ ] Consider enabling "Require pull request reviews" = OFF and "Allow force pushes" = OFF in GitHub repo settings (branch protection for `main`)
