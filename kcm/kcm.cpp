/*
    SPDX-FileCopyrightText: 2026 jannikac <jannikac@mailbox.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "kcm.h"

#include <KConfigGroup>
#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(EaseCursorCrossingKcm, "kcm_easecursorcrossing.json")

static const char s_group[] = "EaseCursorCrossing";

EaseCursorCrossingKcm::EaseCursorCrossingKcm(QObject *parent, const KPluginMetaData &data)
    : KQuickConfigModule(parent, data)
    , m_config(KSharedConfig::openConfig(QStringLiteral("kwinrc")))
{
}

bool EaseCursorCrossingKcm::active() const
{
    return m_active;
}

void EaseCursorCrossingKcm::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    m_active = active;
    Q_EMIT activeChanged();
    updateNeedsSave();
}

int EaseCursorCrossingKcm::threshold() const
{
    return m_threshold;
}

void EaseCursorCrossingKcm::setThreshold(int threshold)
{
    if (m_threshold == threshold) {
        return;
    }
    m_threshold = threshold;
    Q_EMIT thresholdChanged();
    updateNeedsSave();
}

bool EaseCursorCrossingKcm::proportional() const
{
    return m_proportional;
}

void EaseCursorCrossingKcm::setProportional(bool proportional)
{
    if (m_proportional == proportional) {
        return;
    }
    m_proportional = proportional;
    Q_EMIT proportionalChanged();
    updateNeedsSave();
}

void EaseCursorCrossingKcm::load()
{
    m_config->reparseConfiguration();
    const KConfigGroup group = m_config->group(QLatin1String(s_group));
    m_savedActive = group.readEntry(QStringLiteral("Active"), true);
    m_savedThreshold = group.readEntry(QStringLiteral("Threshold"), 25);
    m_savedProportional = group.readEntry(QStringLiteral("Mode"), QStringLiteral("nearest"))
                              .compare(QLatin1String("proportional"), Qt::CaseInsensitive)
        == 0;
    setActive(m_savedActive);
    setThreshold(m_savedThreshold);
    setProportional(m_savedProportional);
    setNeedsSave(false);
}

void EaseCursorCrossingKcm::save()
{
    KConfigGroup group = m_config->group(QLatin1String(s_group));
    // KConfig::Notify makes KConfigWatcher in the running KWin pick the change up immediately
    group.writeEntry(QStringLiteral("Active"), m_active, KConfig::Notify);
    group.writeEntry(QStringLiteral("Threshold"), m_threshold, KConfig::Notify);
    group.writeEntry(QStringLiteral("Mode"), m_proportional ? QStringLiteral("proportional") : QStringLiteral("nearest"), KConfig::Notify);
    group.sync();
    m_savedActive = m_active;
    m_savedThreshold = m_threshold;
    m_savedProportional = m_proportional;
    setNeedsSave(false);
}

void EaseCursorCrossingKcm::defaults()
{
    setActive(true);
    setThreshold(25);
    setProportional(false);
}

void EaseCursorCrossingKcm::updateNeedsSave()
{
    setNeedsSave(m_active != m_savedActive || m_threshold != m_savedThreshold || m_proportional != m_savedProportional);
}

#include "kcm.moc"
