#include <juce_gui_extra/juce_gui_extra.h>

#include "MainComponent.h"

namespace
{
class PG300ControllerApplication final : public juce::JUCEApplication
{
public:
  const juce::String getApplicationName() override       { return "PG-300 Controller"; }
  const juce::String getApplicationVersion() override    { return "0.1.0"; }
  bool moreThanOneInstanceAllowed() override             { return true; }

  void initialise(const juce::String&) override
  {
    mainWindow_.reset(new MainWindow(getApplicationName()));
  }

  void shutdown() override
  {
    mainWindow_ = nullptr;
  }

  void systemRequestedQuit() override
  {
    quit();
  }

  void anotherInstanceStarted(const juce::String&) override {}

private:
  class MainWindow final : public juce::DocumentWindow
  {
  public:
    explicit MainWindow(juce::String name)
      : juce::DocumentWindow(std::move(name),
                             juce::Desktop::getInstance().getDefaultLookAndFeel()
                               .findColour(juce::ResizableWindow::backgroundColourId),
                             juce::DocumentWindow::allButtons)
    {
      setUsingNativeTitleBar(true);
      setContentOwned(new MainComponent(), true);
      setResizable(true, true);
      centreWithSize(1200, 720);
      setVisible(true);
    }

    void closeButtonPressed() override
    {
      juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
  };

  std::unique_ptr<MainWindow> mainWindow_;
};
} // namespace

START_JUCE_APPLICATION(PG300ControllerApplication)

