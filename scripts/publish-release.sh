#!/usr/bin/env bash
set -euo pipefail

# Interactive release helper for specialdlp (adapted from eMuleQt's).
#
# Reads the library version from CMakeLists.txt, lets you release it as-is or
# pick a new one. A new version is written to CMakeLists.txt and the vcxproj
# (SdlpLibVersion) and committed as "release: vX.Y.Z". Then master is pushed.
#
# No tag is pushed from here: .github/workflows/ci.yml builds all platforms,
# runs the diff against upstream and, when everything is green on master,
# creates tag vX.Y.Z at that commit and publishes the GitHub Release with the
# archives + SHA256SUMS.txt. A pushed tag would start a second, duplicate run.
#
# Nothing new to push (version unchanged, master already up to date):
#   Actions -> CI -> Run workflow on master releases the current commit.
#
# Usage: scripts/publish-release.sh

if [[ $# -gt 0 ]]; then
  echo "usage: $0" >&2
  exit 1
fi

# --- locate repo root -------------------------------------------------------
if ! ROOT="$(git rev-parse --show-toplevel 2>/dev/null)"; then
  echo "error: not inside a git repository" >&2
  exit 1
fi
cd "$ROOT"

BRANCH="master"
REMOTE="origin"
VCXPROJ="specialdlp/specialdlp/specialdlp.vcxproj"

# --- guards -----------------------------------------------------------------
if [[ -n "$(git status --porcelain)" ]]; then
  echo "error: working tree has uncommitted changes; commit or stash them first" >&2
  git status --short >&2
  exit 1
fi

if [[ "$(git rev-parse --abbrev-ref HEAD)" != "$BRANCH" ]]; then
  echo "error: releases are published from ${BRANCH}; check it out first" >&2
  exit 1
fi

if ! git fetch --quiet --tags "$REMOTE" "$BRANCH"; then
  echo "error: could not fetch ${REMOTE}" >&2
  exit 1
fi
if ! git merge-base --is-ancestor "${REMOTE}/${BRANCH}" HEAD; then
  echo "error: ${BRANCH} is behind or diverged from ${REMOTE}/${BRANCH}; pull/rebase first" >&2
  exit 1
fi

# --- derive the GitHub owner/repo slug from the remote ----------------------
# The remote URL may embed credentials; everything up to '@' is dropped so a
# token is never printed.
slug="$(git remote get-url "$REMOTE")"
slug="${slug%.git}"
slug="${slug##*@}"
slug="${slug#https://}"
slug="${slug#http://}"
slug="${slug#github.com[:/]}"
if [[ ! "$slug" =~ ^[^/:]+/[^/:]+$ ]]; then
  echo "error: could not derive owner/repo from ${REMOTE} remote" >&2
  exit 1
fi
SLUG="$slug"

# --- current version + suggested next patch bump ---------------------------
# Same sed as ci.yml's Package / Resolve tag steps.
CURRENT="$(sed -n 's/^project(specialdlp VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
if [[ ! "$CURRENT" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
  echo "error: could not read VERSION from CMakeLists.txt" >&2
  exit 1
fi
suggest="${BASH_REMATCH[1]}.${BASH_REMATCH[2]}.$(( BASH_REMATCH[3] + 1 ))"
RULES="$(sed -n 's/^#define DLPVERSION[[:space:]]*\([0-9]*\).*/\1/p' specialdlp/specialdlp/antiLeech.h | head -1)"

tag_released() { git rev-parse -q --verify "refs/tags/$1" >/dev/null; }

echo "Current version: ${CURRENT}  (rules DLP v${RULES})"
echo "Branch:          ${BRANCH} (remote: ${REMOTE} -> ${SLUG})"
if tag_released "v${CURRENT}"; then
  echo "                 v${CURRENT} is already released -- pick a new version"
fi
echo

# --- pick the version -------------------------------------------------------
echo "Press Enter to release the current version (${CURRENT}),"
echo "or type a new version to bump to (suggested next: ${suggest})."
echo
read -r -p "Version to release [${CURRENT}]: " NEW
NEW="${NEW:-$CURRENT}"
NEW="${NEW#v}"

if [[ ! "$NEW" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "error: '${NEW}' is not a valid MAJOR.MINOR.PATCH version" >&2
  exit 1
fi
TAG="v${NEW}"
if tag_released "$TAG"; then
  echo "error: tag ${TAG} already exists" >&2
  exit 1
fi

BUMP=0
[[ "$NEW" != "$CURRENT" ]] && BUMP=1

AHEAD="$(git rev-list --count "${REMOTE}/${BRANCH}..HEAD")"
if [[ "$BUMP" -eq 0 && "$AHEAD" -eq 0 ]]; then
  echo
  echo "Nothing to push: ${BRANCH} is already at ${REMOTE}. Its CI run either published"
  echo "${TAG} already or failed. To (re)build and release this commit:"
  echo "  https://github.com/${SLUG}/actions/workflows/ci.yml -> Run workflow -> ${BRANCH}"
  exit 0
fi

# --- confirm ----------------------------------------------------------------
echo
echo "About to:"
if [[ "$BUMP" -eq 1 ]]; then
  echo "  bump     : ${CURRENT} -> ${NEW}  (CMakeLists.txt, ${VCXPROJ})"
  echo "  commit   : release: ${TAG}"
else
  echo "  commit   : (none -- releasing HEAD at version ${CURRENT})"
fi
echo "  push     : ${REMOTE} ${BRANCH}  (${AHEAD} commit(s)$([[ "$BUMP" -eq 1 ]] && echo " + release commit"))"
echo "  CI       : on green, tags ${TAG} and publishes the release"
echo
read -r -p "Proceed? [y/N]: " CONFIRM
if [[ ! "$CONFIRM" =~ ^[Yy]$ ]]; then
  echo "aborted"
  exit 1
fi

# --- bump + commit ----------------------------------------------------------
if [[ "$BUMP" -eq 1 ]]; then
  sed -i.bak "s/^project(specialdlp VERSION ${CURRENT//./\\.} /project(specialdlp VERSION ${NEW} /" CMakeLists.txt
  sed -i.bak "s|<SdlpLibVersion>${CURRENT//./\\.}</SdlpLibVersion>|<SdlpLibVersion>${NEW}</SdlpLibVersion>|" "$VCXPROJ"
  rm -f CMakeLists.txt.bak "$VCXPROJ.bak"

  if ! grep -q "^project(specialdlp VERSION ${NEW//./\\.} " CMakeLists.txt \
     || ! grep -q "<SdlpLibVersion>${NEW//./\\.}</SdlpLibVersion>" "$VCXPROJ"; then
    echo "error: version bump did not apply; reverting" >&2
    git checkout -- CMakeLists.txt "$VCXPROJ"
    exit 1
  fi

  git add CMakeLists.txt "$VCXPROJ"
  git commit -m "release: ${TAG}"
fi

# --- push -------------------------------------------------------------------
git push "$REMOTE" "$BRANCH"

echo
echo "Pushed ${BRANCH}. CI builds all platforms and publishes ${TAG} when green:"
echo "  https://github.com/${SLUG}/actions/workflows/ci.yml"
echo "  https://github.com/${SLUG}/releases/tag/${TAG}"
echo
echo "eMuleQt: bump the submodule pointer (external/specialdlp) to pick it up."
