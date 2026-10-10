# Upstream contributions

## Standing authorization and completion

I require agents to upstream eligible changes discovered or made during assigned
work. Do the research, prepare and validate the patch, and submit or update the PR
autonomously. Do not ask whether I want it upstreamed or stop at a recommendation.
This authorization includes creating a necessary fork and contribution branch,
pushing the focused patch, required upstream design discussions, and directly
relevant evidence, review replies or proposed patches on other people's PRs.
Use upstream's contribution channel, including merge requests where appropriate.

Eligibility means the change belongs in that upstream's scope, is distributable,
and satisfies its submission requirements. Assess every candidate; do not label a
change ineligible merely because preparation is inconvenient. Mandatory upstreaming
means a justified disposition for every candidate, not a quota of new PRs. Local
preferences, personal agent instructions and rejected out-of-scope features stay
local unless that upstream requests them. Do not sweep unrelated projects.

This does not authorize releases, merging PRs, pushing to another contributor's
branch, or overriding tool permissions. Do not sign legal agreements on my behalf.
If credentials, required hardware, permissions or maintainer decisions block work,
record the precise blocker and next action, finish independent work, and report it.
The standing decision to upstream needs no further confirmation.

## 1. Establish ownership and current requirements

Trace the changed code through my fork to its actual upstream. Verify its current
repository, target branch and contribution channel; a remote name alone is not
proof. A fix in PS5_PayloadSDK or PS5_Mesa may still need a separate contribution to
the project they derive from. Conversely, a local PS5 addition may be outside that
external project's scope. Fix the narrowest layer that owns the behaviour.

Read the current README, CONTRIBUTING files, PR template, applicable instructions,
and relevant maintainer discussions. Record the sources that affect the decision.
Follow requirements for scope, style, generated files, tests, commit format,
licensing, attribution and design discussion. If discussion is required, first look
for an existing one, then participate there or open one focused discussion.
Routine changes covered by an established procedure do not need a speculative
proposal essay. Resolve apparently conflicting guidance using current, specific
maintainer instructions; if unclear, ask a focused question in the relevant thread.

## 2. Check existing work before preparing a contribution

Search the live upstream, across all authors:

- Current target-branch code and relevant commit history, including renamed files
  and symbols: the problem may already be fixed without a PR.
- Open PRs, including drafts, and closed PRs, including merged and unmerged ones.
- Issues and relevant design discussions, including linked or superseding work.

Search by behaviour, error text, affected symbols, file paths and alternative
terminology. Title-only matching is insufficient. Read plausible matches' diffs,
comments and reviews; compare the actual behaviour and scope. Follow pagination
or narrow queries until relevant results are inspected. A truncated result list,
failed request or unavailable search is not evidence that no prior work exists.
Record queries, time checked, relevant links and the reason for the disposition.

| What already exists | Required action |
| --- | --- |
| The fix is merged or present on the target branch | Adopt or backport it as appropriate; update the consumer pin and validate. No duplicate PR. |
| An equivalent PR of mine is open, including a draft | Continue that PR and branch; address its review. Do not open a replacement to avoid its history. |
| An equivalent PR by someone else is open | Reuse it where appropriate. Add only new, directly useful evidence, review or a proposed patch, with credit. Do not create a competing duplicate or push to their branch without permission. |
| Prior work was closed or rejected | Read why. Revisit only with substantive new evidence, changed requirements or a maintainer invitation; explain that difference and link the prior work. Do not evade a scope rejection by renaming the proposal. |
| Work is related but not equivalent | Explain the precise missing behaviour; extend the existing discussion where suitable. Open a distinct PR only when the remaining change has a coherent, non-overlapping scope. |
| No equivalent work was found | Prepare the smallest complete contribution; record the search evidence. |
| Search is incomplete or unavailable | Record the blocker and continue local preparation where useful. Do not publish a new PR until the check can be completed. |

An existing issue is context, not necessarily an existing implementation: follow
its discussion and link a justified PR. An inactive PR is not automatically
abandoned. Ask in the existing thread when continuation needs coordination.

## 3. Prepare and prove the smallest complete change

- Keep integration on my fork's `main`. Prepare upstream work in an isolated
  checkout/worktree and contribution branch from the current upstream target.
  Include only the needed changes, not my fork's accumulated divergence. Preserve
  unrelated working files. Never rewrite published history or force-push.
- One coherent problem per PR. Split independently reviewable fixes, but keep a
  fix and its required declarations, generated output and tests together. Avoid
  splitting one change into a flood of incomplete PRs. State real dependencies.
- Match upstream's design and use its generators. Prefer a consumer-specific build
  flag over a global SDK default when only that consumer needs different behaviour.
  Explain compatibility effects if the broader change is actually necessary.
- Run relevant upstream checks and an appropriate reproducer/regression check.
  Record base/head revisions, commands, environment, before/after results and
  untested limits. For hardware claims, record firmware and build identity with
  sanitized measured results. A successful build proves only that it builds.
- Satisfy explicit pre-submission gates before opening a PR, including a draft.
  A draft is appropriate only when upstream welcomes early collaboration and the
  remaining work is clearly stated. Ready-for-review means its requirements are
  met; draft status does not excuse unsupported claims or missing required proof.
- Preserve authorship and licences. Publish only permitted source and sanitized
  evidence: no credentials, console addresses, proprietary binaries, firmware,
  game data or raw private captures. Use the existing privacy and release rules.

## 4. Explain, recheck, publish and follow through

Write for a human maintainer who has not read this conversation. Use upstream's
template when present. Otherwise, a short description normally suffices:

- **Problem:** the concrete failing behaviour, who encounters it and a reproducer
  or before/after example where helpful.
- **Change and reason:** what the patch does, why it belongs here, compatibility
  effects and the reason for any non-obvious choice.
- **Evidence:** actual commands and results, hardware evidence where required,
  limitations and directly relevant issues or prior PRs. Explain any overlap.

Use a specific title and natural prose. Keep detail proportional to the change;
avoid an essay for a generated-stub addition. Do not paste agent deliberation,
marketing language or unsupported test claims. Verify every statement against the
diff and evidence. Honour upstream's disclosure requirements for AI assistance;
do not invent personal testing or misrepresent who authored the work.

Immediately before publishing, refresh the target branch and repeat the duplicate
check, including my fork's contribution branches and PRs. Reconcile a new match
instead of opening another PR. When several agents are active, record one owner for
the candidate and coordinate publication. Before retrying a create operation with
an unknown outcome, look up whether it already succeeded.

Submit when ready without asking me again. Read the published PR back: verify its
base/head, diff, title, body, state and checks; record its URL. Address available
review feedback within the active task, preserving the existing conversation and
updating the description when the patch's scope changes. Contribute to others'
PRs only when there is new, directly useful information; no duplicate comments,
repeated review requests, automated reminders or pressure for a response.
Do not schedule monitoring unless I request it. Submission does not imply acceptance.

## 5. Leave a record the next agent can use

Use the owning project's existing contribution ledger, or create
`UPSTREAM_CONTRIBUTIONS.md` at its root when the first candidate needs recording.
Keep one entry per problem, with:

- Upstream repository and target branch; concise problem and affected paths/symbols.
- Current owner/branch, search time, queries, relevant sources and prior-work links.
- Disposition: submitted/updated, existing work reused or assisted, already fixed,
  outside scope/rejected with rationale, or blocked with the exact missing step.
- Local and upstream commits, PR URL/state, validation evidence and remaining work.

Update the entry instead of duplicating it. Recheck live state before acting; the
ledger records history, not proof that nothing new exists. Final task reports name
the contributions and links, checks actually run, and unresolved blockers. Never
report a candidate as submitted when it is only prepared, or merged when only open.

## Utmost respect for John Törnblom

**I explicitly require the utmost respect for John Törnblom (`john-tornblom`).**
Apply this whenever he owns or maintains the upstream repository, including
organization-owned projects. Verify ownership or his maintainer role from the
repository and its contribution history; do not rely only on the account in the URL.
All maintainers deserve courtesy; this is an explicit priority for my work with him.

- Read his instructions and prior feedback before writing. Follow his project
  boundaries and requested workflow, and acknowledge corrections by addressing
  them in the patch or explanation.
- Respect his time: submit prepared, focused contributions with verified evidence;
  keep replies concise and useful. Never pressure him for review or acceptance,
  repeatedly ping him, or treat his time as owed to us.
- Credit his work and preserve attribution. Use a professional, considerate tone,
  without sarcasm, condescension, blame, defensive arguments or exaggerated flattery.
- If the evidence supports a technical disagreement, explain it calmly and
  precisely, acknowledge uncertainty and ask a focused question when needed.
  Respect does not mean hiding a bug or claiming agreement against the evidence.
- Honour his scope and acceptance decisions. Do not repackage rejected work or
  approach another thread to circumvent his answer. Revisit only under the
  substantive-new-evidence rule above, with the earlier discussion linked.

Before publishing a PR, issue, review or reply in such a project, reread it for
accuracy, tone and avoidable demands on his time. This is part of autonomous
preparation, not an additional approval step for me.

## SDK maintainer guidance to recheck

These are specific lessons from John Törnblom's `ps5-payload-dev/sdk`, not universal
requirements for unrelated projects. Re-read the live guidance before submitting:

- [README: adding SCE libraries](https://github.com/ps5-payload-dev/sdk#adding-new-sce-libs):
  use the documented stub generator; generated stubs and proprietary API headers
  have different scope. Never publish the input SPRX binaries.
- [PR #71](https://github.com/ps5-payload-dev/sdk/pull/71): the maintainer directed
  stub additions through that generator and excluded proprietary-library headers
  from this SDK. Do not resubmit rejected header additions as another SDK PR.
- [Issue #69](https://github.com/ps5-payload-dev/sdk/issues/69): follow the existing
  procedure and send the focused patch; avoid an unnecessary proposal essay.
- [PR #74](https://github.com/ps5-payload-dev/sdk/pull/74): confirm the problem on
  real hardware before submitting. A draft does not bypass this requirement.
- [PR #75](https://github.com/ps5-payload-dev/sdk/pull/75): consider placing the
  floating-point flag in Mesa's build rather than changing the SDK default for
  every consumer. Treat the discussion as design feedback, not blanket approval.
