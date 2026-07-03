# Whisper integration stub.
# Remove no_whisper from CONFIG when whisper.cpp is vendored.

WHISPER_ROOT = $$PWD/whisper.cpp

exists($$WHISPER_ROOT/whisper.h) | exists($$WHISPER_ROOT/include/whisper.h) {
    CONFIG -= no_whisper
    INCLUDEPATH += $$WHISPER_ROOT $$WHISPER_ROOT/include
    DEFINES += WHISPER_BUILD
    message("Whisper found at $$WHISPER_ROOT")
} else {
    message("Whisper not found — run: git submodule update --init third_party/whisper.cpp")
    message("  or: powershell -File scripts/fetch_deps.ps1 -Whisper")
}