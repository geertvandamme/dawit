#!/usr/bin/env python3
"""
Looks up and edits DawIt's ideas list, FEATURE_IDEAS.html (and features_done.html once it exists), so an agent
needs a call or two instead of many greps through a 400 KB page. The page itself stays as it is, for people.

    tools/ideas.py index [--category WORDS] [--status new|partly|done]
        every idea in one line each, by category and subgroup: title, #anchor and status
    tools/ideas.py show ANCHOR-OR-WORDS ...
        one idea or several as plain text: category, status, plan, the text with links as "Title (#anchor)",
        and the ideas it ties in with; words that match several titles list those instead
    tools/ideas.py search WORDS [--limit N]
        the ideas that match best (titles count more than text), each with a line of context
    tools/ideas.py link-plan ANCHOR plans/name.md
        puts the idea's Plan line after its opening paragraph, replacing an older one
    tools/ideas.py add --category WORDS [--subgroup WORDS] --title "Title" --body FILE
        adds a new idea: its section (the body is the HTML after the <h2>: the opening <p>, then the <ul>) at the
        end of its category's group, its line in the index (in the subgroup, or the category's last list) marked
        New, and every count on the page
    tools/ideas.py set-tag ANCHOR new|partly|done
        changes an idea's label in the index and every count, after its status block changed
    tools/ideas.py check
        checks the page: every idea in the index exactly once, unique anchors, links that resolve, and the counts

The edits write the file in place and print what they changed; add --dry-run to see it first. --dir works on the
pages in another folder (a copy, to try an edit).
"""

import argparse
import html
import os
import re
import sys

REPO = os.path.dirname (os.path.dirname (os.path.abspath (__file__)))
PAGES = ["FEATURE_IDEAS.html", "features_done.html"]
TAGS = { "new": ("tag-new", "New"), "partly": ("tag-partly", "Partly done"), "done": ("tag-done", "Done") }


def slug (title):
    """The anchor GitHub would make of a title: lower case, punctuation dropped, spaces as hyphens."""
    text = html.unescape (title).lower()
    text = re.sub (r"[^\w\- ]", "", text)
    return text.replace (" ", "-")


def to_text (fragment):
    """HTML to plain text: lists as indented bullets, links to other ideas as "Title (#anchor)"."""
    s = re.sub (r'<a class="to-top".*?</a>', "", fragment, flags = re.S)
    s = re.sub (r'<a href="(#[^"]+)">(.*?)</a>', lambda m: f"{m.group (2)} ({m.group (1)})", s, flags = re.S)
    s = re.sub (r'<a href="([^"#][^"]*)">(.*?)</a>', lambda m: f"{m.group (2)} ({m.group (1)})", s, flags = re.S)
    depth = 0
    out = []

    for token in re.split (r"(<[^>]+>)", s):
        if not token.startswith ("<"):
            out.append (re.sub (r"\s+", " ", token))
            continue
        tag = token.lower()
        if tag.startswith (("<ul", "<ol")):
            depth += 1
        elif tag.startswith (("</ul", "</ol")):
            depth -= 1
        elif tag.startswith ("<li"):
            out.append ("\n" + "  " * max (0, depth - 1) + "- ")
        elif tag.startswith (("<p", "<h4", "<div")):
            out.append ("\n")
        elif tag.startswith (("</p", "</h4", "</h2")):
            out.append ("\n")

    text = html.unescape ("".join (out))
    text = re.sub (r"[ \t]+\n", "\n", text)
    return re.sub (r"\n{3,}", "\n\n", text).strip()


class Page:
    """One ideas page, parsed: the index's categories and entries, and the idea sections."""

    def __init__ (self, name):
        self.name = name
        self.path = os.path.join (REPO, name)
        self.text = open (self.path, encoding = "utf-8").read()
        self.parse()

    def parse (self):
        # The structure comment at the top shows example HTML; blank it out (keeping every offset) so it isn't parsed.
        self.masked = re.sub (r"<!--.*?-->", lambda m: " " * len (m.group (0)), self.text, flags = re.S)
        t = self.masked
        nav = re.search (r'<nav class="categories".*?</nav>', t, re.S)
        self.nav = nav
        self.categories = []   # dicts: name, colour, group, entries [(anchor, title, status, subgroup)]
        for block in re.finditer (r'<details class="category (c\d+)"[^>]*>(.*?)</details>', nav.group (0), re.S):
            body = block.group (2)
            head = re.search (r'<h3><a href="#([^"]+)">(.*?)</a>', body)
            entry = { "colour": block.group (1), "group": head.group (1), "name": html.unescape (head.group (2)), "entries": [] }
            subgroup = ""
            for m in re.finditer (r'<h4>(.*?)</h4>|<li><a href="#([^"]+)">(.*?)</a>\s*<span class="tag (tag-\w+)">', body, re.S):
                if m.group (1) is not None:
                    subgroup = html.unescape (m.group (1))
                else:
                    status = { "tag-new": "new", "tag-partly": "partly", "tag-done": "done" }.get (m.group (4), "?")
                    entry["entries"].append ((m.group (2), html.unescape (m.group (3)), status, subgroup))
            self.categories.append (entry)

        self.sections = {}
        for m in re.finditer (r'<section class="idea[^"]*" id="([^"]+)">(.*?)</section>', t, re.S):
            title = re.search (r"<h2>(.*?)</h2>", m.group (2), re.S)
            self.sections[m.group (1)] = { "title": to_text (title.group (1)) if title else m.group (1),
                                           "html": m.group (2), "start": m.start(), "end": m.end() }

    def where (self, anchor):
        for c in self.categories:
            for a, title, status, subgroup in c["entries"]:
                if a == anchor:
                    return c, status, subgroup
        return None, "?", ""

    def write (self, dry_run, what):
        if dry_run:
            print (f"(dry run) would change {self.name}: {what}")
            return
        open (self.path, "w", encoding = "utf-8").write (self.text)
        print (f"{self.name}: {what}")
        self.parse()

    def recount (self):
        """Rewrites every count on the page from the index's labels: the stats, each category's count and summary,
        and each group heading's count."""
        totals = { "new": 0, "partly": 0, "done": 0 }
        t = self.text
        for c in self.categories:
            counts = { "new": 0, "partly": 0, "done": 0 }
            for entry in c["entries"]:
                if entry[2] in counts:
                    counts[entry[2]] += 1
            for k in totals:
                totals[k] += counts[k]
            n = len (c["entries"])
            summary = (f'<p class="cat-summary"><span class="tag tag-done">{counts["done"]} done</span> '
                       f'<span class="tag tag-partly">{counts["partly"]} partly done</span> '
                       f'<span class="tag tag-new">{counts["new"]} new</span></p>')
            g = re.escape (c["group"])
            t = re.sub (r'(<h3><a href="#' + g + r'">.*?</a> <span class="count">)\d+(</span>)', lambda m: f"{m.group (1)}{n}{m.group (2)}", t, count = 1, flags = re.S)
            t = re.sub (r'(<h3><a href="#' + g + r'">.*?</h3>)<p class="cat-summary">.*?</p>', lambda m: m.group (1) + summary, t, count = 1, flags = re.S)
            t = re.sub (r'(<div class="group-head [^"]*" id="' + g + r'">\s*<h2 class="group-title">.*? <span class="count">)\d+(</span>)',
                        lambda m: f"{m.group (1)}{n}{m.group (2)}", t, count = 1, flags = re.S)
        total = sum (totals.values())
        t = re.sub (r'<span><b>\d+</b> ideas</span>', f"<span><b>{total}</b> ideas</span>", t, count = 1)
        t = re.sub (r'<span><b>\d+</b> categories</span>', f"<span><b>{len (self.categories)}</b> categories</span>", t, count = 1)
        for k, label in (("new", "new"), ("partly", "partly done"), ("done", "done")):
            t = re.sub (r'(<span class="p-' + k + r'"><b>)\d+(</b> ' + label + r'</span>)', lambda m: f"{m.group (1)}{totals[k]}{m.group (2)}", t, count = 1)
        changed = t != self.text
        self.text = t
        return changed


def load_pages():
    return [Page (n) for n in PAGES if os.path.exists (os.path.join (REPO, n))]


def find_category (page, words):
    words = words.lower()
    matches = [c for c in page.categories if words in c["name"].lower()]
    if len (matches) != 1:
        sys.exit (f"category '{words}' matches {len (matches)} of: " + ", ".join (c["name"] for c in page.categories))
    return matches[0]


def find_ideas (pages, query):
    """An anchor, or words that match titles: all of the words, in any order."""
    for p in pages:
        if query.lstrip ("#") in p.sections:
            return [(p, query.lstrip ("#"))]
    words = query.lower().split()
    return [(p, a) for p in pages for a, s in p.sections.items() if all (w in s["title"].lower() for w in words)]


def cmd_index (pages, args):
    for p in pages:
        if len (pages) > 1:
            print (f"# {p.name}")
        for c in p.categories:
            if args.category and args.category.lower() not in c["name"].lower():
                continue
            entries = [e for e in c["entries"] if not args.status or e[2] == args.status]
            if not entries:
                continue
            print (f"\n## {c['name']} ({len (c['entries'])})")
            subgroup = None
            for anchor, title, status, sub in entries:
                if sub != subgroup:
                    subgroup = sub
                    if sub:
                        print (f"### {sub}")
                print (f"- {title}  #{anchor}  [{TAGS.get (status, ('', '?'))[1]}]")


def cmd_show (pages, args):
    # Several anchors show several ideas; anything else is one set of words to match titles by.
    known = {a for p in pages for a in p.sections}
    queries = args.what if all (q.lstrip ("#") in known for q in args.what) else [" ".join (args.what)]
    for query in queries:
        found = find_ideas (pages, query)
        if len (found) != 1:
            print (f"'{query}' matches {len (found)} ideas" + (":" if found else "; try search."))
            for p, a in found[:30]:
                print (f"  {p.sections[a]['title']}  #{a}")
            continue
        p, anchor = found[0]
        s = p.sections[anchor]
        category, status, subgroup = p.where (anchor)
        plan = re.search (r'<strong>Plan:</strong>\s*<a href="([^"]+)"', s["html"])
        ties = re.findall (r'<strong>Ties in with:?</strong>:?(.*?)</li>', s["html"], re.S)
        tied = re.findall (r'href="#([^"]+)"', ties[0]) if ties else []
        print (f"# {s['title']}  #{anchor}")
        print (f"{p.name} · {category['name'] if category else 'not in the index'}{' / ' + subgroup if subgroup else ''} · "
               f"{TAGS.get (status, ('', '?'))[1]}{' · plan: ' + plan.group (1) if plan else ''}")
        body = re.sub (r"^.*?</h2>", "", s["html"], count = 1, flags = re.S)
        print ("\n" + to_text (body))
        if tied:
            print ("\nTies in with: " + "; ".join (f"{p.sections[t]['title'] if t in p.sections else '?'} (#{t})" for t in tied))
        print()


def cmd_search (pages, args):
    words = [w.lower() for w in args.words]
    scored = []
    for p in pages:
        for anchor, s in p.sections.items():
            title = s["title"].lower()
            text = to_text (s["html"]).lower()
            score = sum (3 * (w in title) + min (text.count (w), 5) for w in words)
            if score and all (w in title or w in text for w in words) or score >= 3 * len (words):
                sentences = re.split (r"(?<=[.:;])\s+|\n", to_text (s["html"]))
                best = max (sentences, key = lambda x: sum (w in x.lower() for w in words)) if sentences else ""
                scored.append ((score, p, anchor, best))
    for score, p, anchor, best in sorted (scored, key = lambda x: -x[0])[:args.limit]:
        status = p.where (anchor)[1]
        print (f"- {p.sections[anchor]['title']}  #{anchor}  [{TAGS.get (status, ('', '?'))[1]}]")
        print (f"    {best[:200]}")
    if not scored:
        print ("No idea matches all of: " + " ".join (args.words))


def cmd_link_plan (pages, args):
    found = find_ideas (pages, args.anchor)
    if len (found) != 1:
        sys.exit (f"'{args.anchor}' matches {len (found)} ideas; give its anchor")
    p, anchor = found[0]
    plan = args.plan if args.plan.startswith ("plans/") else "plans/" + args.plan
    if not os.path.exists (os.path.join (REPO, plan)):
        print (f"warning: {plan} doesn't exist yet")
    s = p.sections[anchor]
    line = f'<p><strong>Plan:</strong> <a href="{plan}">{plan}</a></p>'
    body = s["html"]
    if re.search (r"<p><strong>Plan:</strong>.*?</p>", body):
        new_body = re.sub (r"<p><strong>Plan:</strong>.*?</p>", line, body, count = 1)
    else:
        # After the opening paragraph: the first plain <p> after the title (not an old "Done:" line or a status block).
        h2 = body.index ("</h2>") + len ("</h2>")
        opening = re.compile (r"<p>(?!<strong>)(?!<em>Status).*?</p>", re.S).search (body, h2)
        if not opening:
            sys.exit (f"#{anchor} has no opening paragraph to put the plan after")
        new_body = body[:opening.end()] + "\n" + line + body[opening.end():]
    p.text = p.text[:s["start"]] + p.text[s["start"]:s["end"]].replace (body, new_body, 1) + p.text[s["end"]:]
    p.write (args.dry_run, f"#{anchor} links {plan}")


def cmd_add (pages, args):
    p = pages[0]
    category = find_category (p, args.category)
    anchor = slug (args.title)
    if any (anchor in q.sections for q in pages):
        sys.exit (f"#{anchor} already exists: add to that idea instead (tools/ideas.py show {anchor})")
    body = open (args.body, encoding = "utf-8").read().strip()
    if not body.startswith ("<p>"):
        sys.exit ("the body starts with the opening <p> (the HTML after the <h2>)")
    title_html = html.escape (args.title, quote = False)
    section = (f'<section class="idea {category["colour"]}" id="{anchor}">\n'
               f'<a class="to-top" href="#ideas-by-category" title="Back to the overview" aria-label="Back to the overview">'
               f'<span aria-hidden="true">&uarr;</span> Top</a>\n<h2>{title_html}</h2>\n{body}\n</section>\n\n')

    # The section goes at the end of its category's group: before the next group's heading, or </main>.
    t = p.text
    group_at = t.index (f'id="{category["group"]}"')
    next_group = re.compile (r'<div class="group-head ').search (t, group_at + 1)
    insert_at = next_group.start() if next_group else t.rindex ("</main>")
    t = t[:insert_at] + section + t[insert_at:]

    # The index line: at the end of the subgroup's list, or of the category's last list.
    nav = re.search (r'<nav class="categories".*?</nav>', t, re.S)
    block = re.search (r'<details class="category ' + category["colour"] + r'"[^>]*>.*?</details>', nav.group (0), re.S)
    block_start = nav.start() + block.start()
    block_text = block.group (0)
    if args.subgroup:
        heads = [m for m in re.finditer (r"<h4>(.*?)</h4>", block_text) if args.subgroup.lower() in html.unescape (m.group (1)).lower()]
        if len (heads) != 1:
            sys.exit (f"subgroup '{args.subgroup}' matches {len (heads)} of: " + ", ".join (html.unescape (h) for h in re.findall (r"<h4>(.*?)</h4>", block_text)))
        list_end = block_text.index ("</ul>", heads[0].end())
    else:
        list_end = block_text.rindex ("</ul>")
    li = f'  <li><a href="#{anchor}">{title_html}</a> <span class="tag tag-new">New</span></li>\n    '
    at = block_start + list_end
    t = t[:at] + li + t[at:]

    p.text = t
    p.parse()
    p.recount()
    p.write (args.dry_run, f"added #{anchor} to {category['name']}{' / ' + args.subgroup if args.subgroup else ''}, marked New, counts updated")


def cmd_set_tag (pages, args):
    for p in pages:
        if args.anchor in p.sections:
            break
    else:
        sys.exit (f"no idea #{args.anchor}")
    cls, label = TAGS[args.status]
    pattern = r'(<li><a href="#' + re.escape (args.anchor) + r'">.*?</a>\s*)<span class="tag tag-\w+">[^<]*</span>'
    t, n = re.subn (pattern, lambda m: f'{m.group (1)}<span class="tag {cls}">{label}</span>', p.text, count = 1, flags = re.S)
    if n != 1:
        sys.exit (f"#{args.anchor} has no line in the index")
    p.text = t
    p.parse()
    p.recount()
    p.write (args.dry_run, f"#{args.anchor} is {label} in the index, counts updated")


def cmd_check (pages, args):
    problems = []
    anchors = {a for p in pages for a in p.sections}
    for p in pages:
        indexed = [e[0] for c in p.categories for e in c["entries"]]
        for a in p.sections:
            n = indexed.count (a)
            if n != 1:
                problems.append (f"{p.name}: #{a} is in the index {n} times")
        for a in set (indexed) - set (p.sections):
            problems.append (f"{p.name}: the index links #{a}, which has no section")
        ids = re.findall (r'<[a-z][^>]*\bid="([^"]+)"', p.masked)
        for a in {i for i in ids if ids.count (i) > 1}:
            problems.append (f"{p.name}: id #{a} is used more than once")
        known = set (ids) | anchors
        for link in set (re.findall (r'<a [^>]*href="#([^"]+)"', p.masked)) - known:
            problems.append (f"{p.name}: a link to #{link} goes nowhere")
        for s in p.sections.values():
            for target in re.findall (r'href="(plans/[^"#]+)"', s["html"]):
                if not os.path.exists (os.path.join (REPO, target)):
                    problems.append (f"{p.name}: #{slug (s['title'])} links {target}, which doesn't exist")
        before = p.text
        if p.recount():
            problems.append (f"{p.name}: some counts don't match the index's labels (run set-tag, or fix by hand)")
        p.text = before
    print ("\n".join (problems) if problems else "The ideas pages are consistent.")
    sys.exit (1 if problems else 0)


def main():
    parser = argparse.ArgumentParser (description = "Look up and edit DawIt's ideas list.")
    parser.add_argument ("--dir", help = "the folder with the pages (default: the repository)")
    sub = parser.add_subparsers (dest = "command", required = True)
    a = sub.add_parser ("index"); a.add_argument ("--category"); a.add_argument ("--status", choices = list (TAGS))
    a = sub.add_parser ("show"); a.add_argument ("what", nargs = "+")
    a = sub.add_parser ("search"); a.add_argument ("words", nargs = "+"); a.add_argument ("--limit", type = int, default = 8)
    a = sub.add_parser ("link-plan"); a.add_argument ("anchor"); a.add_argument ("plan"); a.add_argument ("--dry-run", action = "store_true")
    a = sub.add_parser ("add"); a.add_argument ("--category", required = True); a.add_argument ("--subgroup")
    a.add_argument ("--title", required = True); a.add_argument ("--body", required = True); a.add_argument ("--dry-run", action = "store_true")
    a = sub.add_parser ("set-tag"); a.add_argument ("anchor"); a.add_argument ("status", choices = list (TAGS)); a.add_argument ("--dry-run", action = "store_true")
    sub.add_parser ("check")
    args = parser.parse_args()

    global REPO
    REPO = os.path.abspath (args.dir) if args.dir else REPO
    pages = load_pages()
    { "index": cmd_index, "show": cmd_show, "search": cmd_search, "link-plan": cmd_link_plan,
      "add": cmd_add, "set-tag": cmd_set_tag, "check": cmd_check }[args.command] (pages, args)


if __name__ == "__main__":
    main()
