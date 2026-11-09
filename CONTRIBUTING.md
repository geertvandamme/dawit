# Contributing to DawIt

Thanks for wanting to help. DawIt is a free desktop app for exploring music theory and making music, where everything explains itself to a beginner. This page says how contributions are licensed, how to build and test, and what a change needs before it's merged.

## Ideas and bugs

- **Ideas** are [GitHub issues](https://github.com/geertvandamme/dawit/issues). Search first: many ideas already have an issue, a plan in [plans/](plans/README.md) or a place on the [DawIt project](https://github.com/users/geertvandamme/projects/2).
- **Bugs:** open an issue with your DawIt version, your operating system, the steps that cause the problem, and what you expected to happen.
- **Security problems:** report them privately, as [SECURITY.md](SECURITY.md) describes, never in a public issue.

For anything bigger than a small fix, comment on the issue before you start, so the plan can be agreed first.

## Licence of contributions

DawIt is licensed under the **GNU Affero General Public License v3.0 only** (`AGPL-3.0-only`); see [LICENSE](LICENSE).

- By submitting a contribution, you agree that it is licensed under `AGPL-3.0-only`, the same licence as the rest of DawIt. There is no contributor licence agreement (CLA), and you keep the copyright in your work.
- Only submit work you have the right to submit. Say so in the pull request if part of it isn't your own work, and name its source and licence.
- Third-party code keeps its own licence and stays clearly apart from DawIt's code. A new or upgraded library, model, font, dictionary or asset must have a compatible licence and be pinned with a hash. Its licence and notices go into the root [LICENSE](LICENSE) inventory, [licenses/](licenses/README.md) and the README's *License note* in the same change ([docs/architecture.md](docs/architecture.md#libraries-and-models)).

## Building and testing

The requirements for each platform are in the README's *Building* section. On macOS, Linux or Git Bash on Windows:

```bash
./build.sh --test
```

This builds the Release app and the `DawItTests` engine tests, then runs the tests. It must end with `All checks passed`.

- Fix every compiler warning in DawIt's own code and tests. A warning in third-party code stays only once it has been reviewed and documented, as in [docs/release-third-party-warning-review.md](docs/release-third-party-warning-review.md).
- Run the app with example tracks: `./run.sh -- --demo`.
- Check a UI change in the real app with a screenshot list: `./run.sh -- --screenshots shots.json` ([docs/recipes.md](docs/recipes.md#checking-in-the-real-app)).

## What a change needs

The rules every change follows are in [CLAUDE.md](CLAUDE.md). The project's AI agents use the same file, and it applies to human contributors too. In short:

- **Code style:** JUCE style (Allman braces, 4 spaces, a space before parentheses, `camelCase` functions, `PascalCase` types), everything in the `daw` namespace, and a `/** */` comment on every class and public function.
- **Tests:** new engine, theory and analysis code gets checks in [tests/EngineTests.cpp](tests/EngineTests.cpp), listed in [docs/tests.md](docs/tests.md). Anything saved in a project gets a save-and-load test.
- **Explaining itself:** every control has a tooltip. Anything with theory behind it has a guide. Facts with numbers are tested or come from a source listed in [SOURCES.md](SOURCES.md).
- **Text:** British spelling (colour, centre, analyse), short sentences and concrete examples.
- **Docs:** a user-visible feature adds a README bullet under *Features*, lists its files in [docs/code-layout.md](docs/code-layout.md) and its checks in [docs/tests.md](docs/tests.md).
- **Website:** changes to `website/` follow [website/MAINTAINING.md](website/MAINTAINING.md) and go in a separate commit from app changes.

Where things live and how to add each kind of feature: [docs/architecture.md](docs/architecture.md), [docs/recipes.md](docs/recipes.md) and [docs/ui-design.md](docs/ui-design.md).

## Pull requests

- Keep each pull request to one feature or fix, and link its issue.
- Write the commit subject as what the user gets, for example "A lyrics track you time by dragging or tapping".
- Say how you tested it: the platform, `./build.sh --test`, and screenshots for a UI change.
