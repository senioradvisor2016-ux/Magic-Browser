// Atlas Browser New Tab Page - JavaScript

/**
 * New Tab Page Manager
 */
class NewTabPage {
  constructor() {
    this.currentEngine = 'google';
    this.init();
  }

  init() {
    this.initSearch();
    this.initEngineSelector();
    this.initShortcuts();
    this.initAgentActions();
    this.initModal();
    this.loadStats();
  }

  // ==========================================================================
  // Search
  // ==========================================================================

  initSearch() {
    const searchInput = document.getElementById('search-input');
    const agentSearchBtn = document.getElementById('agent-search');

    searchInput.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') {
        this.performSearch(searchInput.value);
      }
    });

    agentSearchBtn.addEventListener('click', () => {
      const query = searchInput.value;
      if (query) {
        this.showAgentModal(`Search for: ${query}`);
      } else {
        this.showAgentModal();
      }
    });
  }

  performSearch(query) {
    if (!query.trim()) return;

    // Check if it's a URL
    if (this.isUrl(query)) {
      window.location.href = this.normalizeUrl(query);
      return;
    }

    // Search engines
    const engines = {
      google: `https://www.google.com/search?q=${encodeURIComponent(query)}`,
      duckduckgo: `https://duckduckgo.com/?q=${encodeURIComponent(query)}`,
      bing: `https://www.bing.com/search?q=${encodeURIComponent(query)}`,
    };

    window.location.href = engines[this.currentEngine];
  }

  isUrl(str) {
    // Simple URL detection
    return /^(https?:\/\/|www\.)/i.test(str) || 
           /^[a-z0-9]+([\-\.]{1}[a-z0-9]+)*\.[a-z]{2,}$/i.test(str);
  }

  normalizeUrl(url) {
    if (!/^https?:\/\//i.test(url)) {
      return 'https://' + url;
    }
    return url;
  }

  // ==========================================================================
  // Search Engine Selector
  // ==========================================================================

  initEngineSelector() {
    const engines = document.querySelectorAll('.engine');
    
    engines.forEach(btn => {
      btn.addEventListener('click', () => {
        engines.forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        this.currentEngine = btn.dataset.engine;
        localStorage.setItem('atlas-search-engine', this.currentEngine);
      });
    });

    // Load saved preference
    const saved = localStorage.getItem('atlas-search-engine');
    if (saved) {
      this.currentEngine = saved;
      engines.forEach(b => {
        b.classList.toggle('active', b.dataset.engine === saved);
      });
    }
  }

  // ==========================================================================
  // Shortcuts
  // ==========================================================================

  initShortcuts() {
    const addBtn = document.getElementById('add-shortcut');
    
    addBtn.addEventListener('click', () => {
      this.addShortcut();
    });

    // Load custom shortcuts
    this.loadShortcuts();
  }

  addShortcut() {
    const name = prompt('Enter shortcut name:');
    if (!name) return;

    const url = prompt('Enter URL:');
    if (!url) return;

    const shortcuts = this.getShortcuts();
    shortcuts.push({ name, url, icon: '🔗' });
    localStorage.setItem('atlas-shortcuts', JSON.stringify(shortcuts));
    
    this.renderShortcuts();
  }

  getShortcuts() {
    const saved = localStorage.getItem('atlas-shortcuts');
    return saved ? JSON.parse(saved) : [];
  }

  loadShortcuts() {
    // Custom shortcuts would be loaded and rendered here
  }

  renderShortcuts() {
    // Re-render shortcuts grid
  }

  // ==========================================================================
  // Agent Actions
  // ==========================================================================

  initAgentActions() {
    const actions = document.querySelectorAll('.agent-action');
    
    actions.forEach(btn => {
      btn.addEventListener('click', () => {
        const task = btn.dataset.task;
        this.handleAgentAction(task);
      });
    });
  }

  handleAgentAction(task) {
    const prompts = {
      search: 'Search for the following and summarize the results:',
      summarize: 'Summarize the current page content',
      compare: 'Find and compare prices for:',
      fill: 'Fill out the form on this page with:',
    };

    this.showAgentModal(prompts[task] || '');
  }

  // ==========================================================================
  // Modal
  // ==========================================================================

  initModal() {
    const modal = document.getElementById('agent-modal');
    const closeBtn = document.getElementById('modal-close');
    const cancelBtn = document.getElementById('modal-cancel');
    const startBtn = document.getElementById('modal-start');
    const taskInput = document.getElementById('agent-task-input');

    closeBtn.addEventListener('click', () => this.hideModal());
    cancelBtn.addEventListener('click', () => this.hideModal());
    
    modal.addEventListener('click', (e) => {
      if (e.target === modal) this.hideModal();
    });

    startBtn.addEventListener('click', () => {
      const task = taskInput.value;
      const useVision = document.getElementById('agent-vision').checked;
      this.startAgentTask(task, useVision);
    });

    // Escape to close
    document.addEventListener('keydown', (e) => {
      if (e.key === 'Escape') this.hideModal();
    });
  }

  showAgentModal(prefill = '') {
    const modal = document.getElementById('agent-modal');
    const taskInput = document.getElementById('agent-task-input');
    
    modal.classList.add('active');
    taskInput.value = prefill;
    taskInput.focus();
  }

  hideModal() {
    const modal = document.getElementById('agent-modal');
    modal.classList.remove('active');
  }

  startAgentTask(task, useVision) {
    if (!task.trim()) {
      alert('Please enter a task description');
      return;
    }

    // In real implementation:
    // chrome.send('startAgentTask', [task, useVision]);
    
    console.log('Starting agent task:', task, 'Vision:', useVision);
    
    // For demo, show a notification
    this.hideModal();
    this.showNotification('Agent task started: ' + task.substring(0, 50) + '...');
  }

  // ==========================================================================
  // Stats
  // ==========================================================================

  loadStats() {
    // In real implementation:
    // chrome.send('getStats');
    
    // For demo, use localStorage or defaults
    const trackersBlocked = localStorage.getItem('atlas-trackers-blocked') || 0;
    const tasksCompleted = localStorage.getItem('atlas-tasks-completed') || 0;
    
    document.getElementById('trackers-blocked').textContent = 
        Number(trackersBlocked).toLocaleString();
    document.getElementById('tasks-completed').textContent = 
        Number(tasksCompleted).toLocaleString();
  }

  // ==========================================================================
  // Utilities
  // ==========================================================================

  showNotification(message) {
    // Simple notification
    // In real implementation, use proper notification system
    console.log('Notification:', message);
  }
}

// ==========================================================================
// Callback Handlers (called from C++ backend)
// ==========================================================================

function statsLoaded(stats) {
  document.getElementById('trackers-blocked').textContent = 
      stats.trackersBlocked.toLocaleString();
  document.getElementById('tasks-completed').textContent = 
      stats.tasksCompleted.toLocaleString();
}

function agentTaskStarted(taskId) {
  console.log('Agent task started:', taskId);
  // Could show progress indicator
}

function agentTaskCompleted(result) {
  console.log('Agent task completed:', result);
  // Update UI
}

// ==========================================================================
// Initialize
// ==========================================================================

document.addEventListener('DOMContentLoaded', () => {
  window.newTabPage = new NewTabPage();
});
