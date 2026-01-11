#include "MainComponent.h"

#include <cstring>

// =========================
// ToneListModel
// =========================

int MainComponent::ToneListModel::getNumRows()
{
  if (!owner_.bank_)
    return 0;
  return owner_.bank_->toneCount();
}

void MainComponent::ToneListModel::paintListBoxItem(int rowNumber,
                                                    juce::Graphics& g,
                                                    int width,
                                                    int height,
                                                    bool rowIsSelected)
{
  if (rowIsSelected)
    g.fillAll(juce::Colour(0xFF2A2C30));

  g.setColour(juce::Colours::white.withAlpha(rowIsSelected ? 0.95f : 0.8f));
  g.setFont(juce::Font(13.0f));

  const int idx = rowNumber;
  const juce::String prefix = juce::String(idx + 1).paddedLeft('0', 2) + "  ";

  juce::String name = "(empty)";
  if (owner_.bank_)
    name = owner_.bank_->getToneName(idx);

  g.drawText(prefix + name, 6, 0, width - 12, height, juce::Justification::centredLeft);
}

void MainComponent::ToneListModel::selectedRowsChanged(int lastRowSelected)
{
  owner_.selectedTone_ = lastRowSelected;
  owner_.syncToneEditorFromSelection();
}

// =========================
// EditorSurface
// =========================

juce::String MainComponent::EditorSurface::sectionForParam(uint8_t paramId)
{
  // 0x00..0x23 inclusive
  switch (paramId)
  {
    // DCO-ish
    case 0x00:
    case 0x03: case 0x04: case 0x05:
    case 0x06: case 0x07: case 0x08:
    case 0x0B: case 0x0C: case 0x0D: case 0x0E: case 0x0F:
      return "DCO";

    // VCF-ish (+ HPF)
    case 0x01:
    case 0x09:
    case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
      return "VCF";

    // VCA
    case 0x02:
    case 0x16: case 0x17:
      return "VCA";

    // LFO
    case 0x18: case 0x19:
      return "LFO";

    // ENV
    case 0x1A: case 0x1B: case 0x1C: case 0x1D:
    case 0x1E: case 0x1F: case 0x20: case 0x21:
      return "ENV";

    // Chorus
    case 0x0A:
    case 0x22:
      return "CHORUS";

    // Misc
    case 0x23:
      return "MISC";
  }

  return "MISC";
}

int MainComponent::EditorSurface::columnsForSection(const juce::String& section)
{
  if (section == "ENV") return 4;
  if (section == "DCO") return 4;
  if (section == "VCF") return 3;
  if (section == "VCA") return 3;
  if (section == "LFO") return 2;
  if (section == "CHORUS") return 2;
  return 2;
}

MainComponent::EditorSurface::EditorSurface(const std::vector<pg300::PGParameter>& params)
{
  for (auto* g : { &dcoGroup_, &vcfGroup_, &vcaGroup_, &lfoGroup_, &envGroup_, &chorusGroup_, &miscGroup_ })
    addAndMakeVisible(g);

  for (const auto& p : params)
  {
    auto* c = allControls_.add(new pg300::ParameterControl(p));
    c->onValueChanged = [this](uint8_t paramId, uint8_t value) {
      if (onParamChanged)
        onParamChanged(paramId, value);
    };
    addAndMakeVisible(c);
    controlsById_[p.id] = c;

    const auto sec = sectionForParam(p.id);
    if (sec == "DCO") dcoControls_.push_back(c);
    else if (sec == "VCF") vcfControls_.push_back(c);
    else if (sec == "VCA") vcaControls_.push_back(c);
    else if (sec == "LFO") lfoControls_.push_back(c);
    else if (sec == "ENV") envControls_.push_back(c);
    else if (sec == "CHORUS") chorusControls_.push_back(c);
    else miscControls_.push_back(c);
  }
}

void MainComponent::EditorSurface::setValue(uint8_t paramId, uint8_t value, juce::NotificationType nt)
{
  if (auto it = controlsById_.find(paramId); it != controlsById_.end() && it->second != nullptr)
    it->second->setValue(value, nt);
}

uint8_t MainComponent::EditorSurface::getValue(uint8_t paramId) const
{
  if (auto it = controlsById_.find(paramId); it != controlsById_.end() && it->second != nullptr)
    return it->second->value();
  return 0;
}

void MainComponent::EditorSurface::layoutSection(juce::GroupComponent& group,
                                                 const std::vector<pg300::ParameterControl*>& controls,
                                                 int columns)
{
  auto inner = group.getBounds().reduced(10);
  inner.removeFromTop(24); // group title space

  if (controls.empty() || inner.isEmpty())
    return;

  const int cellW = std::max(80, inner.getWidth() / std::max(1, columns));
  const int cellH = 110;

  int x = inner.getX();
  int y = inner.getY();
  int col = 0;

  for (auto* c : controls)
  {
    c->setBounds(x, y, cellW, cellH);
    ++col;
    x += cellW;
    if (col >= columns)
    {
      col = 0;
      x = inner.getX();
      y += cellH;
    }
  }
}

void MainComponent::EditorSurface::resized()
{
  auto r = getLocalBounds().reduced(10);

  // Two-column synth-style layout.
  auto left = r.removeFromLeft((int) (r.getWidth() * 0.58));
  auto right = r;

  auto leftTop = left.removeFromTop((int) (left.getHeight() * 0.40));
  dcoGroup_.setBounds(leftTop.reduced(4));

  auto leftMid = left.removeFromTop((int) (left.getHeight() * 0.55));
  vcfGroup_.setBounds(leftMid.reduced(4));

  vcaGroup_.setBounds(left.reduced(4));

  auto rightTop = right.removeFromTop((int) (right.getHeight() * 0.52));
  envGroup_.setBounds(rightTop.reduced(4));

  auto rightMid = right.removeFromTop((int) (right.getHeight() * 0.45));
  lfoGroup_.setBounds(rightMid.reduced(4));

  auto rightLower = right;
  auto chorusArea = rightLower.removeFromTop((int) (rightLower.getHeight() * 0.55));
  chorusGroup_.setBounds(chorusArea.reduced(4));
  miscGroup_.setBounds(rightLower.reduced(4));

  layoutSection(dcoGroup_, dcoControls_, columnsForSection("DCO"));
  layoutSection(vcfGroup_, vcfControls_, columnsForSection("VCF"));
  layoutSection(vcaGroup_, vcaControls_, columnsForSection("VCA"));
  layoutSection(lfoGroup_, lfoControls_, columnsForSection("LFO"));
  layoutSection(envGroup_, envControls_, columnsForSection("ENV"));
  layoutSection(chorusGroup_, chorusControls_, columnsForSection("CHORUS"));
  layoutSection(miscGroup_, miscControls_, columnsForSection("MISC"));
}

MainComponent::MainComponent()
{
  outCombo_.onChange = [this] { applyMidiSelections(); };
  inCombo_.onChange = [this] { applyMidiSelections(); };

  mergeToggle_.onClick = [this] { midi_.setMergeEnabled(mergeToggle_.getToggleState()); };

  chSlider_.setRange(1, 16, 1);
  chSlider_.setSliderStyle(juce::Slider::IncDecButtons);
  chSlider_.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 24);
  chSlider_.onValueChange = [this] { midi_.setMidiChannel((int) chSlider_.getValue()); };

  manualButton_.onClick = [this] {
    // MANUAL: send all 36 parameters according to current UI state.
    for (const auto& p : params_)
      midi_.sendIPRNow(p.id, currentValues_[p.id]);
  };

  debugEditor_.setMultiLine(true);
  debugEditor_.setReadOnly(true);
  debugEditor_.setScrollbarsShown(true);

  midi_.onSysExSent = [this](const std::vector<uint8_t>& bytes) {
    juce::String s;
    for (size_t i = 0; i < bytes.size(); ++i)
    {
      s += juce::String::toHexString((int) bytes[i]).paddedLeft('0', 2).toUpperCase();
      if (i + 1 < bytes.size())
        s += " ";
    }
    juce::MessageManager::callAsync([this, s] { debugEditor_.setText(s, false); });
  };

  // Librarian
  loadSyxButton_.onClick = [this] { loadSyx(); };
  exportSyxButton_.onClick = [this] { exportSyx(); };
  exportSyxButton_.setEnabled(false);

  toneList_.setRowHeight(22);
  toneNameEditor_.setInputRestrictions(10);
  writeNameButton_.onClick = [this] { writeToneName(); };

  // Editor
  editorViewport_.setViewedComponent(&editorSurface_, false);
  editorSurface_.onParamChanged = [this](uint8_t paramId, uint8_t value) {
    currentValues_[paramId] = value;
    midi_.queueIPR(paramId, value);
  };

  // Initialize defaults (0) for all parameters.
  for (const auto& p : params_)
    currentValues_[p.id] = 0;

  juce::Component* components[] = {
    static_cast<juce::Component*>(&outLabel_),
    static_cast<juce::Component*>(&outCombo_),
    static_cast<juce::Component*>(&inLabel_),
    static_cast<juce::Component*>(&inCombo_),
    static_cast<juce::Component*>(&mergeToggle_),
    static_cast<juce::Component*>(&chLabel_),
    static_cast<juce::Component*>(&chSlider_),
    static_cast<juce::Component*>(&manualButton_),
    static_cast<juce::Component*>(&debugLabel_),
    static_cast<juce::Component*>(&debugEditor_),
    static_cast<juce::Component*>(&librarianGroup_),
    static_cast<juce::Component*>(&loadSyxButton_),
    static_cast<juce::Component*>(&exportSyxButton_),
    static_cast<juce::Component*>(&toneList_),
    static_cast<juce::Component*>(&toneNameLabel_),
    static_cast<juce::Component*>(&toneNameEditor_),
    static_cast<juce::Component*>(&writeNameButton_),
    static_cast<juce::Component*>(&editorGroup_),
    static_cast<juce::Component*>(&editorViewport_),
  };
  for (auto* c : components)
    addAndMakeVisible(c);

  refreshMidiDeviceLists();
  applyMidiSelections();
}

MainComponent::~MainComponent() = default;

void MainComponent::refreshMidiDeviceLists()
{
  auto devs = midi_.listDevices();
  outs_ = devs.outputs;
  ins_ = devs.inputs;

  outCombo_.clear(juce::dontSendNotification);
  for (int i = 0; i < (int) outs_.size(); ++i)
    outCombo_.addItem(outs_[(size_t) i].name, i + 1);

  inCombo_.clear(juce::dontSendNotification);
  inCombo_.addItem("(None)", 1);
  for (int i = 0; i < (int) ins_.size(); ++i)
    inCombo_.addItem(ins_[(size_t) i].name, i + 2);

  outCombo_.setSelectedId(outs_.empty() ? 0 : 1, juce::dontSendNotification);
  inCombo_.setSelectedId(1, juce::dontSendNotification);
}

void MainComponent::applyMidiSelections()
{
  const int outId = outCombo_.getSelectedId();
  if (outId >= 1 && outId <= (int) outs_.size())
    midi_.openOutput(outs_[(size_t) (outId - 1)].identifier);
  else
    midi_.closeOutput();

  const int inId = inCombo_.getSelectedId();
  if (inId <= 1)
    midi_.closeInput();
  else
  {
    const int idx = inId - 2;
    if (idx >= 0 && idx < (int) ins_.size())
      midi_.openInput(ins_[(size_t) idx].identifier);
  }
}

void MainComponent::loadSyx()
{
  fileChooser_.reset(new juce::FileChooser("Load Alpha Juno bank (.SYX)", {}, "*.syx;*.SYX"));
  fileChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                            [this](const juce::FileChooser& fc) {
    const auto f = fc.getResult();
    if (!f.existsAsFile())
      return;

    juce::MemoryBlock mb;
    if (!f.loadFileAsData(mb))
    {
      juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Load failed", "Could not read file.");
      return;
    }

    std::vector<uint8_t> bytes(mb.getSize());
    std::memcpy(bytes.data(), mb.getData(), mb.getSize());

    try
    {
      bank_ = pg300::AlphaJunoBank::parse(bytes);
      exportSyxButton_.setEnabled(true);
      toneList_.updateContent();
      toneList_.selectRow(0);
    }
    catch (const std::exception& e)
    {
      bank_.reset();
      exportSyxButton_.setEnabled(false);
      toneList_.updateContent();
      toneNameEditor_.setText({}, juce::dontSendNotification);
      juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Invalid SYX", e.what());
    }
  });
}

void MainComponent::exportSyx()
{
  if (!bank_)
    return;

  fileChooser_.reset(new juce::FileChooser("Export bank (.SYX)", {}, "*.syx"));
  fileChooser_->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                            [this](const juce::FileChooser& fc) {
    if (!bank_)
      return;
    auto f = fc.getResult();
    if (f.getFileExtension().isEmpty())
      f = f.withFileExtension(".syx");

    const auto bytes = bank_->toSyxBytes();
    if (!f.replaceWithData(bytes.data(), (int) bytes.size()))
      juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Export failed", "Could not write file.");
  });
}

void MainComponent::syncToneEditorFromSelection()
{
  if (!bank_ || selectedTone_ < 0 || selectedTone_ >= bank_->toneCount())
  {
    toneNameEditor_.setText({}, juce::dontSendNotification);
    return;
  }
  toneNameEditor_.setText(bank_->getToneName(selectedTone_), juce::dontSendNotification);
}

void MainComponent::writeToneName()
{
  if (!bank_ || selectedTone_ < 0 || selectedTone_ >= bank_->toneCount())
    return;

  const auto name = toneNameEditor_.getText().toStdString();
  try
  {
    bank_->setToneName(selectedTone_, name);
    toneList_.repaintRow(selectedTone_);
  }
  catch (const std::exception& e)
  {
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Rename failed", e.what());
  }
}

void MainComponent::paint(juce::Graphics& g)
{
  g.fillAll(juce::Colour(0xFF1B1B1D));
  g.setColour(juce::Colours::white.withAlpha(0.06f));
  g.drawRect(getLocalBounds(), 1);
}

void MainComponent::resized()
{
  auto r = getLocalBounds().reduced(12);

  auto top = r.removeFromTop(44);
  outLabel_.setBounds(top.removeFromLeft(70));
  outCombo_.setBounds(top.removeFromLeft(220));
  top.removeFromLeft(12);
  inLabel_.setBounds(top.removeFromLeft(60));
  inCombo_.setBounds(top.removeFromLeft(220));
  top.removeFromLeft(12);
  mergeToggle_.setBounds(top.removeFromLeft(120));
  top.removeFromLeft(12);
  chLabel_.setBounds(top.removeFromLeft(70));
  chSlider_.setBounds(top.removeFromLeft(120));
  top.removeFromLeft(12);
  manualButton_.setBounds(top.removeFromLeft(100));

  r.removeFromTop(12);

  auto dbg = r.removeFromBottom(120);
  debugLabel_.setBounds(dbg.removeFromTop(20));
  debugEditor_.setBounds(dbg);

  r.removeFromBottom(12);

  // Middle: librarian (left) + editor (right)
  auto left = r.removeFromLeft(340);
  librarianGroup_.setBounds(left);

  auto libInner = left.reduced(12);
  libInner.removeFromTop(20); // group title
  auto libButtons = libInner.removeFromTop(28);
  loadSyxButton_.setBounds(libButtons.removeFromLeft(140));
  libButtons.removeFromLeft(8);
  exportSyxButton_.setBounds(libButtons.removeFromLeft(140));
  libInner.removeFromTop(10);

  auto nameArea = libInner.removeFromBottom(74);
  toneNameLabel_.setBounds(nameArea.removeFromTop(18));
  toneNameEditor_.setBounds(nameArea.removeFromTop(26));
  nameArea.removeFromTop(6);
  writeNameButton_.setBounds(nameArea.removeFromTop(24));

  libInner.removeFromBottom(8);
  toneList_.setBounds(libInner);

  auto right = r;
  editorGroup_.setBounds(right);

  auto editorInner = right.reduced(12);
  editorInner.removeFromTop(20); // group title
  editorViewport_.setBounds(editorInner);

  // Make sure viewed component has enough size for viewport scrolling.
  editorSurface_.setSize(editorInner.getWidth(), 900);
}

