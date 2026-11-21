#!/usr/bin/env python3
# minimal markdown to html for the docs/ pages published to github pages
# (copied from signalmesh_CAD/tools/mddoc.py, plus <sub> and > blockquotes)
#
# supports what the board docs use: headings, paragraphs, lists, tables, fenced
# code (mermaid blocks become diagrams), images, inline code, bold and links. links to
# .md files are rewritten to .html so docs cross-link on github pages too
#
# usage:
#   tools/mddoc.py <docs_dir> <out_dir> [title]

import html
import os
import re
import shutil
import sys


def inline(s):
    s = html.escape(s, quote=False)
    s = s.replace('&lt;sub&gt;', '<sub>').replace('&lt;/sub&gt;', '</sub>')
    s = re.sub(r'`([^`]+)`', r'<code>\1</code>', s)
    s = re.sub(r'\*\*([^*]+)\*\*', r'<strong>\1</strong>', s)
    # images before links so ![x](y) is not read as a link
    s = re.sub(r'!\[([^\]]*)\]\(([^)]+)\)', r'<img src="\2" alt="\1">', s)

    def link(m):
        href = m.group(2)
        if not re.match(r'[a-z]+://', href):
            href = re.sub(r'\.md(#|$)', r'.html\1', href)
        return '<a href="%s">%s</a>' % (href, m.group(1))
    return re.sub(r'\[([^\]]+)\]\(([^)]+)\)', link, s)


def cells(row):
    return [c.strip() for c in row.strip().strip('|').split('|')]


def md_to_html(text):
    out = []
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        ln = lines[i]
        if ln.startswith('```'):
            lang = ln[3:].strip()
            body = []
            i += 1
            while i < len(lines) and not lines[i].startswith('```'):
                body.append(lines[i])
                i += 1
            i += 1
            code = '\n'.join(body)
            if lang == 'mermaid':
                # mermaid reads the raw text, keep html-sensitive chars escaped
                out.append('<pre class="mermaid">%s</pre>' % html.escape(code, quote=False))
            else:
                out.append('<pre><code>%s</code></pre>' % html.escape(code, quote=False))
            continue
        m = re.match(r'(#{1,4})\s+(.*)', ln)
        if m:
            n = len(m.group(1))
            # github-style ids so page#heading links work on pages too
            slug = re.sub(r'[^\w\- ]', '', m.group(2).lower()).strip().replace(' ', '-')
            out.append('<h%d id="%s">%s</h%d>' % (n, slug, inline(m.group(2)), n))
            i += 1
            continue
        if ln.startswith('|') and i + 1 < len(lines) and re.match(r'\|?\s*:?-+', lines[i + 1]):
            head = cells(ln)
            i += 2
            rows = []
            while i < len(lines) and lines[i].startswith('|'):
                rows.append(cells(lines[i]))
                i += 1
            t = '<table><thead><tr>' + ''.join('<th>%s</th>' % inline(c) for c in head) + '</tr></thead><tbody>'
            for r in rows:
                t += '<tr>' + ''.join('<td>%s</td>' % inline(c) for c in r) + '</tr>'
            out.append(t + '</tbody></table>')
            continue
        if re.match(r'\s*(-|\d+\.)\s+', ln):
            ordered = bool(re.match(r'\s*\d+\.', ln))
            items = []
            while i < len(lines) and (re.match(r'\s*(-|\d+\.)\s+', lines[i]) or
                                      (lines[i].startswith('   ') and items)):
                if re.match(r'\s*(-|\d+\.)\s+', lines[i]):
                    items.append(re.sub(r'\s*(-|\d+\.)\s+', '', lines[i], count=1))
                else:
                    items[-1] += ' ' + lines[i].strip()
                i += 1
            tag = 'ol' if ordered else 'ul'
            out.append('<%s>%s</%s>' % (tag, ''.join('<li>%s</li>' % inline(x) for x in items), tag))
            continue
        if ln.startswith('>'):
            quote = []
            while i < len(lines) and lines[i].startswith('>'):
                quote.append(lines[i][1:].strip())
                i += 1
            out.append('<blockquote>%s</blockquote>' % inline(' '.join(quote)))
            continue
        if not ln.strip():
            i += 1
            continue
        para = [ln]
        i += 1
        while i < len(lines) and lines[i].strip() and not re.match(r'(#|\||```|>|\s*-\s|\s*\d+\.\s)', lines[i]):
            para.append(lines[i])
            i += 1
        out.append('<p>%s</p>' % inline(' '.join(p.strip() for p in para)))
    return '\n'.join(out)


PAGE = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{{title}}</title>
<style>
  :root { color-scheme: light dark; }
  body { margin: 0; padding: 2rem 1.25rem 4rem; font: 16px/1.55 system-ui, sans-serif;
         background: #0d1117; color: #e6edf3; }
  @media (prefers-color-scheme: light) { body { background: #f6f8fa; color: #1f2328; } }
  main { max-width: 1000px; margin: 0 auto; }
  nav { max-width: 1000px; margin: 0 auto 1.5rem; font-size: .9rem; }
  a { color: #4493f8; text-decoration: none; }
  a:hover { text-decoration: underline; }
  h1 { font-size: 1.6rem; } h2 { font-size: 1.25rem; margin-top: 2rem; } h3 { font-size: 1.05rem; }
  table { border-collapse: collapse; margin: 1rem 0; font-size: .9rem; display: block; overflow-x: auto; }
  th, td { border: 1px solid #30363d; padding: .3rem .6rem; text-align: left; vertical-align: top; }
  @media (prefers-color-scheme: light) { th, td { border-color: #d0d7de; } }
  code { font-size: .85em; background: #8882; padding: .1em .3em; border-radius: 4px; }
  pre { overflow-x: auto; }
  pre.mermaid { background: transparent; text-align: center; }
  blockquote { margin: 1rem 0; padding: .2rem 1rem; border-left: 3px solid #8888; opacity: .9; }
  img { max-width: 100%; height: auto; background: #fff; border-radius: 6px; padding: 6px; }
</style>
</head>
<body>
<nav><a href="index.html">{{board}} docs</a> / <a href="https://github.com/akielaries/signalmesh">source</a> / <a href="https://akielaries.github.io/signalmesh_CAD/">hardware (CAD)</a></nav>
<main>
{{body}}
</main>
<script type="module">
  import mermaid from 'https://cdn.jsdelivr.net/npm/mermaid@10.9.1/dist/mermaid.esm.min.mjs';
  const dark = window.matchMedia('(prefers-color-scheme: dark)').matches;
  mermaid.initialize({ startOnLoad: true, theme: dark ? 'dark' : 'default' });
</script>
</body>
</html>
"""


def build_docs(docs_dir, out_dir, board):
    os.makedirs(out_dir, exist_ok=True)
    pages = 0
    for f in sorted(os.listdir(docs_dir)):
        src = os.path.join(docs_dir, f)
        if not f.endswith('.md'):
            # diagrams (drawio sources, exported svg) and other assets ship as-is
            if os.path.isdir(src):
                shutil.copytree(src, os.path.join(out_dir, f), dirs_exist_ok=True)
            else:
                shutil.copy2(src, os.path.join(out_dir, f))
            continue
        text = open(os.path.join(docs_dir, f), encoding='utf-8').read()
        m = re.search(r'^#\s+(.*)$', text, re.M)
        title = '%s - %s' % (board, m.group(1) if m else f[:-3])
        page = (PAGE.replace('{{title}}', html.escape(title)).replace('{{board}}', html.escape(board))
                .replace('{{body}}', md_to_html(text)))
        with open(os.path.join(out_dir, f[:-3] + '.html'), 'w', encoding='utf-8') as fh:
            fh.write(page)
        pages += 1
    return pages


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('usage: tools/mddoc.py <docs_dir> <out_dir> [title]', file=sys.stderr)
        sys.exit(2)
    n = build_docs(sys.argv[1], sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else 'board')
    print('mddoc: %d page(s) -> %s' % (n, sys.argv[2]))
