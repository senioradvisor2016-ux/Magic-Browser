# Atlas Browser API Documentation

## Overview

Atlas Browser provides a powerful API for extensions and automation. This document covers the `atlas.agent` namespace for AI-powered browser automation.

## atlas.agent API

The agent API allows extensions to control the AI agent for automated browsing tasks.

### Methods

#### `atlas.agent.runTask(task, options)`

Run an AI agent task on the current tab.

**Parameters:**
- `task` (string): Natural language description of the task
- `options` (object, optional):
  - `maxSteps` (number): Maximum steps (default: 50)
  - `timeout` (number): Timeout in ms (default: 300000)
  - `enableVision` (boolean): Use screenshots (default: true)

**Returns:** `Promise<TaskResult>`

**Example:**
```javascript
const result = await atlas.agent.runTask(
  "Find the cheapest flight from NYC to LA next Friday",
  { maxSteps: 30, enableVision: true }
);

console.log(result.status);  // "completed"
console.log(result.answer);  // "The cheapest flight is..."
```

---

#### `atlas.agent.cancelTask(taskId)`

Cancel a running agent task.

**Parameters:**
- `taskId` (string): ID of the task to cancel

**Returns:** `Promise<boolean>`

---

#### `atlas.agent.queryPage(question)`

Ask a question about the current page.

**Parameters:**
- `question` (string): Natural language question

**Returns:** `Promise<string>` - Answer from the LLM

**Example:**
```javascript
const answer = await atlas.agent.queryPage(
  "What is the main topic of this article?"
);
console.log(answer);
```

---

#### `atlas.agent.getElements(options)`

Get interactive elements from the current page.

**Parameters:**
- `options` (object, optional):
  - `selector` (string): Filter by CSS selector
  - `includeHidden` (boolean): Include hidden elements

**Returns:** `Promise<ElementInfo[]>`

**Example:**
```javascript
const elements = await atlas.agent.getElements({ selector: 'button' });

for (const elem of elements) {
  console.log(elem.text, elem.selector);
}
```

---

#### `atlas.agent.performAction(action)`

Perform a single action on the page.

**Parameters:**
- `action` (object):
  - `type` (string): "click" | "type" | "scroll" | "hover" | "navigate"
  - `selector` (string, optional): CSS selector for target
  - `value` (string, optional): Value for type/navigate actions
  - `x` (number, optional): X coordinate
  - `y` (number, optional): Y coordinate

**Returns:** `Promise<ActionResult>`

**Example:**
```javascript
// Click a button
await atlas.agent.performAction({
  type: "click",
  selector: "#submit-btn"
});

// Type into an input
await atlas.agent.performAction({
  type: "type",
  selector: "#search-input",
  value: "Atlas Browser"
});
```

---

#### `atlas.agent.getStatus()`

Get the status of the agent service.

**Returns:** `Promise<AgentStatus>`

**Example:**
```javascript
const status = await atlas.agent.getStatus();
console.log(status.enabled);  // true
console.log(status.provider); // "anthropic"
```

---

#### `atlas.agent.configure(config)`

Configure the agent service.

**Parameters:**
- `config` (object):
  - `provider` (string): "anthropic" | "openai" | "local"
  - `model` (string): Model name
  - `apiKey` (string): API key
  - `maxSteps` (number): Max steps per task
  - `enableVision` (boolean): Enable screenshots

**Returns:** `Promise<boolean>`

---

### Events

#### `atlas.agent.onTaskStarted`

Fired when an agent task starts.

```javascript
atlas.agent.onTaskStarted.addListener((taskId, task) => {
  console.log(`Task started: ${taskId}`);
});
```

#### `atlas.agent.onTaskProgress`

Fired when an agent task makes progress.

```javascript
atlas.agent.onTaskProgress.addListener((taskId, step) => {
  console.log(`Step ${step.stepNumber}: ${step.thought}`);
});
```

#### `atlas.agent.onTaskCompleted`

Fired when an agent task completes.

```javascript
atlas.agent.onTaskCompleted.addListener((taskId, result) => {
  console.log(`Task completed: ${result.status}`);
});
```

---

### Types

#### TaskResult

```typescript
interface TaskResult {
  status: "completed" | "failed" | "cancelled";
  answer?: string;
  duration: number;  // seconds
  steps: TaskStep[];
}
```

#### TaskStep

```typescript
interface TaskStep {
  stepNumber: number;
  thought: string;
  actionType: string;
  selector?: string;
  value?: string;
  success: boolean;
  error?: string;
}
```

#### ElementInfo

```typescript
interface ElementInfo {
  tag: string;
  id?: string;
  class?: string;
  text?: string;
  selector: string;
  visible: boolean;
  clickable: boolean;
  bounds: {
    x: number;
    y: number;
    width: number;
    height: number;
  };
}
```

#### ActionResult

```typescript
interface ActionResult {
  success: boolean;
  error?: string;
}
```

#### AgentStatus

```typescript
interface AgentStatus {
  enabled: boolean;
  provider: string;
  model: string;
  runningTasks: number;
}
```

---

## Permissions

To use the agent API, your extension must declare the `atlas.agent` permission:

```json
{
  "permissions": ["atlas.agent"]
}
```

---

## Best Practices

1. **Use specific selectors**: Prefer ID selectors (`#id`) over class selectors when possible.

2. **Handle failures gracefully**: Always check `result.success` and handle errors.

3. **Set reasonable timeouts**: Long tasks should have longer timeouts.

4. **Use vision when needed**: Enable vision for visually complex pages.

5. **Cancel unused tasks**: Always cancel tasks when no longer needed.

---

## Examples

### Form Automation

```javascript
// Fill out a registration form
const result = await atlas.agent.runTask(
  "Fill out the registration form with: Name: John Doe, Email: john@example.com, then submit"
);
```

### Data Extraction

```javascript
// Extract structured data
const result = await atlas.agent.runTask(
  "Find all product prices on this page and return them as a JSON array"
);

const prices = JSON.parse(result.answer);
```

### Page Analysis

```javascript
// Analyze page content
const summary = await atlas.agent.queryPage(
  "Summarize the main points of this article in 3 bullet points"
);
```

### Multi-step Workflow

```javascript
// Complex workflow with event handling
atlas.agent.onTaskProgress.addListener((taskId, step) => {
  updateUI(step);
});

const result = await atlas.agent.runTask(
  "Search for 'Atlas Browser' on Google, click the first result, and summarize the page"
);
```

---

## Rate Limits

- Maximum concurrent tasks: 1 per tab
- Maximum steps per task: 100
- API call rate limit: 60 calls/minute

---

## Error Handling

```javascript
try {
  const result = await atlas.agent.runTask("...");
  
  if (result.status === "failed") {
    console.error("Task failed:", result.steps[result.steps.length - 1].error);
  }
} catch (error) {
  console.error("API error:", error.message);
}
```
