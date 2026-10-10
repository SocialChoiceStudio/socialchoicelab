"""app.py — Hosted Shiny for Python MVP: "Explore a spatial voting scenario
and export a figure."

Implements step 5 of the nontechnical-user-release plan
(.cursor/plans/nontechnical-user-release_61acb40c.plan.md). The UI only
collects inputs and renders output; all computation goes through
socialchoicelab.run_spatial_voting_mvp(), the application service layer
(see docs/architecture/application_service_layer.md). This file should
stay thin: if a change needs new computation, add it to the service
layer, not here.

Scope is deliberately limited to the single workflow documented in
docs/architecture/nontechnical_mvp_design.md. See that document's
"Explicitly out of scope" table for what this app does not do.

Run locally for development:
    cd app && shiny run --reload app.py

This file is the local/dev entry point. It has not been deployed to a
public host; see docs/architecture/hosted_mvp_design.md for what
remains before a public deployment and user pilot.
"""

from __future__ import annotations

import io

from shiny import App, reactive, render, ui

import socialchoicelab as scl
from socialchoicelab import plots as sclp

def _scenarios_with_status_quo() -> list[str]:
    """Bundled scenario names that have a status quo point.

    run_spatial_voting_mvp() computes the winset of the status quo, so only
    scenarios that define one can run this workflow. Most bundled scenarios
    (illustrative cycling/core examples) omit a status quo entirely, so
    presenting them here would always raise an error. Restrict the dropdown
    to the scenarios that actually work instead of offering choices that
    cannot succeed.
    """
    usable = []
    for name in scl.list_scenarios():
        if scl.load_scenario(name)["status_quo"] is not None:
            usable.append(name)
    return usable


_SCENARIOS = _scenarios_with_status_quo()

_OVERLAY_CHOICES = {
    "pareto": "Pareto set (convex hull)",
    "indifference_curves": "Indifference curves",
    "preferred_regions": "Preferred regions",
}

app_ui = ui.page_sidebar(
    ui.sidebar(
        ui.h4("Spatial voting explorer"),
        ui.input_radio_buttons(
            "input_mode",
            "Voters",
            choices={"scenario": "Bundled scenario", "manual": "Enter manually"},
            selected="scenario",
        ),
        ui.panel_conditional(
            "input.input_mode === 'scenario'",
            ui.input_select("scenario_name", "Scenario", choices=_SCENARIOS),
        ),
        ui.panel_conditional(
            "input.input_mode === 'manual'",
            ui.input_text_area(
                "manual_voters_text",
                "Voter ideal points (one 'x,y' pair per line)",
                value="0,0\n2,0\n1,2",
                rows=6,
            ),
            ui.input_text("manual_sq_text", "Status quo (x,y)", value="1,0.5"),
        ),
        ui.hr(),
        ui.input_radio_buttons(
            "rule_kind",
            "Decision rule",
            choices={"majority": "Simple majority", "supermajority": "Supermajority"},
            selected="majority",
        ),
        ui.panel_conditional(
            "input.rule_kind === 'supermajority'",
            ui.input_slider(
                "supermajority_fraction", "Majority fraction", min=0.51, max=1.0,
                value=0.67, step=0.01,
            ),
        ),
        ui.hr(),
        ui.input_checkbox_group(
            "overlays", "Overlays", choices=_OVERLAY_CHOICES, selected=[]
        ),
        ui.hr(),
        ui.download_button("export_html", "Export HTML"),
        width=340,
    ),
    ui.output_ui("error_banner"),
    ui.output_ui("canvas"),
    ui.output_text("summary_text"),
)


def _build_request(input) -> dict:
    """Translate current UI input values into a run_spatial_voting_mvp() request."""
    request: dict = {
        "decision_rule": {"kind": input.rule_kind()},
        "overlays": {key: (key in input.overlays()) for key in _OVERLAY_CHOICES},
    }
    if input.rule_kind() == "supermajority":
        request["decision_rule"]["fraction"] = input.supermajority_fraction()

    if input.input_mode() == "scenario":
        request["scenario_name"] = input.scenario_name()
        return request

    voters = []
    for line_no, line in enumerate(input.manual_voters_text().splitlines(), start=1):
        line = line.strip()
        if not line:
            continue
        parts = line.split(",")
        if len(parts) != 2:
            raise ValueError(
                f"Voter line {line_no} ('{line}') must have exactly two numbers "
                "separated by a comma, like '0,0'. Fix that line and try again."
            )
        try:
            voters.append([float(parts[0]), float(parts[1])])
        except ValueError as exc:
            raise ValueError(
                f"Voter line {line_no} ('{line}') contains a value that is not "
                "a number. Enter two numbers separated by a comma, like '0,0'."
            ) from exc
    request["manual_voters"] = voters

    sq_parts = input.manual_sq_text().strip().split(",")
    if len(sq_parts) != 2:
        raise ValueError(
            f"Status quo ('{input.manual_sq_text()}') must be exactly two "
            "numbers separated by a comma, like '1,0.5'. Fix it and try again."
        )
    try:
        request["manual_status_quo"] = [float(sq_parts[0]), float(sq_parts[1])]
    except ValueError as exc:
        raise ValueError(
            f"Status quo ('{input.manual_sq_text()}') contains a value that is "
            "not a number. Enter two numbers separated by a comma, like '1,0.5'."
        ) from exc
    return request


def server(input, output, session):
    @reactive.calc
    def result_or_error():
        """Run the MVP workflow; return (result, None) or (None, error_message)."""
        try:
            request = _build_request(input)
            return scl.run_spatial_voting_mvp(request), None
        except ValueError as exc:
            return None, str(exc)

    @render.ui
    def error_banner():
        _, error = result_or_error()
        if error is None:
            return None
        return ui.div(
            ui.strong("Could not run this analysis: "), error,
            style=(
                "background:#fdecea;color:#611a15;border:1px solid #f5c6cb;"
                "border-radius:4px;padding:10px;margin-bottom:10px;"
            ),
        )

    @render.ui
    def canvas():
        res, error = result_or_error()
        if error is not None:
            return None
        html = sclp.render_spatial_voting_html(res["canvas"])
        return ui.tags.iframe(
            srcdoc=html,
            style="width:100%;height:620px;border:none;",
        )

    @render.text
    def summary_text():
        res, error = result_or_error()
        if error is not None:
            return ""
        s = res["summary"]
        return (
            f"Source: {s['source']} | Voters: {s['n_voters']} | "
            f"Decision rule: {s['decision_rule']['kind']} "
            f"(k={s['decision_rule']['k']}) | "
            f"Winset empty: {s['winset_is_empty']}"
        )

    # NOTE: shiny.render.download is deprecated in favor of render.download_button,
    # but render.download_button's auto_output_ui did not wire correctly against
    # an explicit ui.output_ui() placeholder in "core" (non-express) Shiny apps
    # as of shiny 1.8.0; render.download + ui.download_button is kept here as the
    # combination verified to work end to end (see docs/architecture/hosted_mvp_design.md).
    @render.download(filename="spatial_voting.html")
    def export_html():
        res, error = result_or_error()
        if error is not None:
            raise ValueError(
                "Cannot export: the current inputs produced an error. Fix the "
                "error shown above, then export again."
            )
        html = sclp.render_spatial_voting_html(res["canvas"])
        yield io.BytesIO(html.encode("utf-8")).read()


app = App(app_ui, server)
