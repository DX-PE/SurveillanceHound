#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Check documentation navigation, source imports and built local links."""

import argparse
from collections import Counter
from html.parser import HTMLParser
import os
from pathlib import Path
import re
import tomllib
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]


def nav_paths(value):
    if isinstance(value, str):
        yield value
    elif isinstance(value, list):
        for item in value:
            yield from nav_paths(item)
    elif isinstance(value, dict):
        for item in value.values():
            yield from nav_paths(item)


def markdown_files(root):
    for directory, names, files in os.walk(root):
        names[:] = [
            name
            for name in names
            if not name.startswith((".", "build"))
            and name
            not in {"site", "dist", "work", "managed_components", "node_modules"}
        ]
        for name in files:
            if name.endswith(".md"):
                yield Path(directory, name).relative_to(root).as_posix()


def check_sources(root):
    config = tomllib.loads((root / "zensical.toml").read_text())["project"]
    docs = root / config["docs_dir"]
    listed = list(nav_paths(config["nav"]))
    pages = {p.relative_to(docs).as_posix() for p in docs.rglob("*.md")}
    errors = [f"Page is not in navigation: {p}" for p in sorted(pages - set(listed))]
    errors += [
        f"Navigation target is missing: {p}" for p in sorted(set(listed) - pages)
    ]
    errors += [
        f"Duplicate navigation target: {p}" for p, n in Counter(listed).items() if n > 1
    ]
    imports = set()
    for page in docs.rglob("*.md"):
        for name in re.findall(r'^--8<-- "([^"]+)"$', page.read_text(), re.MULTILINE):
            target = (root / name).resolve()
            if not target.is_relative_to(root) or not target.is_file():
                errors.append(f"Invalid documentation import: {name}")
            else:
                imports.add(name)
    # These root entry points lead readers into the canonical site guides.
    entry_points = {
        "README.md": "docs/index.md",
        "CONTRIBUTING.md": "docs/CONTRIBUTING.md",
    }
    for source, destination in entry_points.items():
        if not (root / source).is_file() or not (root / destination).is_file():
            errors.append(
                f"Missing documentation entry point: {source} -> {destination}"
            )
    covered = {f"{config['docs_dir']}/{p}" for p in pages} | imports | set(entry_points)
    errors += [
        f"Markdown is not covered by the site: {p}"
        for p in sorted(set(markdown_files(root)) - covered)
    ]
    return errors, len(pages), len(imports)


class Page(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.ids = set()
        self.links = []

    def handle_starttag(self, tag, attributes):
        attrs = dict(attributes)
        if attrs.get("id"):
            self.ids.add(attrs["id"])
        if tag == "a" and attrs.get("name"):
            self.ids.add(attrs["name"])
        for attribute in ("href", "src"):
            if attrs.get(attribute):
                self.links.append(attrs[attribute])


def check_site(site):
    site = site.resolve()
    pages = {}
    for path in site.rglob("*.html"):
        parsed = Page()
        parsed.feed(path.read_text())
        pages[path] = parsed
    errors = []
    if site / "index.html" not in pages:
        return ["Missing built site/index.html"], 0
    for source, page in pages.items():
        for link in page.links:
            url = urlsplit(link)
            if url.scheme or url.netloc:
                continue
            name = unquote(url.path)
            target = (
                (
                    site / name.lstrip("/")
                    if name.startswith("/")
                    else source.parent / name
                )
                if name
                else source
            )
            target = target.resolve()
            if not target.is_relative_to(site):
                errors.append(f"{source.relative_to(site)}: link escapes site: {link}")
                continue
            if target.is_dir():
                target /= "index.html"
            if not target.is_file():
                errors.append(f"{source.relative_to(site)}: missing target: {link}")
            elif (
                url.fragment
                and target in pages
                and unquote(url.fragment) not in pages[target].ids
            ):
                errors.append(f"{source.relative_to(site)}: missing anchor: {link}")
    return sorted(set(errors)), len(pages)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--site", type=Path, help="Also validate a generated site directory"
    )
    args = parser.parse_args()
    errors, page_count, import_count = check_sources(ROOT)
    if args.site:
        site_errors, built_count = check_site(args.site)
        errors += site_errors
        print(f"Checked local links, assets and anchors in {built_count} built pages.")
    if errors:
        raise SystemExit("\n".join(errors))
    print(
        f"Documentation covers {page_count} pages and {import_count} original source imports."
    )


if __name__ == "__main__":
    main()
