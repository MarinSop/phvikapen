#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQmlIntegration>

#include <memory>

class QTranslator;

namespace phvikapen::app {

// The languages the application was built with, and the one it is speaking. Asking for another one
// puts its catalogue in place and tells the window to read every word again, so the change shows at
// once rather than at the next start.
//
// Adding a language is one more catalogue beside the sources and one more line here. Nothing a
// reader sees is written down twice.
class Languages : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList offered READ offered NOTIFY spokenChanged FINAL)
    Q_PROPERTY(QString spoken READ spoken NOTIFY spokenChanged FINAL)

public:
    // What a language is called where the reader asked for none: the one the machine is set to.
    static constexpr auto kFollowTheMachine = "";

    explicit Languages(QObject* parent = nullptr);
    ~Languages() override;

    Languages(const Languages&) = delete;
    Languages& operator=(const Languages&) = delete;
    Languages(Languages&&) = delete;
    Languages& operator=(Languages&&) = delete;

    // Every language on offer, each with the tag it is asked for by and the name it calls itself.
    // The one that follows the machine is named in whatever language is being spoken, so the list
    // is made again whenever that changes.
    [[nodiscard]] QVariantList offered() const { return m_offered; }

    [[nodiscard]] QString spoken() const { return m_spoken; }

    // Speak a language, named by its tag. Nothing at all follows the machine.
    Q_INVOKABLE void speak(const QString& tag);

signals:
    void spokenChanged();

private:
    [[nodiscard]] bool takeUp(const QString& tag);

    void letGo();

    void listWhatIsOffered();

    QString m_spoken;
    QVariantList m_offered;
    std::unique_ptr<QTranslator> m_words;
    std::unique_ptr<QTranslator> m_toolkit;
};

}
