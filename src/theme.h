#pragma once
#include <QObject>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QVariantMap>
#include <QRegularExpression>
class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap colors READ colors NOTIFY changed)
public:
    Theme() {
        reload();
        auto timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &Theme::reload); timer->start(2000);
    }
    QVariantMap colors() const { return m_colors; }
signals:
    void changed();
private:
    void reload() {
        QString state = qEnvironmentVariable("XDG_STATE_HOME", QDir::homePath() + "/.local/state");
        QFile file(state + "/omarchy/current/theme/colors.toml");
        QByteArray data = file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
        if (m_initialized && data == m_data) return;
        m_initialized = true; m_data = data;
        m_colors = {{"green", "#9ece6a"}, {"red", "#f7768e"}, {"yellow", "#e0af68"}, {"background", "#121212"}, {"lighter_background", "#1e1e1e"}, {"foreground", "#bebebe"}, {"accent", "#e68e0d"}, {"selection", "#333333"}, {"muted", "#333333"}, {"light_foreground", "#8a8a8d"}};
        QRegularExpression pattern("^\\s*([a-z_]+)\\s*=\\s*\"(#[0-9A-Fa-f]{6})\"");
        for (const auto &line : QString::fromUtf8(data).split('\n')) {
            auto match = pattern.match(line);
            if (match.hasMatch()) m_colors[match.captured(1)] = match.captured(2);
        }
        emit changed();
    }
    bool m_initialized = false;
    QByteArray m_data;
    QVariantMap m_colors;
};
