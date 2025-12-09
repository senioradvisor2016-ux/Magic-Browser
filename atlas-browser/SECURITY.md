# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 1.x.x   | :white_check_mark: |
| < 1.0   | :x:                |

## Reporting a Vulnerability

We take security seriously at Atlas Browser. If you discover a security vulnerability, please follow these steps:

### 1. Do NOT open a public issue

Security vulnerabilities should not be reported through public GitHub issues.

### 2. Email us directly

Send an email to **security@atlasbrowser.com** with:

- A description of the vulnerability
- Steps to reproduce the issue
- Potential impact assessment
- Any suggested fixes (if applicable)

### 3. What to expect

- **Initial Response**: Within 48 hours
- **Status Update**: Within 7 days
- **Resolution Timeline**: Depends on severity
  - Critical: 24-48 hours
  - High: 7 days
  - Medium: 30 days
  - Low: 90 days

### 4. Disclosure Policy

- We will work with you to understand and resolve the issue
- We will credit you in the security advisory (unless you prefer anonymity)
- We ask that you do not disclose the vulnerability publicly until we've had time to address it

## Security Best Practices for Users

### API Key Security

- Never share your LLM API keys
- Store API keys only in Atlas Browser's secure settings
- Rotate keys periodically
- Use separate keys for development and production

### Privacy

- Review permissions granted to the AI agent
- Be cautious when running agent tasks on sensitive pages
- Use Private Browsing mode for sensitive tasks
- Regularly clear browsing data

### Extensions

- Only install extensions from trusted sources
- Review extension permissions carefully
- Keep extensions updated

## Security Features

### Built-in Protections

- **Sandboxed Processes**: Each tab runs in an isolated process
- **Tracker Blocking**: Built-in protection against known trackers
- **HTTPS Enforcement**: Automatic upgrade to HTTPS when available
- **Safe Browsing**: Protection against known malicious sites

### Agent Security

- **Permission System**: Agent actions require explicit user consent
- **Action Logging**: All agent actions are logged for review
- **Timeout Protection**: Tasks automatically stop after configurable timeout
- **Domain Restrictions**: Option to restrict agent to specific domains

## Acknowledgments

We thank the following security researchers for their responsible disclosures:

- (This section will be updated as we receive and address reports)

## Contact

- Security issues: security@atlasbrowser.com
- General questions: support@atlasbrowser.com
- PGP Key: Available at https://atlasbrowser.com/.well-known/security.txt
