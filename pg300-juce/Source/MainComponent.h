#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "AlphaJunoBank.h"
#include "MidiEngine.h"
#include "PGParameters.h"
#include "ParameterControl.h"

#include <optional>
#include <unordered_map>

class MainComponent final : public juce::Component
{
public:
  MainComponent();
  ~MainComponent() override;

  void paint(juce::Graphics& g) override;
  void resized() override;

private:
  class ToneListModel final : public juce::ListBoxModel
  {
  public:
    explicit ToneListModel(MainComponent& owner) : owner_(owner) {}
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;

  private:
    MainComponent& owner_;
  };

  class EditorSurface final : public juce::Component
  {
  public:
    explicit EditorSurface(const std::vector<pg300::PGParameter>& params);

    void resized() override;

    void setValue(uint8_t paramId, uint8_t value, juce::NotificationType nt = juce::sendNotification);
    uint8_t getValue(uint8_t paramId) const;

    std::function<void(uint8_t paramId, uint8_t value)> onParamChanged;

  private:
    static juce::String sectionForParam(uint8_t paramId);
    static int columnsForSection(const juce::String& section);

    void layoutSection(juce::GroupComponent& group,
                       const std::vector<pg300::ParameterControl*>& controls,
                       int columns);

    std::unordered_map<uint8_t, pg300::ParameterControl*> controlsById_;

    juce::GroupComponent dcoGroup_ { {}, "DCO" };
    juce::GroupComponent vcfGroup_ { {}, "VCF" };
    juce::GroupComponent vcaGroup_ { {}, "VCA" };
    juce::GroupComponent lfoGroup_ { {}, "LFO" };
    juce::GroupComponent envGroup_ { {}, "ENV" };
    juce::GroupComponent chorusGroup_ { {}, "CHORUS" };
    juce::GroupComponent miscGroup_ { {}, "MISC" };

    juce::OwnedArray<pg300::ParameterControl> allControls_;
    std::vector<pg300::ParameterControl*> dcoControls_, vcfControls_, vcaControls_, lfoControls_, envControls_, chorusControls_, miscControls_;
  };

  void refreshMidiDeviceLists();
  void applyMidiSelections();
  void loadSyx();
  void exportSyx();
  void syncToneEditorFromSelection();
  void writeToneName();

  pg300::MidiEngine midi_;
  std::vector<pg300::PGParameter> params_ { pg300::makePG300Parameters() };
  std::unordered_map<uint8_t, uint8_t> currentValues_;

  std::optional<pg300::AlphaJunoBank> bank_;
  int selectedTone_ { -1 };

  // Top bar
  juce::Label outLabel_ { {}, "MIDI Out" };
  juce::ComboBox outCombo_;
  juce::Label inLabel_ { {}, "MIDI In" };
  juce::ComboBox inCombo_;
  juce::ToggleButton mergeToggle_ { "Merge/Thru" };
  juce::Label chLabel_ { {}, "Channel" };
  juce::Slider chSlider_;
  juce::TextButton manualButton_ { "MANUAL" };

  // Debug
  juce::Label debugLabel_ { {}, "Last SysEx:" };
  juce::TextEditor debugEditor_;

  // Librarian
  juce::GroupComponent librarianGroup_ { {}, "Librarian" };
  juce::TextButton loadSyxButton_ { "Load .SYX" };
  juce::TextButton exportSyxButton_ { "Export .SYX" };
  ToneListModel toneListModel_ { *this };
  juce::ListBox toneList_ { {}, &toneListModel_ };
  juce::Label toneNameLabel_ { {}, "Tone Name (max 10)" };
  juce::TextEditor toneNameEditor_;
  juce::TextButton writeNameButton_ { "Write name" };

  // Editor
  juce::GroupComponent editorGroup_ { {}, "Editor" };
  juce::Viewport editorViewport_;
  EditorSurface editorSurface_ { params_ };

  std::vector<juce::MidiDeviceInfo> outs_;
  std::vector<juce::MidiDeviceInfo> ins_;

  std::shared_ptr<juce::FileChooser> fileChooser_;
};

