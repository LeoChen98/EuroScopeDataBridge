/* EuroScope Data Bridge — static site interactions */

(function () {
  'use strict';

  /* --- Code block copy buttons ------------------------------------------ */
  document.querySelectorAll('.code-block').forEach(function (block) {
    var btn = block.querySelector('.code-copy');
    if (!btn) return;
    btn.addEventListener('click', function () {
      var code = block.querySelector('pre code');
      var text = code ? code.textContent : '';
      var done = function () {
        var original = btn.textContent;
        btn.textContent = 'COPIED';
        btn.classList.add('copied');
        setTimeout(function () {
          btn.textContent = original;
          btn.classList.remove('copied');
        }, 1400);
      };
      if (navigator.clipboard && navigator.clipboard.writeText) {
        navigator.clipboard.writeText(text).then(done, function () { fallbackCopy(text); done(); });
      } else {
        fallbackCopy(text);
        done();
      }
    });
  });

  function fallbackCopy(text) {
    var ta = document.createElement('textarea');
    ta.value = text;
    ta.style.position = 'fixed';
    ta.style.opacity = '0';
    document.body.appendChild(ta);
    ta.select();
    try { document.execCommand('copy'); } catch (e) { /* ignore */ }
    document.body.removeChild(ta);
  }

  /* --- Mobile TOC drawer ------------------------------------------------ */
  var toc = document.getElementById('toc');
  var toggle = document.getElementById('tocToggle');
  if (toc && toggle) {
    toggle.addEventListener('click', function () {
      toc.classList.toggle('open');
      toggle.textContent = toc.classList.contains('open') ? '✕' : '☰';
    });
    toc.querySelectorAll('.toc-link').forEach(function (link) {
      link.addEventListener('click', function () {
        if (toc.classList.contains('open')) {
          toc.classList.remove('open');
          toggle.textContent = '☰';
        }
      });
    });
  }

  /* --- Scrollspy --------------------------------------------------------- */
  var links = Array.prototype.slice.call(document.querySelectorAll('.toc-link'));
  var heads = links
    .map(function (l) { return document.querySelector(l.getAttribute('href')); })
    .filter(Boolean);

  if (links.length && heads.length && 'IntersectionObserver' in window) {
    var activeId = null;
    var observer = new IntersectionObserver(function (entries) {
      entries.forEach(function (entry) {
        if (entry.isIntersecting) activeId = entry.target.id;
      });
      // Pick the topmost visible heading.
      var current = null;
      for (var i = 0; i < heads.length; i++) {
        var rect = heads[i].getBoundingClientRect();
        if (rect.top <= 120) current = heads[i].id;
      }
      var target = current || activeId;
      links.forEach(function (l) {
        l.classList.toggle('active', l.getAttribute('href') === '#' + target);
      });
    }, { rootMargin: '-10% 0px -70% 0px', threshold: 0 });
    heads.forEach(function (h) { observer.observe(h); });
  }

  /* --- Back to top -------------------------------------------------------- */
  var topBtn = document.getElementById('toTop');
  if (topBtn) {
    window.addEventListener('scroll', function () {
      topBtn.classList.toggle('show', window.scrollY > 480);
    }, { passive: true });
    topBtn.addEventListener('click', function () {
      window.scrollTo({ top: 0, behavior: 'smooth' });
    });
  }

  /* --- Footer year -------------------------------------------------------- */
  document.querySelectorAll('[data-year]').forEach(function (el) {
    el.textContent = String(new Date().getFullYear());
  });

  /* --- Latest release version (GitHub API) -------------------------------- */
  var verEls = Array.prototype.slice.call(document.querySelectorAll('[data-version]'));
  if (verEls.length && window.fetch) {
    fetch('https://api.github.com/repos/LeoChen98/EuroscopeDataBridge/releases/latest', {
      headers: { Accept: 'application/vnd.github+json' }
    })
      .then(function (r) { if (!r.ok) throw new Error('http ' + r.status); return r.json(); })
      .then(function (d) {
        var tag = String(d.tag_name || '').trim();
        if (!tag) return;
        // The tag may look like "release/1.1.1" or "v1.1.1" — extract the
        // semver part (MAJOR.MINOR.PATCH, optional -prerelease / +build).
        var m = tag.match(
          /(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?/
        );
        if (!m) return;
        verEls.forEach(function (el) {
          el.textContent = m[0];
          el.title = 'Latest GitHub release: ' + tag;
        });
      })
      .catch(function () { /* offline or rate-limited: keep the "latest" fallback */ });
  }
})();
