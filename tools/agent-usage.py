#!/usr/bin/env python3
"""
What the project's subagents (dawPlanFeature, dawImplement, dawTheory, dawWebsite, dawIdea...) spent their tokens on,
read from the transcripts Claude Code keeps in ~/.claude/projects/<this repo>/<session>/subagents/.

A run's cost grows faster than its length: every turn re-reads the whole context so far, so the column that matters
most is "re-read", the context summed over all turns. The report shows, per agent type, the turns, the context at
the end and the tokens re-read; what filled the context (tool results by kind, estimated at 4 characters a token and
about 1,600 tokens an image); and the code looked up most often (Read, Grep, and grep/sed/cat in Bash).

    tools/agent-usage.py                     every run, summed by agent type
    tools/agent-usage.py --since 2026-10-27  only runs started on or after that day
    tools/agent-usage.py --runs              also one line per run (date, type, turns, re-read, its description)
    tools/agent-usage.py --type dawImplement only that agent type

It only reads the transcripts; it changes nothing.
"""

import argparse
import collections
import glob
import json
import os
import re

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__))) + "/"
PROJECT = os.path.expanduser("~/.claude/projects/" + REPO.rstrip("/").replace("/", "-"))
IMAGE_TOKENS = 1600


def kind_of (name, args):
    """Groups a tool call by what it fetched, so the report can say what fills the context."""
    if name == "Read":
        path = args.get ("file_path", "")
        if path.endswith ((".png", ".jpg")):
            return "Read image"
        path = path.replace (REPO, "")
        for prefix, label in (("plans/", "Read plan"), ("docs/", "Read docs"), ("tests/", "Read tests")):
            if path.startswith (prefix):
                return label
        if path.startswith ("src/"):
            return "Read src (part)" if args.get ("limit") else "Read src (whole)"
        return "Read html" if path.endswith (".html") else "Read other"
    if name == "Bash":
        command = args.get ("command", "")
        if "build.sh" in command or "cmake --build" in command or "DawItTests" in command:
            return "Bash build/test"
        if "<<" in command and "python3" in command:
            return "Bash edit (script)"
        if re.search (r"\b(grep|sed -n|cat|awk|head|tail)\b", command):
            return "Bash look"
        return "Bash other"
    return name


def looked_up_files (name, args):
    """The repository's source and test files a lookup call names."""
    if name == "Read":
        path = args.get ("file_path", "").replace (REPO, "")
        return {path} if path.startswith (("src/", "tests/")) else set()
    text = ""
    if name == "Grep":
        text = f"{args.get ('pattern', '')} {args.get ('path', '')}"
    elif name == "Bash" and kind_of (name, args) == "Bash look":
        text = args.get ("command", "")
    return set (re.findall (r"(?:src|tests)/[\w/]+\.(?:h|cpp)", text))


def result_tokens (content):
    if isinstance (content, str):
        return len (content) // 4
    total = 0
    for part in content or []:
        if part.get ("type") == "image":
            total += IMAGE_TOKENS
        elif part.get ("type") == "text":
            total += len (part.get ("text", "")) // 4
    return total


def read_run (path):
    meta_path = path[:-len (".jsonl")] + ".meta.json"
    meta = json.load (open (meta_path)) if os.path.exists (meta_path) else {}
    run = { "type": meta.get ("agentType", "?"), "description": meta.get ("description", ""), "started": "",
            "turns": 0, "peak": 0, "reread": 0, "output": 0, "results": collections.Counter(), "files": collections.Counter() }
    pending = {}

    for line in open (path):
        try:
            entry = json.loads (line)
        except ValueError:
            continue

        run["started"] = run["started"] or entry.get ("timestamp", "")[:10]
        message = entry.get ("message") or {}
        usage = message.get ("usage")

        if usage and entry.get ("type") == "assistant":
            context = usage.get ("input_tokens", 0) + usage.get ("cache_read_input_tokens", 0) + usage.get ("cache_creation_input_tokens", 0)
            run["turns"] += 1
            run["peak"] = max (run["peak"], context)
            run["reread"] += context
            run["output"] += usage.get ("output_tokens", 0)

        content = message.get ("content")
        if not isinstance (content, list):
            continue

        for block in content:
            if block.get ("type") == "tool_use":
                name, args = block.get ("name"), block.get ("input", {})
                pending[block.get ("id")] = kind_of (name, args)
                for f in looked_up_files (name, args):
                    run["files"][f] += 1
            elif block.get ("type") == "tool_result":
                run["results"][pending.get (block.get ("tool_use_id"), "?")] += result_tokens (block.get ("content"))

    return run


def main():
    parser = argparse.ArgumentParser (description = "What the project's subagents spent their tokens on.")
    parser.add_argument ("--since", help = "only runs started on or after this day (YYYY-MM-DD)")
    parser.add_argument ("--type", help = "only this agent type, such as dawImplement")
    parser.add_argument ("--runs", action = "store_true", help = "also list every run")
    parser.add_argument ("--top", type = int, default = 20, help = "how many looked-up files to list (default 20)")
    options = parser.parse_args()

    runs = [read_run (p) for p in sorted (glob.glob (PROJECT + "/*/subagents/*.jsonl"))]
    runs = [r for r in runs if (not options.since or r["started"] >= options.since) and (not options.type or r["type"] == options.type)]

    if not runs:
        print (f"No subagent runs found in {PROJECT}")
        return

    by_type = collections.defaultdict (list)
    for r in runs:
        by_type[r["type"]].append (r)

    print (f"{len (runs)} subagent runs from {min (r['started'] for r in runs)} to {max (r['started'] for r in runs)}\n")
    print (f"{'agent':18} {'runs':>4} {'turns/run':>9} {'end context':>12} {'re-read/run':>12} {'re-read total':>14} {'output/run':>11}")

    for name, group in sorted (by_type.items(), key = lambda item: -sum (r["reread"] for r in item[1])):
        n = len (group)
        print (f"{name:18} {n:>4} {sum (r['turns'] for r in group) // n:>9} {sum (r['peak'] for r in group) // n:>12,} "
               f"{sum (r['reread'] for r in group) // n:>12,} {sum (r['reread'] for r in group):>14,} {sum (r['output'] for r in group) // n:>11,}")

    print ("\nWhat filled the context (tool results, tokens per run)")
    for name, group in sorted (by_type.items(), key = lambda item: -sum (r["reread"] for r in item[1])):
        totals = collections.Counter()
        for r in group:
            totals.update (r["results"])
        all_results = sum (totals.values()) or 1
        parts = ", ".join (f"{k} {v // len (group):,} ({100 * v // all_results}%)" for k, v in totals.most_common (5))
        print (f"  {name}: {parts}")

    looked_by = collections.defaultdict (set)
    looked = collections.Counter()
    for i, r in enumerate (runs):
        for f, count in r["files"].items():
            looked_by[f].add (i)
            looked[f] += count

    print (f"\nCode looked up by the most runs (runs, lookups)")
    for f, who in sorted (looked_by.items(), key = lambda item: (-len (item[1]), -looked[item[0]]))[:options.top]:
        print (f"  {len (who):3} {looked[f]:5}  {f}")

    if options.runs:
        print ("\nRuns")
        for r in sorted (runs, key = lambda r: r["started"]):
            print (f"  {r['started']}  {r['type']:15} {r['turns']:4} turns  {r['reread']:>12,} re-read  {r['description'][:70]}")


if __name__ == "__main__":
    main()
