# DawIt Harmoniser as a VST3 and Audio Unit

Status: Planned

## Goal

Make DawIt Harmoniser available in other DAWs as a VST3 and, on macOS, an Audio Unit, with its existing controls and guide.

## Decisions for you

The user requested both formats. Keep the existing name and permanent code `Dhrm`; use the manual key and scale in other hosts when no DawIt song context is available.

## What the user does and sees

The effect opens in a host with its existing editor, starting points and guide. Its settings are saved by the host; the existing shared preset library remains available. No new music theory or dependency is needed.

## Steps

- [ ] Separate the Harmoniser editor and guide's song-context access from app-only Tune UI, providing safe defaults in plugin hosts. Keep shared section colours and time-signature calculations accessible without linking app UI.
- [ ] Register the required existing processing and guide sources in the plugin target, remove `APP_ONLY`, and update the registry checks.
- [ ] Build both plugin formats and run the engine tests; verify audio, state recall, manual key and the editor and guide in a host. Validate with `pluginval` and `auval` when available.
- [ ] Update the app documentation and issue progress, documenting any validation still outstanding.

## Tests

Check the built-in registry and state round trip, plugin builds, manual key behaviour without song context, and host audio and guide operation.

## Website hand-off

Update the existing Harmoniser and plugin-format descriptions after the build and validation; no new library or guide is involved.
