#include "VoiceRecognizer.h"

#include "TriggerParser.h"

VoiceRecognizer::VoiceRecognizer(QObject *parent)
    : QObject(parent)
{
}

bool VoiceRecognizer::isAvailable() const
{
#ifdef HAS_WHISPER
    return true;
#else
    return false;
#endif
}

void VoiceRecognizer::startListening()
{
#ifndef HAS_WHISPER
    emit recognitionFailed(QStringLiteral("Whisper is not available"));
    return;
#else
    setListening(true);
#endif
}

void VoiceRecognizer::stopListening()
{
    setListening(false);
}

QString VoiceRecognizer::parseTriggerFromText(const QString &transcript) const
{
    const TriggerParseResult result = TriggerParser::parse(transcript);
    return result.triggered ? result.answer : QString();
}

void VoiceRecognizer::setListening(bool listening)
{
    if (m_listening == listening) {
        return;
    }
    m_listening = listening;
    emit listeningChanged(m_listening);
}
