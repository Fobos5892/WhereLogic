#pragma once

#include <QObject>

class QAudioOutput;

class AudioSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(qreal volumeNormalized READ volumeNormalized NOTIFY volumeChanged)

public:
    explicit AudioSettings(QObject *parent = nullptr);

    int volume() const { return m_volume; }
    qreal volumeNormalized() const;

    void setVolume(int volume);
    void applyTo(QAudioOutput *output) const;

signals:
    void volumeChanged();

private:
    void load();
    void save();

    int m_volume = 80;
};
