# PG-300 Controller (JUCE)

JUCE-based macOS (and cross-platform) desktop app that mimics a Roland PG-300 style editor for **Alpha Juno-2 / MKS-50**:

- **Realtime IPR SysEx** per parameter with throttle/coalesce.
- **.SYX bank librarian**: parse/rename/export tone names.
- **MIDI I/O**: select MIDI Out, optional MIDI In with merge/thru.

## Build (CMake)

```bash
cmake -S pg300-juce -B pg300-juce/build
cmake --build pg300-juce/build -j
ctest --test-dir pg300-juce/build --output-on-failure
```

Notes:
- JUCE is fetched via CMake `FetchContent` (internet required at configure time).

