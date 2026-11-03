#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

# Run the "Full Build" CI workflow (.github/workflows/ci_full.yml) on
# the current branch of the current repo, via the gh CLI.
#
# usage: run_full_build.sh [-w|--watch] [-R|--repo OWNER/REPO] [--pr N]
#                          [-h|--help]
#
#   -w, --watch        Wait for the run to finish and exit with its status.
#   --pr N             Run on pull request N instead of the current branch:
#                      fetch the PR head, push it to origin as branch
#                      "pr-N" (overwriting any previous one), and run
#                      there. The PR is looked up in the "upstream" remote,
#                      else gh's default repo. Review the PR first: its
#                      code, including any workflow changes, runs in your
#                      repo with your secrets.
#   -R, --repo REPO    Repo to run in. Default: the repo of the remote the
#                      branch tracks (else origin), not gh's default repo,
#                      which is the upstream when a fork has an upstream
#                      remote.
#
# The workflow must exist on the remote branch, so the branch has to
# be pushed first, and gh must be authenticated (gh auth login).

set -euo pipefail

WORKFLOW="Full Build"
WATCH=0
REPO=""
PR=""

while [ $# -gt 0 ]; do
    case "$1" in
        -w|--watch) WATCH=1 ;;
        -R|--repo)  [ $# -ge 2 ] || { echo "$1 needs an argument" >&2; exit 2; }
                    REPO="$2"; shift ;;
        --pr)       [ $# -ge 2 ] || { echo "$1 needs an argument" >&2; exit 2; }
                    PR="$2"; shift
                    case "$PR" in ''|*[!0-9]*) echo "--pr needs a number" >&2; exit 2 ;; esac ;;
        -h|--help)  sed -n '5,26p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
    shift
done

command -v gh >/dev/null 2>&1 || { echo "error: gh is not installed" >&2; exit 1; }

if [ -n "$PR" ]; then
    # Copy the PR head into a branch of origin, where we can dispatch.
    if git remote get-url upstream >/dev/null 2>&1; then
        PR_SRC=$(git remote get-url upstream)
    else
        PR_SRC="https://github.com/$(gh repo view --json nameWithOwner --jq .nameWithOwner).git"
    fi
    BRANCH="pr-$PR"
    REMOTE=origin
    echo "Fetching PR #$PR from $PR_SRC..."
    git fetch -q "$PR_SRC" "pull/$PR/head"
    PR_SHA=$(git rev-parse FETCH_HEAD)
    echo "Pushing ${PR_SHA:0:8} to $REMOTE as $BRANCH..."
    git push -q --force "$REMOTE" "$PR_SHA:refs/heads/$BRANCH"
else
    BRANCH=$(git rev-parse --abbrev-ref HEAD)
    if [ "$BRANCH" = "HEAD" ]; then
        echo "error: detached HEAD; check out a branch" >&2
        exit 1
    fi
    REMOTE=$(git config "branch.$BRANCH.remote" || echo origin)
    [ "$REMOTE" = "." ] && REMOTE=origin
fi

if [ -z "$REPO" ]; then
    URL=$(git remote get-url "$REMOTE")
    REPO=$(echo "$URL" | sed -E 's#^(https?://[^/]+/|ssh://[^/]+/|[^@]+@[^:]+:)##; s#\.git$##')
fi

if [ -z "$PR" ]; then
    # Warn if the remote branch doesn't match what is checked out locally.
    git fetch -q "$REMOTE" "$BRANCH" 2>/dev/null || true
    if ! REMOTE_SHA=$(git rev-parse -q --verify "$REMOTE/$BRANCH"); then
        echo "error: branch '$BRANCH' has not been pushed to $REMOTE" >&2
        exit 1
    fi
    if [ "$REMOTE_SHA" != "$(git rev-parse HEAD)" ]; then
        echo "warning: local '$BRANCH' differs from $REMOTE/$BRANCH;" \
             "the run will use the pushed commit ${REMOTE_SHA:0:8}" >&2
    fi
fi

echo "Dispatching '$WORKFLOW' on branch '$BRANCH' in $REPO..."
START=$(date -u +%Y-%m-%dT%H:%M:%SZ)
gh workflow run "$WORKFLOW" --repo "$REPO" --ref "$BRANCH"

# gh workflow run doesn't print the run id; poll for the new run.
RUN_ID=""
for _ in $(seq 1 30); do
    RUN_ID=$(gh run list --repo "$REPO" --workflow "$WORKFLOW" --branch "$BRANCH" \
                 --event workflow_dispatch --limit 5 \
                 --json databaseId,createdAt \
                 --jq "[.[] | select(.createdAt >= \"$START\")] | first | .databaseId // empty")
    [ -n "$RUN_ID" ] && break
    sleep 2
done

if [ -z "$RUN_ID" ]; then
    echo "error: dispatched, but could not find the new run" >&2
    exit 1
fi

gh run view --repo "$REPO" "$RUN_ID" --json url --jq '.url'
echo "Follow with: gh run watch --repo $REPO $RUN_ID"

if [ "$WATCH" -eq 1 ]; then
    gh run watch --repo "$REPO" "$RUN_ID" --exit-status
fi
