# API Key Setup Guide

## ⚠️ Security Warning

**NEVER commit API keys to version control!**

API keys should be:
- Stored locally on your machine
- Set via environment variables
- Configured through the browser's settings UI

## Setting Up Your Anthropic API Key

### Method 1: Atlas Browser Settings (Recommended)

1. Open Atlas Browser
2. Navigate to `atlas://settings` or press `Ctrl+,`
3. Go to "AI Agent" section
4. Select "Anthropic" as provider
5. Paste your API key in the "API Key" field
6. Click "Save"

The key is stored securely in your browser profile and never exposed.

### Method 2: Environment Variable

Set the environment variable before launching Atlas:

**Linux/macOS:**
```bash
export ANTHROPIC_API_KEY="your-api-key-here"
./atlas
```

**Windows (PowerShell):**
```powershell
$env:ANTHROPIC_API_KEY="your-api-key-here"
.\atlas.exe
```

**Windows (CMD):**
```cmd
set ANTHROPIC_API_KEY=your-api-key-here
atlas.exe
```

### Method 3: Configuration File (Development Only)

1. Copy the example config:
   ```bash
   cp config/api_keys.example.json config/api_keys.json
   ```

2. Edit `config/api_keys.json` and add your key:
   ```json
   {
     "anthropic": {
       "api_key": "sk-ant-api03-xxxxx",
       "model": "claude-sonnet-4-20250514"
     }
   }
   ```

3. **IMPORTANT**: Ensure `api_keys.json` is in `.gitignore`!

## Getting API Keys

### Anthropic (Claude)
1. Go to https://console.anthropic.com/
2. Create an account or sign in
3. Navigate to "API Keys"
4. Click "Create Key"
5. Copy the key (starts with `sk-ant-`)

### OpenAI (GPT-4)
1. Go to https://platform.openai.com/
2. Create an account or sign in
3. Navigate to "API Keys"
4. Click "Create new secret key"
5. Copy the key (starts with `sk-`)

### Local Models (Ollama)
No API key needed! Just:
1. Install Ollama: https://ollama.ai/
2. Pull a model: `ollama pull llama3`
3. Select "Local" in Atlas settings

## Verifying Your Key

1. Open Atlas Browser
2. Open the Agent Panel (`Ctrl+Shift+A`)
3. Type a simple task: "What is 2+2?"
4. Click "Start"

If configured correctly, you'll see the agent respond.

## Troubleshooting

### "Invalid API Key" Error
- Check that you copied the full key
- Verify the key hasn't expired
- Ensure you have API credits/quota

### "Network Error"
- Check your internet connection
- Verify firewall isn't blocking API requests
- Try a different network

### Key Not Persisting
- Check that you clicked "Save" in settings
- Verify your profile directory is writable
- Try restarting the browser

## Security Best Practices

1. **Rotate keys regularly** - Change your API keys every few months
2. **Use separate keys** - Development vs production
3. **Monitor usage** - Check your API dashboard for unusual activity
4. **Set spending limits** - Configure billing alerts
5. **Never share keys** - Each developer should have their own
