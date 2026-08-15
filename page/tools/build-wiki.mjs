#!/usr/bin/env node
// ============================================================================
// EuroScope Data Bridge — static wiki page generator
//
// Converts docs/wiki.md and docs/wiki_CN.md into the static HTML pages
// page/en/wiki.html and page/zh/wiki.html. Pure Node.js, no dependencies.
//
// Usage:  node page/tools/build-wiki.mjs
// ============================================================================

import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = dirname(fileURLToPath(import.meta.url));
const ROOT = resolve(HERE, '..', '..');
const PAGE = join(ROOT, 'page');

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

function escapeHtml(s) {
  return s
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

// GitHub-style heading slug: lowercase, keep letters/numbers/space/_/-,
// drop everything else, each space becomes a dash.
function slugify(text) {
  return text
    .toLowerCase()
    .trim()
    .replace(/[^\p{L}\p{N}\s_-]/gu, '')
    .replace(/\s/g, '-');
}

function isBlank(line) { return /^\s*$/.test(line); }
function isHr(line) { return /^\s*(---|\*\*\*)\s*$/.test(line); }
function isFence(line) { return /^```/.test(line); }
function isHeading(line) { return /^#{1,4}\s+\S/.test(line); }
function isBlockquote(line) { return /^>\s?/.test(line); }
function isList(line) { return /^\s*-\s+/.test(line); }
function isTableStart(lines, i) {
  return /^\s*\|/.test(lines[i]) && i + 1 < lines.length && isTableSeparator(lines[i + 1]);
}
function isTableSeparator(line) {
  return /^\s*\|?[\s:|-]+\|?\s*$/.test(line) && line.includes('-');
}

// Split a table row into cells, ignoring "|" inside inline-code spans.
function splitRow(line) {
  let s = line.trim();
  if (s.startsWith('|')) s = s.slice(1);
  if (s.endsWith('|')) s = s.slice(0, -1);
  const cells = [];
  let cur = '';
  let inCode = false;
  for (const ch of s) {
    if (ch === '`') { inCode = !inCode; cur += ch; }
    else if (ch === '|' && !inCode) { cells.push(cur.trim()); cur = ''; }
    else cur += ch;
  }
  cells.push(cur.trim());
  return cells;
}

// Inline markdown: code spans, bold, links (HTML already escaped).
function inline(s) {
  let r = escapeHtml(s);
  r = r.replace(/`([^`]+)`/g, '<code>$1</code>');
  r = r.replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>');
  r = r.replace(/\[([^\]]+)\]\(([^)]+)\)/g, (m, text, href) =>
    `<a href="${href}">${text}</a>`);
  return r;
}

// ---------------------------------------------------------------------------
// Block-level converter
// ---------------------------------------------------------------------------

function convert(md) {
  const lines = md.split(/\r?\n/);
  const out = [];
  const headings = [];          // { level, text, id } for the sidebar TOC
  const anchors = new Set();    // all generated heading ids
  const used = new Map();       // slug -> occurrence count (GitHub -1 dedup)

  const anchorFor = (text) => {
    let slug = slugify(text);
    if (used.has(slug)) {
      const n = used.get(slug) + 1;
      used.set(slug, n);
      slug = `${slug}-${n}`;
    } else {
      used.set(slug, 1);
    }
    return slug;
  };

  let i = 0;
  while (i < lines.length) {
    const line = lines[i];

    // Skip the markdown language-switch line (the site has its own toggle).
    if (/^\*\*(语言|Language)[:：]/.test(line.trim())) { i++; continue; }

    if (isBlank(line)) { i++; continue; }

    // Skip the inline table of contents (the site renders a sidebar TOC).
    if (isHeading(line) && /^\s*#{2}\s+(目录|Table of contents)\s*$/i.test(line)) {
      i++;
      while (i < lines.length && !isHr(lines[i])) i++;
      i++; // consume the "---"
      continue;
    }

    // Fenced code block
    if (isFence(line)) {
      const lang = line.replace(/^```/, '').trim() || 'text';
      const buf = [];
      i++;
      while (i < lines.length && !isFence(lines[i])) { buf.push(lines[i]); i++; }
      i++; // closing fence
      out.push(renderCodeBlock(lang, buf.join('\n')));
      continue;
    }

    // Heading
    if (isHeading(line)) {
      const m = line.match(/^(#{1,4})\s+(.*)$/);
      const level = m[1].length;
      const text = m[2].trim();
      const id = anchorFor(text);
      anchors.add(id);
      headings.push({ level, text: text.replace(/`/g, ''), id });
      out.push(`<h${level} id="${id}">${inline(text)}</h${level}>`);
      i++;
      continue;
    }

    // Horizontal rule
    if (isHr(line)) { out.push('<hr>'); i++; continue; }

    // Table
    if (isTableStart(lines, i)) {
      const headerCells = splitRow(lines[i]);
      const sepCells = splitRow(lines[i + 1]);
      const aligns = sepCells.map((c) => {
        const t = c.trim();
        if (t.startsWith(':') && t.endsWith(':')) return 'center';
        if (t.endsWith(':')) return 'right';
        return 'left';
      });
      i += 2;
      const rows = [];
      while (i < lines.length && /^\s*\|/.test(lines[i])) {
        rows.push(splitRow(lines[i]));
        i++;
      }
      out.push('<div class="table-wrap"><table><thead><tr>'
        + headerCells.map((c, idx) =>
            `<th${aligns[idx] !== 'left' ? ` style="text-align:${aligns[idx]}"` : ''}>${inline(c)}</th>`)
            .join('')
        + '</tr></thead><tbody>'
        + rows.map((r) => '<tr>'
            + r.map((c, idx) =>
                `<td${aligns[idx] !== 'left' ? ` style="text-align:${aligns[idx]}"` : ''}>${inline(c)}</td>`)
                .join('')
            + '</tr>').join('')
        + '</tbody></table></div>');
      continue;
    }

    // Blockquote
    if (isBlockquote(line)) {
      const buf = [];
      while (i < lines.length && isBlockquote(lines[i])) {
        buf.push(lines[i].replace(/^>\s?/, ''));
        i++;
      }
      out.push(`<blockquote>${inline(buf.join(' '))}</blockquote>`);
      continue;
    }

    // Unordered list
    if (isList(line)) {
      const items = [];
      while (i < lines.length && isList(lines[i])) {
        items.push(lines[i].replace(/^\s*-\s+/, ''));
        i++;
      }
      out.push(`<ul>${items.map((it) => `<li>${inline(it)}</li>`).join('')}</ul>`);
      continue;
    }

    // Paragraph: gather lines until the next block boundary.
    const buf = [line];
    i++;
    while (i < lines.length
      && !isBlank(lines[i])
      && !isFence(lines[i])
      && !isHeading(lines[i])
      && !isHr(lines[i])
      && !isBlockquote(lines[i])
      && !isList(lines[i])
      && !isTableStart(lines, i)) {
      buf.push(lines[i]);
      i++;
    }
    out.push(`<p>${inline(buf.join(' '))}</p>`);
  }

  return { body: out.join('\n'), headings, anchors };
}

function renderCodeBlock(lang, code) {
  const labels = { json: 'JSON', jsonc: 'JSON', javascript: 'JavaScript', js: 'JavaScript', bash: 'Bash' };
  const label = labels[lang] || lang.toUpperCase();
  return [
    '<div class="code-block">',
    '  <div class="code-head">',
    `    <span class="code-lang">${escapeHtml(label)}</span>`,
    '    <button class="code-copy" type="button" aria-label="Copy">Copy</button>',
    '  </div>',
    `  <pre><code class="lang-${escapeHtml(lang)}">${escapeHtml(code)}</code></pre>`,
    '</div>',
  ].join('\n');
}

// ---------------------------------------------------------------------------
// Page template
// ---------------------------------------------------------------------------

const BRAND_SVG = [
  '<svg viewBox="0 0 24 24" width="18" height="18" aria-hidden="true">',
  '  <circle cx="12" cy="12" r="10" fill="none" stroke="currentColor" stroke-width="1.4" opacity="0.35"/>',
  '  <circle cx="12" cy="12" r="5.5" fill="none" stroke="currentColor" stroke-width="1.4" opacity="0.7"/>',
  '  <path d="M12 12 L18.6 5.4" stroke="currentColor" stroke-width="1.4"/>',
  '  <circle cx="12" cy="12" r="1.7" fill="currentColor"/>',
  '</svg>',
].join('\n');

function buildPage(opts) {
  const { lang, title, nav, tocHtml, bodyHtml } = opts;
  const { homeLabel, docsLabel, langLabel, langHref, footNotes } = nav;

  const tocSection = tocHtml
    ? [
        '<aside class="toc" id="toc">',
        '  <div class="toc-head">ON THIS PAGE</div>',
        '  <nav class="toc-nav">',
        tocHtml,
        '  </nav>',
        '</aside>',
      ].join('\n')
    : '';

  return `<!doctype html>
<html lang="${lang}">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>${escapeHtml(title)}</title>
<meta name="description" content="EuroScope Data Bridge — local WebSocket API for live EuroScope flight data. Documentation and quick start.">
<link rel="stylesheet" href="../assets/style.css">
</head>
<body class="wiki-body">
<header class="nav">
  <div class="nav-inner">
    <a class="brand" href="index.html">
      ${BRAND_SVG}
      <span class="brand-name">EuroScope <em>Data Bridge</em></span>
    </a>
    <nav class="nav-links">
      <a href="index.html">${homeLabel}</a>
      <a href="wiki.html" class="active">${docsLabel}</a>
      <a class="nav-lang" href="${langHref}">${langLabel}</a>
    </nav>
    <button class="toc-toggle" id="tocToggle" type="button" aria-label="Toggle contents">☰</button>
  </div>
</header>
${tocSection}
<main class="main">
  <article class="doc">
${bodyHtml}
  </article>
</main>
<footer class="foot">
  <div class="foot-inner">
    <p>EuroScope Data Bridge · MIT License © 2026 Leo Chen</p>
    <p class="foot-links">
      <a href="https://github.com/LeoChen98/EuroscopeDataBridge" target="_blank" rel="noopener">GitHub</a>
      <span>·</span>
      ${footNotes}
    </p>
  </div>
</footer>
<script src="../assets/script.js"></script>
</body>
</html>
`;
}

function buildToc(headings) {
  return headings
    .filter((h) => h.level >= 2 && h.level <= 4)
    .map((h) =>
      `    <a class="toc-link toc-l${h.level}" href="#${h.id}">${escapeHtml(h.text)}</a>`)
    .join('\n');
}

// ---------------------------------------------------------------------------
// Link validation
// ---------------------------------------------------------------------------

function checkLinks(bodyHtml, anchors, pageName) {
  const missing = [];
  for (const m of bodyHtml.matchAll(/href="#([^"]+)"/g)) {
    if (!anchors.has(m[1])) missing.push(m[1]);
  }
  if (missing.length) {
    console.warn(`[warn] ${pageName}: unresolved anchors: ${missing.join(', ')}`);
  }
  return missing.length === 0;
}

// ---------------------------------------------------------------------------
// Build both wiki pages
// ---------------------------------------------------------------------------

const jobs = [
  {
    src: join(ROOT, 'docs', 'wiki_CN.md'),
    out: join(PAGE, 'zh', 'wiki.html'),
    lang: 'zh-CN',
    nav: {
      homeLabel: '首页',
      docsLabel: '文档',
      langLabel: 'EN',
      langHref: '../en/wiki.html',
      footNotes: '<a href="../zh/wiki.html">中文</a><span>·</span><a href="../en/wiki.html">English</a>',
    },
  },
  {
    src: join(ROOT, 'docs', 'wiki.md'),
    out: join(PAGE, 'en', 'wiki.html'),
    lang: 'en',
    nav: {
      homeLabel: 'Home',
      docsLabel: 'Docs',
      langLabel: '中文',
      langHref: '../zh/wiki.html',
      footNotes: '<a href="../en/wiki.html">English</a><span>·</span><a href="../zh/wiki.html">中文</a>',
    },
  },
];

for (const job of jobs) {
  mkdirSync(dirname(job.out), { recursive: true });
  const md = readFileSync(job.src, 'utf8');
  const { body, headings, anchors } = convert(md);
  const tocHtml = buildToc(headings);
  const title = headings.find((h) => h.level === 1)?.text || 'EuroScope Data Bridge';
  const html = buildPage({ ...job, title, tocHtml, bodyHtml: body });
  writeFileSync(job.out, html, 'utf8');
  const ok = checkLinks(body, anchors, job.src);
  const counts = {};
  for (const h of headings) counts[`h${h.level}`] = (counts[`h${h.level}`] || 0) + 1;
  console.log(`[built] ${job.out}`);
  console.log(`        headings: ${JSON.stringify(counts)}, links ok: ${ok}`);
}
