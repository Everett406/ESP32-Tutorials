// ESP32 学习教程站点主脚本
(function() {
  'use strict';

  // DOM 元素
  const contentWrapper = document.getElementById('contentWrapper');
  const markdownBody = document.getElementById('markdownBody');
  const navTree = document.getElementById('navTree');
  const sidebar = document.getElementById('sidebar');
  const menuToggle = document.getElementById('menuToggle');
  const themeToggle = document.getElementById('themeToggle');
  const annoToggle = document.getElementById('annoToggle');
  const annotationPanel = document.getElementById('annotationPanel');
  const annotationList = document.getElementById('annotationList');
  const annoPopup = document.getElementById('annoPopup');
  const annoNote = document.getElementById('annoNote');
  const saveAnnoBtn = document.getElementById('saveAnno');
  const cancelAnnoBtn = document.getElementById('cancelAnno');
  const exportAnnoBtn = document.getElementById('exportAnno');
  const importAnnoBtn = document.getElementById('importAnno');
  const clearAnnoBtn = document.getElementById('clearAnno');
  const heroStats = document.getElementById('heroStats');
  const contentArea = document.getElementById('contentArea');

  // 状态
  let contentIndex = null;
  let currentPath = '';
  let pendingSelection = null;
  let selectedColor = 'yellow';

  const ANNO_STORAGE_KEY = 'esp32-tutorials-annotations';

  // ==================== 初始化 ====================

  async function init() {
    loadTheme();
    loadAnnotations();
    setupEventListeners();
    setupMarked();

    try {
      const response = await fetch('content/index.json');
      contentIndex = await response.json();
      buildNavigation();
      updateHeroStats();
      handleRoute();
    } catch (error) {
      console.error('加载内容索引失败:', error);
      markdownBody.innerHTML = '<p style="color:red">加载内容失败，请刷新页面重试。</p>';
      markdownBody.classList.remove('hidden');
      contentWrapper.classList.add('hidden');
    }
  }

  // ==================== Marked 配置 ====================

  function setupMarked() {
    marked.setOptions({
      breaks: true,
      gfm: true,
      headerIds: true,
      highlight: function(code, lang) {
        if (lang && hljs.getLanguage(lang)) {
          return hljs.highlight(code, { language: lang }).value;
        }
        return hljs.highlightAuto(code).value;
      }
    });
  }

  // ==================== 导航 ====================

  function buildNavigation() {
    if (!contentIndex) return;

    const sections = [
      { key: 'arduino', title: 'Arduino 入门教程', icon: '📚' },
      { key: 'c', title: 'C 语言教程', icon: '🔤' },
      { key: 'projects', title: '综合项目', icon: '🛠️' }
    ];

    navTree.innerHTML = '';

    sections.forEach(section => {
      const items = contentIndex[section.key] || [];
      if (items.length === 0) return;

      const sectionEl = document.createElement('div');
      sectionEl.className = 'nav-section';

      const secTitle = document.createElement('div');
      secTitle.className = 'nav-section-title';
      secTitle.textContent = `${section.icon} ${section.title}`;
      sectionEl.appendChild(secTitle);

      // 按 subtitle 分组
      const groups = {};
      const groupOrder = [];
      items.forEach(item => {
        const subtitle = item.subtitle || '其他';
        if (!groups[subtitle]) {
          groups[subtitle] = [];
          groupOrder.push(subtitle);
        }
        groups[subtitle].push(item);
      });

      // 渲染分组
      groupOrder.forEach((subtitle, idx) => {
        const groupItems = groups[subtitle];

        const groupHeader = document.createElement('div');
        groupHeader.className = 'nav-group-header';
        groupHeader.innerHTML = `
          <svg class="nav-group-arrow" width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><polyline points="9 18 15 12 9 6"/></svg>
          <span class="nav-group-label">${subtitle}</span>
          <span class="nav-group-count">${groupItems.length}</span>
        `;

        const groupBody = document.createElement('div');
        groupBody.className = 'nav-group-body';

        groupItems.forEach(item => {
          const link = document.createElement('a');
          link.className = 'nav-item';
          link.href = `#${item.path}`;
          link.dataset.path = item.path;
          link.title = item.title;

          const displayTitle = item.title.length > 22
            ? item.title.slice(0, 20) + '…'
            : item.title;
          link.textContent = displayTitle;

          link.addEventListener('click', (e) => {
            e.preventDefault();
            loadContent(item.path);
            if (window.innerWidth <= 768) {
              closeSidebar();
            }
          });

          groupBody.appendChild(link);
        });

        groupHeader.addEventListener('click', () => {
          groupHeader.classList.toggle('collapsed');
          groupBody.classList.toggle('collapsed');
        });

        // 第一个分组默认展开
        if (idx !== 0) {
          groupHeader.classList.add('collapsed');
        }

        sectionEl.appendChild(groupHeader);
        sectionEl.appendChild(groupBody);
      });

      navTree.appendChild(sectionEl);
    });
  }

  function updateActiveNav(path) {
    document.querySelectorAll('.nav-item').forEach(item => {
      item.classList.toggle('active', item.dataset.path === path);
    });
    if (path) {
      document.querySelectorAll('.nav-group-header').forEach(header => {
        header.classList.remove('collapsed');
      });
      document.querySelectorAll('.nav-group-body').forEach(body => {
        body.classList.remove('collapsed');
      });
    }
  }

  // ==================== 内容加载 ====================

  async function loadContent(path) {
    if (!path) {
      showHome();
      return;
    }

    currentPath = path;
    updateActiveNav(path);

    try {
      const response = await fetch(`content/${path}`);
      if (!response.ok) throw new Error('加载失败');
      const text = await response.text();

      const isIno = path.endsWith('.ino');
      let html;

      if (isIno) {
        html = `<h1>${getTitleFromPath(path)}</h1>\n<pre><code class="language-cpp">${escapeHtml(text)}</code></pre>`;
      } else {
        html = marked.parse(text);
      }

      markdownBody.innerHTML = html;
      contentWrapper.classList.add('hidden');
      markdownBody.classList.remove('hidden');

      wrapCodeBlocks();

      markdownBody.querySelectorAll('pre code').forEach(block => {
        hljs.highlightElement(block);
      });

      restoreAnnotations();
      window.scrollTo(0, 0);
      history.pushState(null, '', `#${path}`);
    } catch (error) {
      console.error('加载内容失败:', error);
      markdownBody.innerHTML = `<p style="color:red">无法加载: ${path}</p>`;
      markdownBody.classList.remove('hidden');
      contentWrapper.classList.add('hidden');
    }
  }

  function showHome() {
    contentWrapper.classList.remove('hidden');
    markdownBody.classList.add('hidden');
    updateActiveNav('');
    history.pushState(null, '', window.location.pathname);
  }

  function handleRoute() {
    const hash = window.location.hash.slice(1);
    if (hash) {
      loadContent(hash);
    } else {
      showHome();
    }
  }

  function getTitleFromPath(path) {
    if (!contentIndex) return path;

    const allItems = [
      ...(contentIndex.arduino || []),
      ...(contentIndex.c || []),
      ...(contentIndex.projects || [])
    ];

    const item = allItems.find(i => i.path === path);
    return item ? item.title : path;
  }

  function wrapCodeBlocks() {
    markdownBody.querySelectorAll('pre').forEach(pre => {
      const wrapper = document.createElement('div');
      wrapper.className = 'code-block';
      pre.parentNode.insertBefore(wrapper, pre);
      wrapper.appendChild(pre);

      const copyBtn = document.createElement('button');
      copyBtn.className = 'code-copy';
      copyBtn.textContent = '复制';
      copyBtn.addEventListener('click', () => {
        const code = pre.querySelector('code') || pre;
        navigator.clipboard.writeText(code.textContent).then(() => {
          copyBtn.textContent = '已复制';
          setTimeout(() => copyBtn.textContent = '复制', 1500);
        });
      });
      wrapper.appendChild(copyBtn);
    });
  }

  function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
  }

  // ==================== 主题 ====================

  function loadTheme() {
    const saved = localStorage.getItem('esp32-tutorials-theme');
    const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
    const theme = saved || (prefersDark ? 'dark' : 'light');
    document.documentElement.setAttribute('data-theme', theme);
  }

  function toggleTheme() {
    const current = document.documentElement.getAttribute('data-theme');
    const next = current === 'dark' ? 'light' : 'dark';
    document.documentElement.setAttribute('data-theme', next);
    localStorage.setItem('esp32-tutorials-theme', next);
  }

  // ==================== 标注系统 ====================

  function loadAnnotations() {}

  function getAnnotations() {
    try {
      const data = localStorage.getItem(ANNO_STORAGE_KEY);
      return data ? JSON.parse(data) : [];
    } catch (e) {
      return [];
    }
  }

  function saveAnnotations(annotations) {
    localStorage.setItem(ANNO_STORAGE_KEY, JSON.stringify(annotations));
    renderAnnotationList();
  }

  function addAnnotation(path, text, note, color, startOffset, endOffset) {
    const annotations = getAnnotations();
    annotations.push({
      id: Date.now().toString(36) + Math.random().toString(36).substr(2),
      path,
      text,
      note,
      color,
      startOffset,
      endOffset,
      createdAt: new Date().toISOString()
    });
    saveAnnotations(annotations);
  }

  function deleteAnnotation(id) {
    const annotations = getAnnotations().filter(a => a.id !== id);
    saveAnnotations(annotations);
    restoreAnnotations();
  }

  function handleTextSelection() {
    const selection = window.getSelection();
    const text = selection.toString().trim();

    if (!text || text.length < 2) {
      hideAnnoPopup();
      return;
    }

    const range = selection.getRangeAt(0);

    if (!markdownBody.contains(range.commonAncestorContainer)) {
      return;
    }

    pendingSelection = {
      text,
      range: range.cloneRange()
    };

    showAnnoPopup(selection);
  }

  function showAnnoPopup(selection) {
    const range = selection.getRangeAt(0);
    const rect = range.getBoundingClientRect();

    annoPopup.style.left = `${Math.max(10, rect.left + window.scrollX)}px`;
    annoPopup.style.top = `${rect.bottom + window.scrollY + 8}px`;
    annoPopup.classList.remove('hidden');

    annoNote.value = '';
    setSelectedColor('yellow');
  }

  function hideAnnoPopup() {
    annoPopup.classList.add('hidden');
    pendingSelection = null;
  }

  function setSelectedColor(color) {
    selectedColor = color;
    annoPopup.querySelectorAll('.color-dot').forEach(dot => {
      dot.classList.toggle('selected', dot.dataset.color === color);
    });
  }

  function savePendingAnnotation() {
    if (!pendingSelection || !currentPath) return;

    const { text, range } = pendingSelection;
    const note = annoNote.value.trim();

    const bodyRange = document.createRange();
    bodyRange.selectNodeContents(markdownBody);
    bodyRange.setEnd(range.startContainer, range.startOffset);
    const startOffset = bodyRange.toString().length;
    const endOffset = startOffset + text.length;

    try {
      const span = document.createElement('span');
      span.className = `anno-highlight ${selectedColor}`;
      span.title = note || '已标注';
      range.surroundContents(span);

      addAnnotation(currentPath, text, note, selectedColor, startOffset, endOffset);
      hideAnnoPopup();
      window.getSelection().removeAllRanges();
    } catch (e) {
      console.error('标注失败:', e);
      alert('无法标注跨越多元素的选区，请尝试选择同一段落内的文字。');
      hideAnnoPopup();
    }
  }

  function restoreAnnotations() {
    markdownBody.querySelectorAll('.anno-highlight').forEach(span => {
      const parent = span.parentNode;
      while (span.firstChild) {
        parent.insertBefore(span.firstChild, span);
      }
      parent.removeChild(span);
      parent.normalize();
    });

    if (!currentPath) return;

    const annotations = getAnnotations().filter(a => a.path === currentPath);
    if (annotations.length === 0) return;

    const walker = document.createTreeWalker(
      markdownBody,
      NodeFilter.SHOW_TEXT,
      null,
      false
    );

    const textNodes = [];
    let node;
    while (node = walker.nextNode()) {
      textNodes.push(node);
    }

    annotations.forEach(anno => {
      for (const textNode of textNodes) {
        const index = textNode.textContent.indexOf(anno.text);
        if (index !== -1) {
          const range = document.createRange();
          range.setStart(textNode, index);
          range.setEnd(textNode, index + anno.text.length);

          try {
            const span = document.createElement('span');
            span.className = `anno-highlight ${anno.color}`;
            span.title = anno.note || '已标注';
            span.dataset.annoId = anno.id;
            range.surroundContents(span);
          } catch (e) {}
          break;
        }
      }
    });
  }

  function renderAnnotationList() {
    const annotations = getAnnotations();

    if (annotations.length === 0) {
      annotationList.innerHTML = '<p class="annotation-empty">暂无标注，选中页面文字即可添加。</p>';
      return;
    }

    annotationList.innerHTML = '';

    const groups = {};
    annotations.forEach(anno => {
      if (!groups[anno.path]) groups[anno.path] = [];
      groups[anno.path].push(anno);
    });

    Object.entries(groups).forEach(([path, items]) => {
      const groupTitle = document.createElement('div');
      groupTitle.className = 'annotation-group-title';
      groupTitle.style.cssText = 'font-size:0.8rem;color:var(--text-muted);margin:16px 0 8px;padding-bottom:4px;border-bottom:1px solid var(--border);';
      groupTitle.textContent = getTitleFromPath(path);
      annotationList.appendChild(groupTitle);

      items.forEach(anno => {
        const item = document.createElement('div');
        item.className = 'annotation-item';
        item.innerHTML = `
          <div class="annotation-item-header">
            <span class="annotation-color-bar" style="background: var(--anno-${anno.color}, ${getColorHex(anno.color)})"></span>
            <span>${formatDate(anno.createdAt)}</span>
          </div>
          <div class="annotation-text">${escapeHtml(anno.text)}</div>
          ${anno.note ? `<div class="annotation-note">${escapeHtml(anno.note)}</div>` : ''}
          <div class="annotation-item-actions">
            <button class="btn-text" data-goto="${anno.path}">跳转</button>
            <button class="btn-text btn-danger" data-delete="${anno.id}">删除</button>
          </div>
        `;

        item.querySelector('[data-goto]').addEventListener('click', () => {
          loadContent(anno.path);
        });
        item.querySelector('[data-delete]').addEventListener('click', () => {
          deleteAnnotation(anno.id);
        });

        annotationList.appendChild(item);
      });
    });
  }

  function getColorHex(color) {
    const colors = {
      yellow: '#ffd666',
      green: '#a5d6a7',
      blue: '#90caf9',
      pink: '#f48fb1'
    };
    return colors[color] || '#ffd666';
  }

  function formatDate(iso) {
    try {
      const date = new Date(iso);
      return `${date.getMonth() + 1}/${date.getDate()} ${date.getHours()}:${String(date.getMinutes()).padStart(2, '0')}`;
    } catch (e) {
      return '';
    }
  }

  function exportAnnotations() {
    const data = getAnnotations();
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `esp32-annotations-${new Date().toISOString().slice(0, 10)}.json`;
    a.click();
    URL.revokeObjectURL(url);
  }

  function importAnnotations() {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = '.json';
    input.onchange = (e) => {
      const file = e.target.files[0];
      if (!file) return;

      const reader = new FileReader();
      reader.onload = (event) => {
        try {
          const imported = JSON.parse(event.target.result);
          if (Array.isArray(imported)) {
            const existing = getAnnotations();
            const merged = [...existing, ...imported];
            saveAnnotations(merged);
            restoreAnnotations();
            alert(`成功导入 ${imported.length} 条标注`);
          }
        } catch (err) {
          alert('导入失败：文件格式不正确');
        }
      };
      reader.readAsText(file);
    };
    input.click();
  }

  function clearAllAnnotations() {
    if (confirm('确定要清空所有标注吗？此操作不可恢复。')) {
      saveAnnotations([]);
      restoreAnnotations();
    }
  }

  // ==================== 侧边栏与面板 ====================

  function toggleSidebar() {
    sidebar.classList.toggle('open');
    updateOverlay();
  }

  function closeSidebar() {
    sidebar.classList.remove('open');
    updateOverlay();
  }

  function toggleAnnotationPanel() {
    annotationPanel.classList.toggle('hidden');
    contentArea.classList.toggle('with-annotation');
    renderAnnotationList();

    if (window.innerWidth <= 1024) {
      annotationPanel.classList.toggle('open');
      updateOverlay();
    }
  }

  function updateOverlay() {
    const needOverlay = sidebar.classList.contains('open') ||
                        annotationPanel.classList.contains('open');

    let overlay = document.querySelector('.overlay');
    if (needOverlay && !overlay) {
      overlay = document.createElement('div');
      overlay.className = 'overlay';
      overlay.addEventListener('click', () => {
        closeSidebar();
        annotationPanel.classList.remove('open');
        updateOverlay();
      });
      document.body.appendChild(overlay);
    }

    if (overlay) {
      overlay.classList.toggle('active', needOverlay);
      if (!needOverlay) {
        setTimeout(() => overlay.remove(), 300);
      }
    }
  }

  // ==================== 事件监听 ====================

  function setupEventListeners() {
    window.addEventListener('hashchange', handleRoute);
    window.addEventListener('popstate', handleRoute);

    document.querySelectorAll('[data-home]').forEach(el => {
      el.addEventListener('click', (e) => {
        e.preventDefault();
        showHome();
      });
    });

    document.querySelectorAll('[data-load]').forEach(el => {
      el.addEventListener('click', (e) => {
        e.preventDefault();
        loadContent(el.dataset.load);
      });
    });

    themeToggle.addEventListener('click', toggleTheme);
    menuToggle.addEventListener('click', toggleSidebar);
    annoToggle.addEventListener('click', toggleAnnotationPanel);

    document.addEventListener('mouseup', (e) => {
      if (annoPopup.contains(e.target)) return;
      setTimeout(handleTextSelection, 10);
    });

    annoPopup.querySelectorAll('.color-dot').forEach(dot => {
      dot.addEventListener('click', () => setSelectedColor(dot.dataset.color));
    });

    saveAnnoBtn.addEventListener('click', savePendingAnnotation);
    cancelAnnoBtn.addEventListener('click', hideAnnoPopup);
    exportAnnoBtn.addEventListener('click', exportAnnotations);
    importAnnoBtn.addEventListener('click', importAnnotations);
    clearAnnoBtn.addEventListener('click', clearAllAnnotations);
  }

  // ==================== 首页统计 ====================

  function updateHeroStats() {
    if (!contentIndex || !heroStats) return;

    const arduino = (contentIndex.arduino || []).length;
    const c = (contentIndex.c || []).length;
    const projects = (contentIndex.projects || []).length;

    heroStats.innerHTML = `
      <div class="stat-item">
        <div class="stat-value">${arduino}</div>
        <div class="stat-label">Arduino 教程</div>
      </div>
      <div class="stat-item">
        <div class="stat-value">${c}</div>
        <div class="stat-label">C 语言教程</div>
      </div>
      <div class="stat-item">
        <div class="stat-value">${projects}</div>
        <div class="stat-label">综合项目</div>
      </div>
    `;
  }

  init();
})();
