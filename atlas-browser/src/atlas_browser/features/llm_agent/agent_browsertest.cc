// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/features/llm_agent/browser/agent_service.h"

#include "base/test/bind.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace atlas {

class AgentBrowserTest : public InProcessBrowserTest {
 protected:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    
    // Start test server
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  AgentService* GetAgentService() {
    return AgentService::GetForBrowserContext(browser()->profile());
  }

  content::WebContents* GetActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }
};

// Basic test that agent service is available
IN_PROC_BROWSER_TEST_F(AgentBrowserTest, ServiceExists) {
  EXPECT_NE(GetAgentService(), nullptr);
}

// Test agent configuration persists
IN_PROC_BROWSER_TEST_F(AgentBrowserTest, ConfigurationPersists) {
  auto* service = GetAgentService();
  ASSERT_NE(service, nullptr);

  AgentService::Config config = service->GetConfig();
  config.max_steps = 100;
  config.enable_vision = false;
  service->Configure(config);

  EXPECT_EQ(service->GetConfig().max_steps, 100);
  EXPECT_FALSE(service->GetConfig().enable_vision);
}

// Test that agent can be started on a page
IN_PROC_BROWSER_TEST_F(AgentBrowserTest, StartTaskOnPage) {
  // Navigate to test page
  GURL test_url = embedded_test_server()->GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));

  auto* service = GetAgentService();
  ASSERT_NE(service, nullptr);

  // Configure with mock provider for testing
  AgentService::Config config;
  config.provider = AgentService::Config::Provider::kMock;
  service->Configure(config);

  // Start a task
  base::RunLoop run_loop;
  bool task_completed = false;

  service->StartTask(
      GetActiveWebContents(),
      "Test task",
      base::BindLambdaForTesting(
          [&](agent::mojom::TaskResultPtr result) {
            task_completed = true;
            run_loop.Quit();
          }));

  // Wait for completion (with timeout)
  run_loop.Run();
  EXPECT_TRUE(task_completed);
}

// Test cancelling a task
IN_PROC_BROWSER_TEST_F(AgentBrowserTest, CancelTask) {
  // Navigate to test page
  GURL test_url = embedded_test_server()->GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));

  auto* service = GetAgentService();
  ASSERT_NE(service, nullptr);

  // Start and immediately cancel
  service->StartTask(
      GetActiveWebContents(),
      "Long running task",
      base::BindLambdaForTesting(
          [](agent::mojom::TaskResultPtr result) {}));

  // Cancel all tasks
  service->CancelAllTasks();

  // Service should be idle
  EXPECT_FALSE(service->HasActiveTasks());
}

// Test DOM extraction
IN_PROC_BROWSER_TEST_F(AgentBrowserTest, DOMExtraction) {
  // Navigate to test page with interactive elements
  GURL test_url = embedded_test_server()->GetURL("/form.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));

  auto* service = GetAgentService();
  ASSERT_NE(service, nullptr);

  base::RunLoop run_loop;
  std::string dom_result;
  std::vector<agent::mojom::ElementInfoPtr> elements;

  service->GetDOM(
      GetActiveWebContents(),
      base::BindLambdaForTesting(
          [&](const std::string& dom,
              std::vector<agent::mojom::ElementInfoPtr> elems) {
            dom_result = dom;
            elements = std::move(elems);
            run_loop.Quit();
          }));

  run_loop.Run();

  // Should have extracted DOM
  EXPECT_FALSE(dom_result.empty());
  // Should have found interactive elements (form has inputs, buttons)
  EXPECT_GT(elements.size(), 0u);
}

// Test screenshot capture
IN_PROC_BROWSER_TEST_F(AgentBrowserTest, ScreenshotCapture) {
  // Navigate to any page
  GURL test_url = embedded_test_server()->GetURL("/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));

  auto* service = GetAgentService();
  ASSERT_NE(service, nullptr);

  base::RunLoop run_loop;
  std::string screenshot_base64;

  service->CaptureScreenshot(
      GetActiveWebContents(),
      base::BindLambdaForTesting(
          [&](const std::string& screenshot) {
            screenshot_base64 = screenshot;
            run_loop.Quit();
          }));

  run_loop.Run();

  // Should have captured screenshot (base64 PNG)
  EXPECT_FALSE(screenshot_base64.empty());
  // Base64 PNG starts with "iVBOR"
  EXPECT_TRUE(screenshot_base64.find("iVBOR") == 0 ||
              screenshot_base64.find("/9j/") == 0);  // or JPEG
}

// Test performing actions
IN_PROC_BROWSER_TEST_F(AgentBrowserTest, PerformClickAction) {
  // Navigate to test page with a button
  GURL test_url = embedded_test_server()->GetURL("/button.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));

  auto* service = GetAgentService();
  ASSERT_NE(service, nullptr);

  // Create click action
  auto action = agent::mojom::AgentAction::New();
  action->type = agent::mojom::ActionType::kClick;
  action->selector = "#test-button";

  base::RunLoop run_loop;
  agent::mojom::ActionResultPtr result;

  service->PerformAction(
      GetActiveWebContents(),
      std::move(action),
      base::BindLambdaForTesting(
          [&](agent::mojom::ActionResultPtr r) {
            result = std::move(r);
            run_loop.Quit();
          }));

  run_loop.Run();

  // Click should succeed
  EXPECT_TRUE(result->success);
}

// Test performing type action
IN_PROC_BROWSER_TEST_F(AgentBrowserTest, PerformTypeAction) {
  // Navigate to test page with an input
  GURL test_url = embedded_test_server()->GetURL("/form.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), test_url));

  auto* service = GetAgentService();
  ASSERT_NE(service, nullptr);

  // Create type action
  auto action = agent::mojom::AgentAction::New();
  action->type = agent::mojom::ActionType::kType;
  action->selector = "#name-input";
  action->value = "Test User";

  base::RunLoop run_loop;
  agent::mojom::ActionResultPtr result;

  service->PerformAction(
      GetActiveWebContents(),
      std::move(action),
      base::BindLambdaForTesting(
          [&](agent::mojom::ActionResultPtr r) {
            result = std::move(r);
            run_loop.Quit();
          }));

  run_loop.Run();

  // Type should succeed
  EXPECT_TRUE(result->success);
}

}  // namespace atlas
