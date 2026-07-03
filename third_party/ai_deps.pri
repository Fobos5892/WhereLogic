# Optional AI runtime dependencies (models, download helpers).
# Extend this file when wiring automatic model fetch.

AI_MANIFEST = $$PWD/../config/ai_manifest.json
exists($$AI_MANIFEST) {
    message("AI manifest: $$AI_MANIFEST")
}
