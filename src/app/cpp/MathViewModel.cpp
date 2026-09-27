#include "app/cpp/MathViewModel.hpp"

#include "core/Error.hpp"
#include "core/math/Answer.hpp"
#include "core/math/Equation.hpp"
#include "core/math/Plotting.hpp"
#include "core/math/Reading.hpp"
#include "core/math/Solving.hpp"

#include <QPointF>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace phvikapen::app {
namespace {

constexpr int kFewestSamples = 33;
constexpr qreal kSmallestFrame = 1e-6;
constexpr qreal kLargestFrame = 1e9;

[[nodiscard]] MathViewModel::Reason reasonOf(core::Working::Reason reason) noexcept {
    switch (reason) {
    case core::Working::Reason::Divided:
        return MathViewModel::Reason::Divided;
    case core::Working::Reason::Formula:
        return MathViewModel::Reason::Formula;
    case core::Working::Reason::Searched:
        return MathViewModel::Reason::Searched;
    case core::Working::Reason::Answered:
        return MathViewModel::Reason::Reached;
    default:
        return MathViewModel::Reason::Gathered;
    }
}

[[nodiscard]] QString answersOf(const core::Solution& solution) {
    QString said;
    for (const double answer : solution.answers) {
        if (!said.isEmpty()) {
            said += QStringLiteral(", ");
        }
        said += QString::fromStdString(std::string{solution.letter}) + QStringLiteral(" = ")
                + QString::fromStdString(core::writtenAnswer(answer));
    }
    return said;
}

}

MathViewModel::MathViewModel(QObject* parent) : QObject(parent) {}

void MathViewModel::setSaid(const QString& said) {
    if (said == m_said) {
        return;
    }
    ask(said);
}

void MathViewModel::setSamples(int samples) {
    const int wanted = std::clamp(samples, kFewestSamples, core::kMostSamples);
    if (wanted == m_samples) {
        return;
    }
    m_samples = wanted;
    redraw();
}

QVariantMap MathViewModel::frame() const {
    return QVariantMap{
        {QStringLiteral("left"), m_frame.left},
        {QStringLiteral("right"), m_frame.right},
        {QStringLiteral("bottom"), m_frame.bottom},
        {QStringLiteral("top"), m_frame.top},
    };
}

void MathViewModel::forget() {
    m_said.clear();
    m_statement.reset();
    m_state = State::Nothing;
    m_answer.clear();
    m_message.clear();
    m_working.clear();
    m_letters.clear();
    m_letter.clear();
    m_drawable = false;
    m_runs.clear();
    m_curveMessage.clear();
    emit askedChanged();
    emit curveChanged();
}

void MathViewModel::refuse(const QString& why) {
    m_state = State::Refused;
    m_answer.clear();
    m_working.clear();
    m_message = why;
    m_drawable = false;
    m_runs.clear();
    m_curveMessage.clear();
    emit askedChanged();
    emit curveChanged();
}

void MathViewModel::ask(const QString& said) {
    m_said = said;
    m_letter.clear();
    work();
}

void MathViewModel::work() {
    const QString asked = m_said.trimmed();
    if (asked.isEmpty()) {
        forget();
        return;
    }
    const core::Result<core::Statement> statement = core::statementOf(asked.toStdString());
    if (!statement) {
        m_statement.reset();
        m_letters.clear();
        refuse(QString::fromStdString(statement.error().message));
        return;
    }
    m_statement = *statement;
    m_letters = QString::fromStdString(core::lettersOf(*statement));
    m_message.clear();

    if (m_letters.isEmpty()) {
        const core::Result<double> answer = core::answerWith(statement->left, {});
        if (!answer) {
            refuse(QString::fromStdString(answer.error().message));
            return;
        }
        m_state = State::Answered;
        m_answer = QString::fromStdString(core::writtenAnswer(*answer));
        m_working.clear();
        m_drawable = false;
        m_runs.clear();
        m_curveMessage.clear();
        emit askedChanged();
        emit curveChanged();
        return;
    }

    // A statement with more than one letter in it has no one answer, but it does have a curve, and
    // that is what the reader asked for by writing it.
    if (m_letters.size() > 1) {
        m_state = State::Drawn;
        m_answer.clear();
        m_working.clear();
        m_letter = m_letters.left(1);
        emit askedChanged();
        redraw();
        return;
    }

    m_letter = m_letters.left(1);
    const core::Result<core::Solution> solution =
        core::solvedFor(*statement, m_letter.front().toLatin1(), {});
    if (!solution) {
        m_state = State::Refused;
        m_answer.clear();
        m_working.clear();
        m_message = QString::fromStdString(solution.error().message);
    } else {
        m_state = solution->always ? State::Always : State::Solved;
        m_answer = answersOf(*solution);
        m_working.clear();
        for (const core::Working& line : solution->working) {
            m_working.append(QVariantMap{
                {QStringLiteral("reason"), static_cast<int>(reasonOf(line.reason))},
                {QStringLiteral("said"), QString::fromStdString(line.said)},
                {QStringLiteral("number"), line.number},
            });
        }
    }
    emit askedChanged();
    redraw();
}

void MathViewModel::redraw() {
    m_runs.clear();
    m_curveMessage.clear();
    m_drawable = false;
    if (!m_statement) {
        emit askedChanged();
        emit curveChanged();
        return;
    }
    const core::Result<std::pair<char, char>> axes = core::axesOf(*m_statement);
    if (!axes) {
        m_curveMessage = QString::fromStdString(axes.error().message);
        emit askedChanged();
        emit curveChanged();
        return;
    }
    m_drawable = true;
    const core::Result<core::Curve> curve =
        core::plotted(*m_statement, axes->first, axes->second, m_frame, m_samples);
    if (!curve) {
        m_curveMessage = QString::fromStdString(curve.error().message);
        emit askedChanged();
        emit curveChanged();
        return;
    }
    for (const auto& run : curve->runs) {
        QVariantList spots;
        spots.reserve(static_cast<qsizetype>(run.size()));
        for (const core::Spot& spot : run) {
            spots.append(QPointF{spot.across, spot.up});
        }
        m_runs.append(QVariant::fromValue(spots));
    }
    emit askedChanged();
    emit curveChanged();
}

void MathViewModel::look(qreal left, qreal right, qreal bottom, qreal top) {
    const core::Frame wanted{
        .left = left,
        .right = right,
        .bottom = bottom,
        .top = top,
    };
    if (!core::isDrawable(wanted) || core::widthOf(wanted) < kSmallestFrame
        || core::heightOf(wanted) < kSmallestFrame || core::widthOf(wanted) > kLargestFrame
        || core::heightOf(wanted) > kLargestFrame) {
        return;
    }
    m_frame = wanted;
    redraw();
}

void MathViewModel::moveBy(qreal across, qreal up) {
    look(m_frame.left + across, m_frame.right + across, m_frame.bottom + up, m_frame.top + up);
}

void MathViewModel::zoomBy(qreal factor, qreal aboutAcross, qreal aboutUp) {
    if (!std::isfinite(factor) || factor <= 0.0) {
        return;
    }
    look(aboutAcross + ((m_frame.left - aboutAcross) * factor),
         aboutAcross + ((m_frame.right - aboutAcross) * factor),
         aboutUp + ((m_frame.bottom - aboutUp) * factor),
         aboutUp + ((m_frame.top - aboutUp) * factor));
}

void MathViewModel::resetFrame() {
    m_frame = core::Frame{};
    redraw();
}

QString MathViewModel::written() const {
    switch (m_state) {
    case State::Answered:
        return m_said.trimmed() + QStringLiteral(" = ") + m_answer;
    case State::Solved:
        return m_answer;
    case State::Always:
    case State::Drawn:
        return m_said.trimmed();
    default:
        return {};
    }
}

QString MathViewModel::quickAnswer(const QString& said) {
    const QString asked = said.trimmed();
    if (!asked.endsWith(QLatin1Char{'='}) || asked.size() < 2) {
        return {};
    }
    const QString sum = asked.chopped(1).trimmed();
    // A line with no sign of arithmetic in it is writing, not a sum, whatever it ends with.
    static const QString kSigns = QStringLiteral("+-*/^√");
    if (sum.isEmpty()
        || std::ranges::none_of(kSigns, [&sum](QChar sign) { return sum.contains(sign); })) {
        return {};
    }
    const core::Result<core::Equation> equation = core::equationOf(sum.toStdString());
    if (!equation || !core::lettersOf(*equation).empty()) {
        return {};
    }
    const core::Result<double> answer = core::answerOf(*equation);
    if (!answer) {
        return {};
    }
    return QString::fromStdString(core::writtenAnswer(*answer));
}

}
