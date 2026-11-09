# Security

DawIt runs on your own computer. It opens project and audio files, loads plugins, downloads models when you ask it to, and talks to Ollama on your machine over HTTP. A problem in any of those could let a crafted file, plugin or download harm someone's computer, so please report it privately.

## Reporting a problem

Use **[Report a vulnerability](https://github.com/geertvandamme/dawit/security/advisories/new)** on the repository's *Security* tab. The report is visible only to the maintainers until a fix is published.

Please don't open a public issue, pull request or discussion for a security problem.

A useful report says:

- which DawIt version and operating system you used;
- what an attacker would need (a file the user opens, a plugin, a network position);
- the steps to reproduce it, with a small example file if there is one;
- what happens, and what you expected.

## What happens next

DawIt is a volunteer project, so replies are on a best-effort basis. You'll get an answer in the advisory thread, and a fix goes into the next release. The release notes credit you unless you'd rather not be named.

## Supported versions

Only the latest release gets security fixes. Before the first published release, fixes go into the `main` branch.

## Out of scope

- Problems in a third-party plugin, Ollama or a model itself: please report those to their makers. A way for one of them to harm DawIt or the user's files is in scope.
- Anything that needs someone who already controls the user's account or computer.
