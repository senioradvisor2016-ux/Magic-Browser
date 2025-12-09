// Atlas Browser Settings - JavaScript

/**
 * Settings Manager
 * Handles communication with the browser backend via chrome.send()
 */
class SettingsManager {
  constructor() {
    this.settings = {};
    this.init();
  }

  init() {
    // Initialize navigation
    this.initNavigation();
    
    // Load settings from backend
    this.loadSettings();
    
    // Initialize event listeners
    this.initEventListeners();
    
    // Initialize API key toggle
    this.initApiKeyToggle();
  }

  // ==========================================================================
  // Navigation
  // ==========================================================================

  initNavigation() {
    const navItems = document.querySelectorAll('.nav-item');
    
    navItems.forEach(item => {
      item.addEventListener('click', (e) => {
        e.preventDefault();
        const section = item.dataset.section;
        this.showSection(section);
      });
    });

    // Handle hash navigation
    if (window.location.hash) {
      const section = window.location.hash.substring(1);
      this.showSection(section);
    }
  }

  showSection(sectionId) {
    // Update nav
    document.querySelectorAll('.nav-item').forEach(item => {
      item.classList.toggle('active', item.dataset.section === sectionId);
    });

    // Update sections
    document.querySelectorAll('.settings-section').forEach(section => {
      section.classList.toggle('active', section.id === sectionId);
    });

    // Update URL
    history.replaceState(null, '', `#${sectionId}`);
  }

  // ==========================================================================
  // Settings Management
  // ==========================================================================

  loadSettings() {
    // Request settings from backend
    // In real implementation: chrome.send('loadSettings');
    
    // For demo, use defaults
    this.settings = {
      agent: {
        enabled: true,
        provider: 'anthropic',
        model: 'claude-sonnet-4-20250514',
        apiKey: '',
        maxSteps: 50,
        enableVision: true,
        enableLabels: true,
      },
      privacy: {
        trackerBlocking: true,
        fingerprintProtection: true,
        blockedDomains: [],
      },
      updates: {
        autoUpdate: true,
        channel: 'stable',
      },
    };

    this.applySettings();
  }

  applySettings() {
    // Agent settings
    document.getElementById('agent-enabled').checked = this.settings.agent.enabled;
    document.getElementById('agent-provider').value = this.settings.agent.provider;
    document.getElementById('agent-model').value = this.settings.agent.model;
    document.getElementById('agent-api-key').value = this.settings.agent.apiKey;
    document.getElementById('agent-max-steps').value = this.settings.agent.maxSteps;
    document.getElementById('agent-vision').checked = this.settings.agent.enableVision;
    document.getElementById('agent-labels').checked = this.settings.agent.enableLabels;

    // Privacy settings
    document.getElementById('privacy-tracker-blocking').checked = 
        this.settings.privacy.trackerBlocking;
    document.getElementById('privacy-fingerprint').checked = 
        this.settings.privacy.fingerprintProtection;
    document.getElementById('blocked-domains').value = 
        this.settings.privacy.blockedDomains.join('\n');

    // Update settings
    document.getElementById('auto-updates').checked = this.settings.updates.autoUpdate;
    document.getElementById('update-channel').value = this.settings.updates.channel;
  }

  saveSettings() {
    // Collect all settings
    this.settings.agent.enabled = document.getElementById('agent-enabled').checked;
    this.settings.agent.provider = document.getElementById('agent-provider').value;
    this.settings.agent.model = document.getElementById('agent-model').value;
    this.settings.agent.apiKey = document.getElementById('agent-api-key').value;
    this.settings.agent.maxSteps = parseInt(document.getElementById('agent-max-steps').value);
    this.settings.agent.enableVision = document.getElementById('agent-vision').checked;
    this.settings.agent.enableLabels = document.getElementById('agent-labels').checked;

    this.settings.privacy.trackerBlocking = 
        document.getElementById('privacy-tracker-blocking').checked;
    this.settings.privacy.fingerprintProtection = 
        document.getElementById('privacy-fingerprint').checked;
    this.settings.privacy.blockedDomains = 
        document.getElementById('blocked-domains').value
            .split('\n')
            .filter(d => d.trim());

    this.settings.updates.autoUpdate = document.getElementById('auto-updates').checked;
    this.settings.updates.channel = document.getElementById('update-channel').value;

    // Send to backend
    // In real implementation: chrome.send('saveSettings', [this.settings]);
    console.log('Settings saved:', this.settings);
    
    this.showNotification('Settings saved!');
  }

  // ==========================================================================
  // Event Listeners
  // ==========================================================================

  initEventListeners() {
    // Auto-save on change
    const inputs = document.querySelectorAll('input, select, textarea');
    inputs.forEach(input => {
      input.addEventListener('change', () => this.saveSettings());
    });

    // Provider change - update model options
    document.getElementById('agent-provider').addEventListener('change', (e) => {
      this.updateModelOptions(e.target.value);
    });

    // Check for updates button
    document.getElementById('check-updates').addEventListener('click', () => {
      this.checkForUpdates();
    });
  }

  initApiKeyToggle() {
    const toggleBtn = document.getElementById('toggle-api-key');
    const apiKeyInput = document.getElementById('agent-api-key');
    
    toggleBtn.addEventListener('click', () => {
      if (apiKeyInput.type === 'password') {
        apiKeyInput.type = 'text';
        toggleBtn.textContent = 'Hide';
      } else {
        apiKeyInput.type = 'password';
        toggleBtn.textContent = 'Show';
      }
    });
  }

  updateModelOptions(provider) {
    const modelSelect = document.getElementById('agent-model');
    modelSelect.innerHTML = '';

    const models = {
      anthropic: [
        { value: 'claude-sonnet-4-20250514', label: 'Claude Sonnet 4' },
        { value: 'claude-3-5-sonnet-20241022', label: 'Claude 3.5 Sonnet' },
        { value: 'claude-3-opus-20240229', label: 'Claude 3 Opus' },
      ],
      openai: [
        { value: 'gpt-4o', label: 'GPT-4o' },
        { value: 'gpt-4-turbo', label: 'GPT-4 Turbo' },
        { value: 'gpt-4', label: 'GPT-4' },
      ],
      local: [
        { value: 'llama3', label: 'Llama 3' },
        { value: 'llama2', label: 'Llama 2' },
        { value: 'mistral', label: 'Mistral' },
        { value: 'codellama', label: 'Code Llama' },
      ],
    };

    (models[provider] || []).forEach(model => {
      const option = document.createElement('option');
      option.value = model.value;
      option.textContent = model.label;
      modelSelect.appendChild(option);
    });
  }

  // ==========================================================================
  // Updates
  // ==========================================================================

  checkForUpdates() {
    const statusText = document.getElementById('update-status-text');
    const checkBtn = document.getElementById('check-updates');
    
    checkBtn.disabled = true;
    statusText.textContent = 'Checking for updates...';

    // In real implementation: chrome.send('checkForUpdates');
    
    // Simulate check
    setTimeout(() => {
      statusText.textContent = "You're up to date!";
      checkBtn.disabled = false;
    }, 2000);
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

function settingsLoaded(settings) {
  window.settingsManager.settings = settings;
  window.settingsManager.applySettings();
}

function updateCheckResult(hasUpdate, version) {
  const statusText = document.getElementById('update-status-text');
  const checkBtn = document.getElementById('check-updates');
  
  checkBtn.disabled = false;
  
  if (hasUpdate) {
    statusText.textContent = `Update available: ${version}`;
  } else {
    statusText.textContent = "You're up to date!";
  }
}

function statisticsUpdated(stats) {
  document.getElementById('trackers-blocked').textContent = 
      stats.trackersBlocked.toLocaleString();
  document.getElementById('fingerprints-blocked').textContent = 
      stats.fingerprintsBlocked.toLocaleString();
}

// ==========================================================================
// Initialize
// ==========================================================================

document.addEventListener('DOMContentLoaded', () => {
  window.settingsManager = new SettingsManager();
});
