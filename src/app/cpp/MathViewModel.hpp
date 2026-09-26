#pragma once

#include "core/math/Plotting.hpp"
#include "core/math/Reading.hpp"
#include "core/math/Solving.hpp"

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QtQmlIntegration>
#include <QtTypes>

#include <optional>

namespace phvikapen::app {

// Arithmetic and algebra as the window asks for them: a line of writing goes in, and an answer,
// the working that reached it, and a curve to draw come back. It knows nothing of notebooks, so
// the same answer can be asked for from a panel, a menu or a box of type.
class MathViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString said READ said WRITE setSaid NOTIFY askedChanged FINAL)
    Q_PROPERTY(State state READ state NOTIFY askedChanged FINAL)
    Q_PROPERTY(QString answer READ answer NOTIFY askedChanged FINAL)
    Q_PROPERTY(QString message READ message NOTIFY askedChanged FINAL)
    Q_PROPERTY(QVariantList working READ working NOTIFY askedChanged FINAL)
    Q_PROPERTY(QString letters READ letters NOTIFY askedChanged FINAL)
    Q_PROPERTY(QString letter READ letter NOTIFY askedChanged FINAL)
    Q_PROPERTY(bool drawable READ drawable NOTIFY askedChanged FINAL)
    Q_PROPERTY(QVariantList runs READ runs NOTIFY curveChanged FINAL)
    Q_PROPERTY(QVariantMap frame READ frame NOTIFY curveChanged FINAL)
    Q_PROPERTY(QString curveMessage READ curveMessage NOTIFY curveChanged FINAL)

public:
    // What became of the last thing asked.
    enum class State : quint8 {
        // Nothing has been asked yet.
        Nothing,
        // A sum with no letters in it, worked out.
        Answered,
        // A statement with a letter in it, solved.
        Solved,
        // A statement that holds whatever the letter stands for.
        Always,
        // A statement with two letters in it: there is no one answer, but there is a curve.
        Drawn,
        // Nothing could be made of what was asked.
        Refused,
    };
    Q_ENUM(State)

    // Why one line of the working was taken, named rather than written out, so that the window
    // says it in the language the reader asked for.
    enum class Reason : quint8 {
        Gathered,
        Divided,
        Formula,
        Reached,
    };
    Q_ENUM(Reason)

    // How finely a curve is worked out before the window says how wide it is.
    static constexpr int kDefaultSamples = 257;

    explicit MathViewModel(QObject* parent = nullptr);

    [[nodiscard]] QString said() const { return m_said; }

    void setSaid(const QString& said);

    [[nodiscard]] State state() const { return m_state; }

    [[nodiscard]] QString answer() const { return m_answer; }

    [[nodiscard]] QString message() const { return m_message; }

    [[nodiscard]] QVariantList working() const { return m_working; }

    [[nodiscard]] QString letters() const { return m_letters; }

    [[nodiscard]] QString letter() const { return m_letter; }

    [[nodiscard]] bool drawable() const { return m_drawable; }

    [[nodiscard]] QVariantList runs() const { return m_runs; }

    [[nodiscard]] QVariantMap frame() const;

    [[nodiscard]] QString curveMessage() const { return m_curveMessage; }

    // A line of writing worked out or solved, and the graph drawn again where it can be drawn.
    Q_INVOKABLE void ask(const QString& said);

    Q_INVOKABLE void forget();

    // The part of the graph being looked at, moved and made larger or smaller.
    Q_INVOKABLE void look(qreal left, qreal right, qreal bottom, qreal top);
    Q_INVOKABLE void moveBy(qreal across, qreal up);
    Q_INVOKABLE void zoomBy(qreal factor, qreal aboutAcross, qreal aboutUp);
    Q_INVOKABLE void resetFrame();

    // How many places across the graph are worked out. The window says how wide it is, so that a
    // narrow graph is not worked out more finely than it can show.
    Q_INVOKABLE void setSamples(int samples);

    // What an answer looks like where it goes back on the page: the line asked, an equals sign and
    // the answer, or the letter and what it stands for.
    Q_INVOKABLE [[nodiscard]] QString written() const;

    // Whether a line of writing is arithmetic that ends in an equals sign with nothing after it,
    // and what it comes to. Nothing comes back where it is not.
    Q_INVOKABLE [[nodiscard]] static QString quickAnswer(const QString& said);

signals:
    void askedChanged();
    void curveChanged();

private:
    void work();
    void redraw();
    void refuse(const QString& why);

    QString m_said;
    State m_state{State::Nothing};
    QString m_answer;
    QString m_message;
    QVariantList m_working;
    QString m_letters;
    QString m_letter;
    bool m_drawable{};
    QVariantList m_runs;
    QString m_curveMessage;
    core::Frame m_frame;
    int m_samples{kDefaultSamples};
    std::optional<core::Statement> m_statement;
};

}
