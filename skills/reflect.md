---
description: Learn from user corrections in MemoChat by comparing staged AI work with unstaged user edits and distilling one reusable rule.
---

# MemoChat Reflect

Use after an agent's changes were staged and the user made additional unstaged corrections.

## Inputs

`$ARGUMENTS` may be an `.ai/<project>` name. If provided, read the latest task context before analyzing the diff.

## Step 1: Gather Evidence

Run:

```powershell
git status --short
git diff --cached
git diff
```

If staged or unstaged diff is empty, stop and explain that reflection requires both.

If a project name was provided:

1. read `.ai/<project>/about.md`
2. find the latest letter folder
3. read `.ai/<project>/<letter>/context.md`
4. read `.ai/<project>/<letter>/plan.md` if present

## Step 2: Classify The Correction

Ask:

- Did the user fix a task-specific mistake, or a reusable project convention?
- Is the issue already covered by a local rule, README, script, or pattern?
- Would documenting this prevent future mistakes in unrelated MemoChat work?
- Does it involve Docker-only infrastructure, stable ports, D: storage, CMake presets, runtime scripts, service boundaries, migrations, or QML/server contract patterns?

Pick at most one strongest insight.

## Step 3: Decide Where It Belongs

- Put repo-wide rules in a local agent instruction file when the user wants that.
- Put task-specific lessons in `.ai/<project>/<letter>/review*.md` or a result log.
- Put mechanical review checks in a review checklist only if one exists or the user asks for one.
- Do not create extra documentation files unless requested.

## Step 4: Propose Before Editing

Do not silently edit guideline files. Show:

- the proposed text
- the target file and location
- why it is general enough
- why it does not duplicate existing guidance

Wait for user approval before applying.

## Rules

- Keep the rule short and positive.
- Do not document one-off bug fixes.
- Do not paste large diffs into docs.
- Do not contradict Docker-only infrastructure or stable port requirements.
