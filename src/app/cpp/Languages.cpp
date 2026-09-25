#include "app/cpp/Languages.hpp"

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QQmlEngine>
#include <QString>
#include <QStringLiteral>
#include <QTranslator>
#include <QVariantList>
#include <QVariantMap>

#include <array>
#include <memory>
#include <utility>

namespace phvikapen::app {
namespace {

// One language on offer: the tag its catalogue is named by, and the name it calls itself, which is
// left as it is in every language so that a reader can find their own.
struct Offer {
    const char* tag;
    const char* name;
};

constexpr std::array kOffers{
    Offer{.tag = "en", .name = "English"},
    Offer{.tag = "hr", .name = "Hrvatski"},
};

constexpr auto kCatalogues = ":/i18n";
constexpr auto kBaseName = "phvikapen";

// The language of the machine, cut down to the part a catalogue is named by.
[[nodiscard]] QString machineLanguage() {
    return QLocale::system().name().section(QLatin1Char{'_'}, 0, 0);
}

}

Languages::Languages(QObject* parent) : QObject(parent) {
    listWhatIsOffered();
}

Languages::~Languages() {
    letGo();
}

void Languages::listWhatIsOffered() {
    QVariantList list;
    list.append(QVariantMap{
        {QStringLiteral("tag"), QString::fromLatin1(kFollowTheMachine)},
        {QStringLiteral("name"), tr("System language")},
    });
    for (const Offer& offer : kOffers) {
        list.append(QVariantMap{
            {QStringLiteral("tag"), QString::fromLatin1(offer.tag)},
            {QStringLiteral("name"), QString::fromLatin1(offer.name)},
        });
    }
    m_offered = std::move(list);
}

void Languages::speak(const QString& tag) {
    letGo();
    const QString wanted = tag.isEmpty() ? machineLanguage() : tag;
    static_cast<void>(takeUp(wanted));
    m_spoken = tag;
    listWhatIsOffered();
    emit spokenChanged();
    // Every word the window shows is read again. A binding on qsTr answers to this; anything the
    // window kept a copy of does not, which is why nothing keeps one.
    if (QQmlEngine* const engine = qmlEngine(this); engine != nullptr) {
        engine->retranslate();
    }
}

bool Languages::takeUp(const QString& tag) {
    // English is what the sources are written in, so it needs no catalogue of its own.
    if (tag.isEmpty() || tag == QStringLiteral("en")) {
        return true;
    }
    auto words = std::make_unique<QTranslator>();
    if (!words->load(QStringLiteral("%1_%2").arg(QString::fromLatin1(kBaseName), tag),
                     QString::fromLatin1(kCatalogues))) {
        return false;
    }
    if (!QCoreApplication::installTranslator(words.get())) {
        return false;
    }
    m_words = std::move(words);

    // What the toolkit says for itself, in the dialogs it opens. Where it was not shipped, the
    // application still speaks the language the reader asked for.
    auto toolkit = std::make_unique<QTranslator>();
    if (toolkit->load(QLocale{tag}, QStringLiteral("qtbase"), QStringLiteral("_"),
                      QLibraryInfo::path(QLibraryInfo::TranslationsPath))
        && QCoreApplication::installTranslator(toolkit.get())) {
        m_toolkit = std::move(toolkit);
    }
    return true;
}

void Languages::letGo() {
    for (std::unique_ptr<QTranslator>* held : {&m_words, &m_toolkit}) {
        if (*held) {
            QCoreApplication::removeTranslator(held->get());
            held->reset();
        }
    }
}

}
