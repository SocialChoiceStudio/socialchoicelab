# Hosted MVP app

A Shiny for Python app implementing the single nontechnical-user workflow
described in
[docs/architecture/nontechnical_mvp_design.md](../docs/architecture/nontechnical_mvp_design.md).
See [docs/architecture/hosted_mvp_design.md](../docs/architecture/hosted_mvp_design.md)
for what has and has not been verified, including deployment status.

## Run locally (development)

Requires the `scs_api` C library already built (see
`docs/development/setup_checklist.md`) and the `socialchoicelab` Python
package installed from `../python/`.

```bash
pip install -r requirements.txt
export SCS_LIB_PATH=../build
export DYLD_LIBRARY_PATH=../build   # Linux: LD_LIBRARY_PATH
shiny run --reload app.py
```

Open the URL `shiny run` prints (typically `http://127.0.0.1:8000`).

## Run in a container

From the repository root:

```bash
docker build -f app/Dockerfile -t socialchoicelab-mvp .
docker run --rm -p 8000:8000 socialchoicelab-mvp
```

The `Dockerfile` has not been build-verified in an environment with Docker
available; verify it builds successfully before relying on it.

## Tests

```bash
pytest tests/
```

These are smoke tests for request-building and scenario-filtering logic.
They do not start a server or browser; see `hosted_mvp_design.md` for the
manual browser verification already performed.
