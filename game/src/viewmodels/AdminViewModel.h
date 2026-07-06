#pragma once

#include <QHash>
#include <QImage>
#include <QObject>
#include <QVariantList>
#include <QString>

class DatabaseManager;
class ImageProcessor;
class PuzzleImageProvider;

class AdminViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList presets READ presets NOTIFY presetsChanged)
    Q_PROPERTY(QVariantList catalogRounds READ catalogRounds NOTIFY catalogRoundsChanged)
    Q_PROPERTY(QVariantList presetRoundIds READ presetRoundIds NOTIFY presetRoundIdsChanged)
    Q_PROPERTY(QVariantList puzzles READ puzzles NOTIFY puzzlesChanged)

    Q_PROPERTY(int selectedPresetId READ selectedPresetId WRITE setSelectedPresetId NOTIFY selectedPresetIdChanged)
    Q_PROPERTY(int selectedRoundId READ selectedRoundId WRITE setSelectedRoundId NOTIFY selectedRoundIdChanged)
    Q_PROPERTY(int selectedPuzzleId READ selectedPuzzleId NOTIFY selectedPuzzleIdChanged)

    Q_PROPERTY(QString selectedPresetName READ selectedPresetName NOTIFY selectedPresetIdChanged)
    Q_PROPERTY(QString editPresetName READ editPresetName WRITE setEditPresetName NOTIFY editPresetNameChanged)
    Q_PROPERTY(QString editAnswer READ editAnswer WRITE setEditAnswer NOTIFY editAnswerChanged)
    Q_PROPERTY(QString editHint READ editHint WRITE setEditHint NOTIFY editHintChanged)
    Q_PROPERTY(QString editQuotes READ editQuotes WRITE setEditQuotes NOTIFY editQuotesChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString previewImageUrl READ previewImageUrl NOTIFY previewImageChanged)
    Q_PROPERTY(QString puzzleImageUrl READ puzzleImageUrl NOTIFY puzzleImageChanged)
    Q_PROPERTY(int imageSlotCount READ imageSlotCount NOTIFY imageSlotsChanged)
    Q_PROPERTY(int selectedImageSlot READ selectedImageSlot WRITE setSelectedImageSlot NOTIFY selectedImageSlotChanged)
    Q_PROPERTY(bool showGamePreview READ showGamePreview NOTIFY previewImageChanged)
    Q_PROPERTY(bool hasPreviewImage READ hasPreviewImage NOTIFY previewImageChanged)
    Q_PROPERTY(bool hasMaskContour READ hasMaskContour NOTIFY maskContourChanged)
    Q_PROPERTY(QString maskContour READ maskContour NOTIFY maskContourChanged)
    Q_PROPERTY(QString selectedRoundLayoutType READ selectedRoundLayoutType NOTIFY selectedRoundIdChanged)
    Q_PROPERTY(bool isPhotoMaskRound READ isPhotoMaskRound NOTIFY selectedRoundIdChanged)
    Q_PROPERTY(bool imageProcessing READ imageProcessing NOTIFY imageProcessingChanged)
    Q_PROPERTY(bool maskProcessing READ maskProcessing NOTIFY maskProcessingChanged)

    Q_PROPERTY(bool roundConfigOpen READ roundConfigOpen NOTIFY roundConfigOpenChanged)
    Q_PROPERTY(QString configRoundTitle READ configRoundTitle NOTIFY roundConfigOpenChanged)
    Q_PROPERTY(bool configUsesImages READ configUsesImages NOTIFY roundConfigOpenChanged)
    Q_PROPERTY(bool configUsesTexts READ configUsesTexts NOTIFY roundConfigOpenChanged)
    Q_PROPERTY(int configImageSlotCount READ configImageSlotCount NOTIFY roundConfigOpenChanged)
    Q_PROPERTY(int configTextSlotCount READ configTextSlotCount NOTIFY roundConfigOpenChanged)
    Q_PROPERTY(int answerOptionCount READ answerOptionCount NOTIFY answerOptionsChanged)
    Q_PROPERTY(int maxAnswerOptions READ maxAnswerOptions CONSTANT)

public:
    enum RoundDataStatus {
        DataEmpty = 0,
        DataPartial = 1,
        DataComplete = 2
    };
    Q_ENUM(RoundDataStatus)

    explicit AdminViewModel(DatabaseManager *database, QObject *parent = nullptr);

    void setImageProcessor(ImageProcessor *processor);
    void setImageProvider(PuzzleImageProvider *provider);

    QVariantList presets() const { return m_presets; }
    QVariantList catalogRounds() const { return m_catalogRounds; }
    QVariantList presetRoundIds() const { return m_presetRoundIdsList; }
    QVariantList puzzles() const { return m_puzzles; }

    int selectedPresetId() const { return m_selectedPresetId; }
    int selectedRoundId() const { return m_selectedRoundId; }
    int selectedPuzzleId() const { return m_selectedPuzzleId; }

    QString selectedPresetName() const;
    QString editPresetName() const { return m_editPresetName; }
    QString editAnswer() const { return m_editAnswer; }
    QString editHint() const { return m_editHint; }
    QString editQuotes() const { return m_editQuotes; }
    QString statusMessage() const { return m_statusMessage; }
    QString previewImageUrl() const;
    QString puzzleImageUrl() const;
    int imageSlotCount() const;
    int selectedImageSlot() const { return m_selectedImageSlot; }
    bool showGamePreview() const;
    bool hasPreviewImage() const { return !m_sourceImage.isNull(); }
    bool hasMaskContour() const { return !m_maskContour.isEmpty(); }
    QString maskContour() const { return m_maskContour; }
    QString selectedRoundLayoutType() const { return m_selectedRoundLayoutType; }
    bool isPhotoMaskRound() const { return m_selectedRoundLayoutType == QStringLiteral("FULL_MASK"); }
    bool imageProcessing() const { return m_imageProcessing; }
    bool maskProcessing() const { return m_maskProcessing; }

    bool roundConfigOpen() const { return m_roundConfigOpen; }
    QString configRoundTitle() const { return m_configRoundTitle; }
    bool configUsesImages() const { return configImageSlotCount() > 0; }
    bool configUsesTexts() const { return configTextSlotCount() > 0; }
    int configImageSlotCount() const;
    int configTextSlotCount() const;
    int answerOptionCount() const { return m_answerOptions.size(); }
    int maxAnswerOptions() const { return 10; }

    void setSelectedPresetId(int presetId);
    void setSelectedRoundId(int roundId);
    void setEditPresetName(const QString &name);
    void setEditAnswer(const QString &answer);
    void setEditHint(const QString &hint);
    void setEditQuotes(const QString &quotes);
    void setSelectedImageSlot(int slot);

    Q_INVOKABLE void refreshPresets();
    Q_INVOKABLE void refreshCatalogRounds();
    Q_INVOKABLE void refreshPresetRounds();
    Q_INVOKABLE void refreshPuzzles();
    Q_INVOKABLE void createPreset();
    Q_INVOKABLE void savePresetMeta();
    Q_INVOKABLE void deleteSelectedPreset();
    Q_INVOKABLE void deletePreset(int presetId);
    Q_INVOKABLE int roundTemplateStatus(int roundId) const;
    Q_INVOKABLE void togglePresetRound(int roundId, bool enabled);
    Q_INVOKABLE void savePresetRounds();
    Q_INVOKABLE bool isRoundEnabled(int roundId) const;
    Q_INVOKABLE void setRoundEnabled(int roundId, bool enabled);
    Q_INVOKABLE void openRoundConfig(int roundId);
    Q_INVOKABLE void closeRoundConfig();
    Q_INVOKABLE void saveRoundConfig();
    Q_INVOKABLE void addAnswerOption();
    Q_INVOKABLE QString cardTextAt(int index) const;
    Q_INVOKABLE void setCardTextAt(int index, const QString &text);
    Q_INVOKABLE QString answerOptionAt(int index) const;
    Q_INVOKABLE void setAnswerOptionAt(int index, const QString &text);
    Q_INVOKABLE QString cardTextPlaceholder(int index) const;
    Q_INVOKABLE QString answerOptionPlaceholder() const;
    Q_INVOKABLE void selectPuzzle(int puzzleId);
    Q_INVOKABLE void createPuzzle();
    Q_INVOKABLE void savePuzzle();
    Q_INVOKABLE void deleteSelectedPuzzle();
    Q_INVOKABLE bool importPuzzleImage(const QUrl &fileUrl);
    Q_INVOKABLE bool markMissingArea(double relX, double relY);
    Q_INVOKABLE bool markMissingRegion(double relX, double relY, double relW, double relH);
    Q_INVOKABLE bool slotHasImage(int slotIndex) const;
    Q_INVOKABLE QString slotThumbnailUrl(int slotIndex) const;
    Q_INVOKABLE void focusPhotoMaskRound();
    Q_INVOKABLE void startPhotoPuzzle();
    Q_INVOKABLE void clearMask();
    Q_INVOKABLE QString label(const QString &key) const;
    Q_INVOKABLE bool selectPresetByName(const QString &name);

signals:
    void presetsChanged();
    void catalogRoundsChanged();
    void presetRoundIdsChanged();
    void puzzlesChanged();
    void selectedPresetIdChanged(int presetId);
    void selectedRoundIdChanged(int roundId);
    void selectedPuzzleIdChanged();
    void editPresetNameChanged();
    void editAnswerChanged();
    void editHintChanged();
    void editQuotesChanged();
    void statusMessageChanged();
    void previewImageChanged();
    void puzzleImageChanged();
    void maskContourChanged();
    void imageSlotsChanged();
    void selectedImageSlotChanged();
    void imageProcessingChanged();
    void maskProcessingChanged();
    void roundConfigOpenChanged();
    void answerOptionsChanged();
    void cardTextsChanged();

private:
    void rebuildPresetList();
    void rebuildCatalogRounds();
    void rebuildPresetRoundIds();
    void rebuildPuzzleList();
    void loadPuzzleEditor(int puzzleId);
    void loadImageSlot(int slotIndex);
    void cacheCurrentSlotState();
    void refreshEditorPreview();
    void clearPuzzleEditor();
    void setStatusMessage(const QString &message);
    void updatePreviewProvider();
    void bumpPreviewRevision();
    void setImageProcessingBusy(bool busy);
    QString quoteSlotsJsonFromEditor() const;
    QString answerOptionsJsonFromEditor() const;
    void ensureConfigCardTextSize();
    static int imageSlotCountForLayout(const QString &layoutType);
    static int textSlotCountForLayout(const QString &layoutType);
    int evaluateRoundTemplateStatus(int roundId) const;

    DatabaseManager *m_database = nullptr;
    ImageProcessor *m_imageProcessor = nullptr;
    PuzzleImageProvider *m_imageProvider = nullptr;

    QVariantList m_presets;
    QVariantList m_catalogRounds;
    QVariantList m_presetRoundIdsList;
    QVariantList m_puzzles;

    int m_selectedPresetId = 0;
    int m_selectedRoundId = 0;
    int m_selectedPuzzleId = 0;
    int m_selectedTemplateId = 0;
    int m_selectedImageSlot = 0;
    int m_previewRevision = 0;
    int m_puzzleImageRevision = 0;
    int m_previewJobId = 0;
    int m_regionJobId = 0;
    int m_processingJobs = 0;
    bool m_imageProcessing = false;
    bool m_maskProcessing = false;

    QString m_editPresetName;
    QString m_editAnswer;
    QString m_editHint;
    QString m_editQuotes;
    QString m_statusMessage;
    QString m_maskContour;
    QString m_selectedRoundLayoutType;
    bool m_roundConfigOpen = false;
    QString m_configRoundTitle;
    QStringList m_configCardTexts;
    QStringList m_answerOptions;
    QImage m_sourceImage;
    QImage m_previewImage;
    QByteArray m_pendingImageBytes;
    QHash<int, QImage> m_slotImageCache;
    QHash<int, QByteArray> m_slotPendingBytes;
};
