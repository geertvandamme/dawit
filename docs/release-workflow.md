# Creating a draft release

This is the shared workflow for `/daw-release` in Claude and `$daw-release` in
Codex. It creates a new version tag and starts the GitHub Actions draft release;
it does not publish the draft. Read `docs/draft-releases.md`, the current
`.github/workflows/release.yml` and `DISTRIBUTION_TODO.md` before acting.

1. Use the tag supplied by the user. If none was supplied, list up to 20 recent
   repository tags for information, with `gh api 'repos/geertvandamme/dawit/tags?per_page=20' --jq '.[].name'`,
   and ask the user to choose a **new** tag. If none exist, say so and ask for
   one. Do not pick the next number yourself. Listing is read-only.
2. Check the chosen name is `v` followed by the exact numeric project version
   in `CMakeLists.txt`, as required by the release workflow. Verify it is a
   valid Git ref and does not already exist locally or remotely. Never move,
   delete, reuse or force-push a tag. If there is a mismatch or an existing tag,
   explain the problem and ask for a suitable new tag; do not silently bump the
   version or commit changes.
3. Determine the exact commit to release (the user's specified commit, otherwise
   the current HEAD). Check that commit contains the workflow and matching
   version and is already reachable on the intended `geertvandamme/dawit`
   remote. Confirm the remote URL identifies that repository. Check relevant
   release changes in the working tree: uncommitted files are not part of the
   release, so stop and explain if they are needed. Do not commit or push a
   branch unless separately asked. Check the known distribution limitations;
   the resulting draft is a candidate, not a publication-ready release.
4. A user request to run this release workflow authorises creating the selected
   new tag at that commit and pushing that tag to the verified remote. Create an
   explicit tag ref and push only that ref, without force. If a push has an
   uncertain outcome, inspect the remote tag and workflow before retrying. If
   the tag is already present at another commit, stop.
5. Find the Actions run for this tag and commit, wait for the platform jobs and
   draft creation, and report the run and draft URLs, tag, commit, platform
   outcomes and remaining release blockers. If the run fails or cannot be
   observed, report its status and logs or access problem; do not claim success.
   Never publish the draft, replace assets or push another tag unless asked.

If the tag-push workflow is missing from the tagged commit, or GitHub Actions,
permissions or the remote are unavailable, report the blocker rather than
creating a tag that cannot produce the requested draft.
