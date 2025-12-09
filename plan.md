# 🗺️ Projektplan: Atlas Browser

> Chromium-baserad webbläsare med inbyggt LLM-agentläge

---

## 📌 Projektöversikt

| Attribut | Värde |
|----------|-------|
| **Projektnamn** | Atlas Browser |
| **Typ** | Chromium Fork + AI Agent |
| **Målplattformar** | Linux, macOS, Windows |
| **Estimerad tid** | 12-18 månader |
| **Team storlek** | 5-10 utvecklare |

---

## 🎯 Milstolpar

```
Q1 2025        Q2 2025        Q3 2025        Q4 2025        Q1 2026
   │              │              │              │              │
   ▼              ▼              ▼              ▼              ▼
┌──────┐      ┌──────┐      ┌──────┐      ┌──────┐      ┌──────┐
│  M1  │ ──── │  M2  │ ──── │  M3  │ ──── │  M4  │ ──── │  M5  │
│Setup │      │Agent │      │ UI   │      │ Beta │      │Launch│
└──────┘      └──────┘      └──────┘      └──────┘      └──────┘
```

---

## 📅 Fas 1: Foundation (Vecka 1-8)

### Mål
Få igång Chromium-bygge med basic branding

### Uppgifter

| # | Uppgift | Ansvarig | Tid | Beroenden |
|---|---------|----------|-----|-----------|
| 1.1 | Sätta upp utvecklingsmiljö | DevOps | 1v | - |
| 1.2 | Hämta Chromium källkod | DevOps | 1v | 1.1 |
| 1.3 | Konfigurera CI/CD pipeline | DevOps | 2v | 1.2 |
| 1.4 | Skapa branding-filer | Frontend | 1v | 1.2 |
| 1.5 | Implementera AtlasMainDelegate | Core | 2v | 1.2 |
| 1.6 | Första lyckade build | Alla | 1v | 1.3-1.5 |
| 1.7 | Grundläggande testning | QA | 1v | 1.6 |

### Leverabler
- [ ] Fungerande build-pipeline
- [ ] Atlas Browser binär med custom branding
- [ ] Dokumentation för build-processen

### Risker
| Risk | Sannolikhet | Impact | Mitigation |
|------|-------------|--------|------------|
| Build-problem | Hög | Hög | Dedikerad DevOps, följ Chromium docs exakt |
| Hårdvarubrist | Medium | Hög | Cloud build machines (AWS/GCP) |

---

## 📅 Fas 2: Agent Core (Vecka 9-20)

### Mål
Implementera grundläggande LLM-agent som kan interagera med webbsidor

### Uppgifter

| # | Uppgift | Ansvarig | Tid | Beroenden |
|---|---------|----------|-----|-----------|
| 2.1 | Designa Mojo interfaces | Arkitekt | 1v | 1.6 |
| 2.2 | Implementera AgentService | Backend | 3v | 2.1 |
| 2.3 | Implementera DOMExtractor | Backend | 2v | 2.1 |
| 2.4 | Implementera LLMClient | Backend | 2v | 2.2 |
| 2.5 | Implementera ActionExecutor | Backend | 2v | 2.3 |
| 2.6 | Screenshot capture | Backend | 1v | 2.2 |
| 2.7 | Element highlighting | Frontend | 1v | 2.3 |
| 2.8 | Integration & testning | QA | 2v | 2.4-2.7 |

### Leverabler
- [ ] Fungerande agent som kan:
  - Extrahera DOM-struktur
  - Skicka till LLM (Claude/GPT)
  - Utföra click, type, scroll
  - Ta screenshots
- [ ] Grundläggande agent API

### Tekniska beslut

```
┌─────────────────────────────────────────────────────────┐
│                    BESLUT 2.1                           │
├─────────────────────────────────────────────────────────┤
│ Fråga: Vilken LLM-provider ska vara default?            │
│                                                         │
│ Alternativ:                                             │
│   A) Anthropic Claude (bäst för agenter)               │
│   B) OpenAI GPT-4 (mest känd)                          │
│   C) Lokal modell (privacy, men sämre)                 │
│                                                         │
│ Rekommendation: A) Claude som default, stöd för alla   │
└─────────────────────────────────────────────────────────┘
```

---

## 📅 Fas 3: User Interface (Vecka 21-32)

### Mål
Bygga användarvänligt gränssnitt för agenten

### Uppgifter

| # | Uppgift | Ansvarig | Tid | Beroenden |
|---|---------|----------|-----|-----------|
| 3.1 | Designa UI mockups | Design | 2v | - |
| 3.2 | Implementera AtlasToolbar | Frontend | 2v | 3.1 |
| 3.3 | Implementera AgentPanelView | Frontend | 3v | 3.1, 2.8 |
| 3.4 | Action log & visualisering | Frontend | 2v | 3.3 |
| 3.5 | Inställningar/konfiguration | Frontend | 2v | 3.3 |
| 3.6 | Keyboard shortcuts | Frontend | 1v | 3.2 |
| 3.7 | Dark mode & themes | Frontend | 1v | 3.2 |
| 3.8 | Usability testing | UX | 2v | 3.4-3.7 |

### Leverabler
- [ ] Agent panel med:
  - Task input
  - Start/pause/stop
  - Live action log
  - Element highlighting
- [ ] Inställningssida för LLM-konfiguration
- [ ] Keyboard shortcuts (Cmd+Shift+A för agent)

### UI Wireframe

```
┌─────────────────────────────────────────────────────────────────┐
│ ← → ↻  [═══════════════════ URL ════════════════════]  🤖 ⚙️  │
├─────────────────────────────────────────────────────────────────┤
│                                           ┌───────────────────┐ │
│                                           │   🤖 AI Agent     │ │
│                                           ├───────────────────┤ │
│                                           │ What should I do? │ │
│         W E B   C O N T E N T            │ ┌───────────────┐ │ │
│                                           │ │ Find best...  │ │ │
│                                           │ └───────────────┘ │ │
│                                           │ [▶️ Start]        │ │
│                                           │                   │ │
│                                           │ ─── Action Log ── │ │
│                                           │ ✓ Clicked search  │ │
│                                           │ ✓ Typed query     │ │
│                                           │ → Analyzing...    │ │
│                                           └───────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📅 Fas 4: Polish & Beta (Vecka 33-44)

### Mål
Stabilisera, optimera och förbereda för beta-release

### Uppgifter

| # | Uppgift | Ansvarig | Tid | Beroenden |
|---|---------|----------|-----|-----------|
| 4.1 | Performance-optimering | Backend | 2v | 3.8 |
| 4.2 | Säkerhetsgenomgång | Security | 2v | 3.8 |
| 4.3 | Error handling & recovery | Backend | 2v | 4.1 |
| 4.4 | Auto-update system | DevOps | 2v | 4.1 |
| 4.5 | Telemetri & analytics | Backend | 1v | 4.1 |
| 4.6 | Dokumentation | Tech Writer | 2v | 4.3 |
| 4.7 | Beta-packaging (alla plattformar) | DevOps | 2v | 4.4 |
| 4.8 | Beta-testning | QA + Community | 3v | 4.7 |

### Leverabler
- [ ] Stabil beta-release
- [ ] Auto-update fungerande
- [ ] Användardokumentation
- [ ] Known issues lista

### Beta Testplan

```
Beta Testing Waves:
━━━━━━━━━━━━━━━━━━

Wave 1 (Vecka 41): Internal team (10 användare)
  └─ Fokus: Stabilitet, crashes

Wave 2 (Vecka 42): Invited testers (100 användare)  
  └─ Fokus: Användbarhet, edge cases

Wave 3 (Vecka 43-44): Public beta (1000+ användare)
  └─ Fokus: Skalbarhet, diverse use cases
```

---

## 📅 Fas 5: Launch (Vecka 45-52)

### Mål
Offentlig release av Atlas Browser 1.0

### Uppgifter

| # | Uppgift | Ansvarig | Tid | Beroenden |
|---|---------|----------|-----|-----------|
| 5.1 | Fixa beta-feedback | Alla | 3v | 4.8 |
| 5.2 | Final säkerhetsgranskning | Security | 1v | 5.1 |
| 5.3 | Kod-signering & notarization | DevOps | 1v | 5.2 |
| 5.4 | Produktions-packaging | DevOps | 1v | 5.3 |
| 5.5 | Website & landing page | Marketing | 2v | - |
| 5.6 | Launch-kampanj | Marketing | 1v | 5.4, 5.5 |
| 5.7 | Support-setup | Support | 1v | 5.4 |
| 5.8 | **🚀 LAUNCH** | Alla | - | 5.6, 5.7 |

### Launch Checklist

- [ ] Alla plattformar byggda och testade
- [ ] Kod-signering (Apple, Microsoft)
- [ ] Auto-update server live
- [ ] Website publicerad
- [ ] Dokumentation komplett
- [ ] Support-kanaler öppna
- [ ] Monitoring & alerting aktivt
- [ ] Rollback-plan testad

---

## 👥 Team & Roller

### Föreslaget team

| Roll | Antal | Ansvar |
|------|-------|--------|
| **Tech Lead** | 1 | Arkitektur, kodgranskning |
| **Backend/Core** | 2-3 | Chromium-integration, AgentService |
| **Frontend** | 1-2 | UI, Views, UX |
| **DevOps** | 1 | CI/CD, builds, releases |
| **QA** | 1 | Testing, kvalitet |
| **Security** | 0.5 | Säkerhetsgranskningar |

### Kompetenskrav

```
Must-have:
├── C++ (modern, 17/20)
├── Chromium architecture knowledge
├── Multi-process debugging
└── GN/Ninja build system

Nice-to-have:
├── Mojo IPC experience
├── Blink/V8 internals
├── macOS/Windows native dev
└── LLM/AI integration
```

---

## 💰 Budget (Estimat)

### Utvecklingskostnader

| Kategori | Månadskostnad | 12 mån | 18 mån |
|----------|---------------|--------|--------|
| Team (6 FTE × $15K) | $90,000 | $1,080,000 | $1,620,000 |
| Cloud/Infra | $5,000 | $60,000 | $90,000 |
| LLM API-kostnader | $2,000 | $24,000 | $36,000 |
| Verktyg/Licenser | $1,000 | $12,000 | $18,000 |
| **Total** | **$98,000** | **$1,176,000** | **$1,764,000** |

### Infrastrukturkostnader

```
Build Servers:
├── Linux builds: 2× c5.4xlarge ($500/mån)
├── macOS builds: 2× Mac Studio ($200/mån, AWS Mac)
├── Windows builds: 2× m5.2xlarge ($300/mån)
└── CI runner costs: ~$500/mån

Total infra: ~$2,000-5,000/mån
```

---

## ⚠️ Risker & Mitigering

### Högrisk

| Risk | Sannolikhet | Impact | Mitigation |
|------|-------------|--------|------------|
| Chromium-uppdateringar bryter vår kod | Hög | Hög | Dedikerad tid för rebase (20% av sprint) |
| Byggtider för långa | Hög | Medium | Inkrementella builds, distributed build |
| LLM-kostnader eskalerar | Medium | Medium | Rate limiting, lokal modell-fallback |

### Mediumrisk

| Risk | Sannolikhet | Impact | Mitigation |
|------|-------------|--------|------------|
| Säkerhetssårbarheter | Medium | Hög | Regelbundna audits, bug bounty |
| macOS notarization-problem | Medium | Medium | Tidigt testa, följ Apple guidelines |
| Team attrition | Medium | Hög | Dokumentation, kunskapsdelning |

### Lågrisk

| Risk | Sannolikhet | Impact | Mitigation |
|------|-------------|--------|------------|
| Konkurrenter lanserar först | Låg | Medium | Fokus på kvalitet, nisch-features |
| LLM-provider ändrar API | Låg | Medium | Abstraktionslager, multi-provider |

---

## 📊 KPI:er & Framgångsmått

### Tekniska KPI:er

| Metrik | Mål | Mätmetod |
|--------|-----|----------|
| Build-tid | < 30 min (inkr.) | CI metrics |
| Crash rate | < 0.1% | Telemetri |
| Agent success rate | > 80% | Task completion |
| Memory usage | < 500MB (idle) | Profiling |

### Användarmått (post-launch)

| Metrik | Mål (3 mån) | Mål (12 mån) |
|--------|-------------|--------------|
| Downloads | 10,000 | 100,000 |
| DAU | 1,000 | 20,000 |
| Agent tasks/day | 5,000 | 100,000 |
| NPS | > 40 | > 50 |

---

## 🔄 Agil Process

### Sprint-struktur

```
2-veckors sprints:
━━━━━━━━━━━━━━━━━━

Dag 1:  Sprint Planning
        └─ Välj tasks från backlog
        └─ Estimera i story points

Dag 2-9: Development
        └─ Daily standups (15 min)
        └─ Pair programming för komplex kod
        └─ Code reviews (krav för merge)

Dag 10: Sprint Review
        └─ Demo av färdiga features
        └─ Stakeholder feedback

Dag 10: Retrospective
        └─ Vad gick bra/dåligt?
        └─ Action items för nästa sprint
```

### Definition of Done

- [ ] Kod skriven och self-reviewed
- [ ] Unit tests (>80% coverage för ny kod)
- [ ] Code review godkänd
- [ ] Dokumentation uppdaterad
- [ ] CI pipeline grön
- [ ] Manuellt testad på alla plattformar
- [ ] Inga kända regressioner

---

## 📁 Projektstruktur

```
atlas-browser/
├── .github/
│   └── workflows/
│       ├── build-linux.yml
│       ├── build-macos.yml
│       └── build-windows.yml
├── docs/
│   ├── architecture.md
│   ├── building.md
│   └── contributing.md
├── patches/
│   └── chromium/
│       └── [våra patches mot upstream]
├── src/
│   └── atlas_browser/
│       ├── browser/
│       ├── features/
│       ├── extensions/
│       └── resources/
├── scripts/
│   ├── build.sh
│   ├── update-chromium.sh
│   └── package.sh
├── BUILD.gn
├── args.gn
└── README.md
```

---

## 📞 Nästa steg

### Omedelbart (Vecka 1)

1. **Sätt upp utvecklingsmiljö**
   - Provisioned build-maskin (64+ kärnor, 128GB RAM)
   - Installera depot_tools
   - Klona Chromium (~30GB)

2. **Skapa projekt-repo**
   - GitHub/GitLab setup
   - Branch protection rules
   - CI/CD pipelines

3. **Team kickoff**
   - Genomgång av denna plan
   - Tilldela roller
   - Sätt upp kommunikationskanaler

### Vecka 2-4

4. **Första build**
   - Konfigurera args.gn
   - Genomför första full build
   - Dokumentera problem och lösningar

5. **Branding**
   - Skapa AtlasMainDelegate
   - Byt ut ikoner och namn
   - Verifiera custom browser startar

---

## ✅ Godkännande

| Roll | Namn | Datum | Signatur |
|------|------|-------|----------|
| Project Sponsor | | | |
| Tech Lead | | | |
| Product Owner | | | |

---

*Senast uppdaterad: 2025-01-XX*
*Version: 1.0*
