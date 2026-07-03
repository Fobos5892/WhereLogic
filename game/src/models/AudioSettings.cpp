#include "AudioSettings.h"

#include <QAudioOutput>
#include <QSettings>

namespace {

constexpr int kDefaultVolume = 80;
constexpr int kMinVolume = 0;
constexpr int kMaxVolume = 100;
constexpr char kVolumeKey[] = "audio/volume";

} // namespace

AudioSettings::AudioSettings(QObject *parent)
    : QObject(parent)
{
    load();
}

qreal AudioSettings::volumeNormalized() const
{
    return qreal(m_volume) / qreal(kMaxVolume);
}

void AudioSettings::setVolume(int volume)
{
    const int clamped = qBound(kMinVolume, volume, kMaxVolume);
    if (m_volume == clamped) {
        return;
    }
    m_volume = clamped;
    save();
    emit volumeChanged();
}

void AudioSettings::applyTo(QAudioOutput *output) const
{
    if (!output) {
        return;
    }
    output->setVolume(volumeNormalized());
}

void AudioSettings::load()
{
    QSettings settings;
    m_volume = settings.value(QString::fromLatin1(kVolumeKey), kDefaultVolume).toInt();
    m_volume = qBound(kMinVolume, m_volume, kMaxVolume);
}

void AudioSettings::save()
{
    QSettings settings;
    settings.setValue(QString::fromLatin1(kVolumeKey), m_volume);
}
