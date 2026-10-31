/* Progressive enhancement: all content and links exist in the HTML. */
(() => {
  document.documentElement.classList.add('js');
  const menu = document.querySelector('.menu-toggle');
  const nav = document.getElementById('main-nav');
  menu.hidden = false;
  function closeMenu() {
    nav.classList.remove('is-open');
    menu.setAttribute('aria-expanded', 'false');
  }
  menu.addEventListener('click', () => {
    const open = menu.getAttribute('aria-expanded') !== 'true';
    menu.setAttribute('aria-expanded', String(open));
    nav.classList.toggle('is-open', open);
  });
  document.addEventListener('keydown', event => {
    if (event.key === 'Escape' && menu.getAttribute('aria-expanded') === 'true') {
      closeMenu(); menu.focus();
    }
  });
  document.addEventListener('click', event => {
    if (!event.target.closest('.header')) closeMenu();
  });
  nav.addEventListener('click', event => { if (event.target.closest('a')) closeMenu(); });

  const themeButton = document.querySelector('.theme-toggle');
  const systemTheme = matchMedia('(prefers-color-scheme: dark)');
  let preference = null;
  try { preference = localStorage.getItem('dawit-next-theme'); } catch (_) { /* A blocked store still allows this visit's toggle. */ }
  if (preference === 'light' || preference === 'dark') document.documentElement.dataset.theme = preference;
  function updateThemeLabel() {
    const isDark = document.documentElement.dataset.theme ? document.documentElement.dataset.theme === 'dark' : systemTheme.matches;
    themeButton.setAttribute('aria-label', `Switch to ${isDark ? 'light' : 'dark'} theme`);
    themeButton.title = `Switch to ${isDark ? 'light' : 'dark'} theme`;
  }
  themeButton.hidden = false;
  updateThemeLabel();
  systemTheme.addEventListener('change', updateThemeLabel);
  themeButton.addEventListener('click', () => {
    const isDark = document.documentElement.dataset.theme ? document.documentElement.dataset.theme === 'dark' : systemTheme.matches;
    const theme = isDark ? 'light' : 'dark';
    document.documentElement.dataset.theme = theme;
    try { localStorage.setItem('dawit-next-theme', theme); } catch (_) { /* Keep theme working without persistence. */ }
    updateThemeLabel();
  });

  const controllers = [];
  document.querySelectorAll('[data-catalog]').forEach(tools => {
    const guides = tools.dataset.catalog === 'guides';
    const scope = guides ? document.getElementById('library') : document.querySelector('main');
    const items = [...scope.querySelectorAll('[data-item]')];
    const groups = [...scope.querySelectorAll('[data-group]')];
    const input = tools.querySelector('input');
    const buttons = [...tools.querySelectorAll('[data-filter]')];
    const status = tools.querySelector('.results-count');
    const empty = scope.querySelector('.empty-state');
    const searchable = new Map(items.map(item => [item, item.textContent.toLocaleLowerCase('en-GB')]));
    let selected = 'all';
    function update() {
      const words = input.value.trim().toLocaleLowerCase('en-GB').split(/\s+/).filter(Boolean);
      let visible = 0;
      items.forEach(item => {
        const match = (selected === 'all' || item.dataset.groupKey === selected) && words.every(word => searchable.get(item).includes(word));
        item.hidden = !match;
        if (match) ++visible;
      });
      groups.forEach(group => { group.hidden = ![...group.querySelectorAll('[data-item]')].some(item => !item.hidden); });
      buttons.forEach(button => button.setAttribute('aria-pressed', String(button.dataset.filter === selected)));
      status.textContent = `${visible} ${guides ? (visible === 1 ? 'topic' : 'topics') : (visible === 1 ? 'feature' : 'features')}${selected === 'all' ? '' : ' in this area'}${words.length ? ' matching your search' : ''}`;
      if (empty) empty.hidden = visible > 0;
    }
    input.addEventListener('input', update);
    buttons.forEach(button => button.addEventListener('click', () => { selected = button.dataset.filter; update(); }));
    controllers.push({ scope, reveal() { selected = 'all'; input.value = ''; update(); } });
    update();
  });
  function openHashTarget() {
    if (!location.hash) return;
    let id;
    try { id = decodeURIComponent(location.hash.slice(1)); } catch (_) { return; }
    const target = document.getElementById(id);
    if (!target) return;
    controllers.forEach(controller => { if (controller.scope.contains(target)) controller.reveal(); });
    const details = target.closest('details');
    if (details) details.open = true;
    requestAnimationFrame(() => target.scrollIntoView({ block: 'start', behavior: 'instant' }));
  }
  window.addEventListener('hashchange', openHashTarget);
  openHashTarget();

  document.querySelectorAll('.copy-command').forEach(button => {
    if (!navigator.clipboard) return;
    button.hidden = false;
    button.addEventListener('click', async () => {
      const status = button.closest('.code-panel').querySelector('.copy-status');
      try {
        await navigator.clipboard.writeText(document.getElementById(button.dataset.copy).textContent);
        status.textContent = 'Build commands copied.';
        button.textContent = 'Copied';
      } catch (_) { status.textContent = 'Select the commands above to copy them.'; }
    });
  });
})();
