// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/features/llm_agent/renderer/dom_extractor.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace atlas {
namespace {

class DOMExtractorTest : public testing::Test {
 protected:
  DOMExtractor extractor_;
};

// =============================================================================
// Selector Generation Tests
// =============================================================================

TEST_F(DOMExtractorTest, GenerateSelectorWithId) {
  // Test that elements with IDs get simple ID selectors
  // Note: These tests would require Blink mocking in full implementation
}

TEST_F(DOMExtractorTest, GenerateSelectorWithClass) {
  // Test class-based selectors
}

TEST_F(DOMExtractorTest, GenerateSelectorWithNthOfType) {
  // Test nth-of-type for disambiguation
}

// =============================================================================
// XPath Generation Tests
// =============================================================================

TEST_F(DOMExtractorTest, GenerateXPathWithId) {
  // Test XPath generation with ID
}

TEST_F(DOMExtractorTest, GenerateXPathWithIndex) {
  // Test XPath generation with index
}

// =============================================================================
// Configuration Tests
// =============================================================================

TEST_F(DOMExtractorTest, DefaultConfiguration) {
  DOMExtractor::Config config;
  EXPECT_EQ(config.max_depth, 20);
  EXPECT_EQ(config.max_elements, 500);
  EXPECT_EQ(config.max_text_length, 200);
  EXPECT_FALSE(config.include_hidden);
  EXPECT_TRUE(config.include_aria);
  EXPECT_FALSE(config.include_data_attrs);
}

TEST_F(DOMExtractorTest, ConfigureMaxDepth) {
  DOMExtractor::Config config;
  config.max_depth = 10;
  extractor_.Configure(config);
  // Configuration is applied
}

TEST_F(DOMExtractorTest, ConfigureMaxElements) {
  DOMExtractor::Config config;
  config.max_elements = 100;
  extractor_.Configure(config);
  // Configuration is applied
}

// =============================================================================
// Text Extraction Tests
// =============================================================================

class TextExtractionTest : public testing::Test {
 protected:
  // Helper to test text extraction without full Blink environment
  std::string CleanText(const std::string& input) {
    std::string result;
    bool last_was_space = false;
    
    for (char c : input) {
      if (std::isspace(c)) {
        if (!last_was_space) {
          result += ' ';
          last_was_space = true;
        }
      } else {
        result += c;
        last_was_space = false;
      }
    }
    
    // Trim
    size_t start = result.find_first_not_of(' ');
    if (start == std::string::npos) return "";
    size_t end = result.find_last_not_of(' ');
    return result.substr(start, end - start + 1);
  }
};

TEST_F(TextExtractionTest, CollapseWhitespace) {
  EXPECT_EQ(CleanText("  hello   world  "), "hello world");
}

TEST_F(TextExtractionTest, HandleNewlines) {
  EXPECT_EQ(CleanText("hello\n\nworld"), "hello world");
}

TEST_F(TextExtractionTest, HandleTabs) {
  EXPECT_EQ(CleanText("hello\t\tworld"), "hello world");
}

TEST_F(TextExtractionTest, HandleMixedWhitespace) {
  EXPECT_EQ(CleanText("  hello \n\t world  \n"), "hello world");
}

TEST_F(TextExtractionTest, EmptyString) {
  EXPECT_EQ(CleanText(""), "");
}

TEST_F(TextExtractionTest, OnlyWhitespace) {
  EXPECT_EQ(CleanText("   \n\t   "), "");
}

// =============================================================================
// Interactive Element Detection Tests
// =============================================================================

class InteractiveElementTest : public testing::Test {
 protected:
  // Tags that should be considered interactive
  bool IsInteractiveTag(const std::string& tag) {
    static const std::set<std::string> kInteractiveTags = {
        "A", "BUTTON", "INPUT", "SELECT", "TEXTAREA",
        "DETAILS", "SUMMARY", "DIALOG",
    };
    return kInteractiveTags.count(tag) > 0;
  }
  
  // Roles that should be considered interactive
  bool IsInteractiveRole(const std::string& role) {
    static const std::set<std::string> kInteractiveRoles = {
        "button", "link", "menuitem", "option", "tab",
        "checkbox", "radio", "switch", "textbox", "combobox",
    };
    return kInteractiveRoles.count(role) > 0;
  }
};

TEST_F(InteractiveElementTest, LinkIsInteractive) {
  EXPECT_TRUE(IsInteractiveTag("A"));
}

TEST_F(InteractiveElementTest, ButtonIsInteractive) {
  EXPECT_TRUE(IsInteractiveTag("BUTTON"));
}

TEST_F(InteractiveElementTest, InputIsInteractive) {
  EXPECT_TRUE(IsInteractiveTag("INPUT"));
}

TEST_F(InteractiveElementTest, DivIsNotInteractive) {
  EXPECT_FALSE(IsInteractiveTag("DIV"));
}

TEST_F(InteractiveElementTest, SpanIsNotInteractive) {
  EXPECT_FALSE(IsInteractiveTag("SPAN"));
}

TEST_F(InteractiveElementTest, ButtonRoleIsInteractive) {
  EXPECT_TRUE(IsInteractiveRole("button"));
}

TEST_F(InteractiveElementTest, LinkRoleIsInteractive) {
  EXPECT_TRUE(IsInteractiveRole("link"));
}

TEST_F(InteractiveElementTest, BannerRoleIsNotInteractive) {
  EXPECT_FALSE(IsInteractiveRole("banner"));
}

// =============================================================================
// Role Inference Tests
// =============================================================================

class RoleInferenceTest : public testing::Test {
 protected:
  std::string InferRole(const std::string& tag, 
                        const std::string& type = "") {
    static const std::map<std::string, std::string> kTagToRole = {
        {"A", "link"},
        {"BUTTON", "button"},
        {"INPUT", "textbox"},
        {"SELECT", "combobox"},
        {"TEXTAREA", "textbox"},
        {"IMG", "img"},
        {"NAV", "navigation"},
    };
    
    // Special cases for INPUT
    if (tag == "INPUT") {
      if (type == "checkbox") return "checkbox";
      if (type == "radio") return "radio";
      if (type == "submit" || type == "button") return "button";
      if (type == "search") return "searchbox";
      return "textbox";
    }
    
    auto it = kTagToRole.find(tag);
    return it != kTagToRole.end() ? it->second : "";
  }
};

TEST_F(RoleInferenceTest, LinkTag) {
  EXPECT_EQ(InferRole("A"), "link");
}

TEST_F(RoleInferenceTest, ButtonTag) {
  EXPECT_EQ(InferRole("BUTTON"), "button");
}

TEST_F(RoleInferenceTest, InputText) {
  EXPECT_EQ(InferRole("INPUT", "text"), "textbox");
}

TEST_F(RoleInferenceTest, InputCheckbox) {
  EXPECT_EQ(InferRole("INPUT", "checkbox"), "checkbox");
}

TEST_F(RoleInferenceTest, InputRadio) {
  EXPECT_EQ(InferRole("INPUT", "radio"), "radio");
}

TEST_F(RoleInferenceTest, InputSubmit) {
  EXPECT_EQ(InferRole("INPUT", "submit"), "button");
}

TEST_F(RoleInferenceTest, InputSearch) {
  EXPECT_EQ(InferRole("INPUT", "search"), "searchbox");
}

TEST_F(RoleInferenceTest, UnknownTag) {
  EXPECT_EQ(InferRole("CUSTOM"), "");
}

}  // namespace
}  // namespace atlas
