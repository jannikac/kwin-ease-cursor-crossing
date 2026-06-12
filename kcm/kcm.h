/*
    SPDX-FileCopyrightText: 2026 jannikac <jannikac@mailbox.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <KQuickConfigModule>
#include <KSharedConfig>

class EaseCursorCrossingKcm : public KQuickConfigModule
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(int threshold READ threshold WRITE setThreshold NOTIFY thresholdChanged)
    Q_PROPERTY(bool proportional READ proportional WRITE setProportional NOTIFY proportionalChanged)

public:
    explicit EaseCursorCrossingKcm(QObject *parent, const KPluginMetaData &data);

    bool active() const;
    void setActive(bool active);
    int threshold() const;
    void setThreshold(int threshold);
    bool proportional() const;
    void setProportional(bool proportional);

    void load() override;
    void save() override;
    void defaults() override;

Q_SIGNALS:
    void activeChanged();
    void thresholdChanged();
    void proportionalChanged();

private:
    void updateNeedsSave();

    KSharedConfig::Ptr m_config;
    bool m_active = true;
    int m_threshold = 25;
    bool m_proportional = false;
    bool m_savedActive = true;
    int m_savedThreshold = 25;
    bool m_savedProportional = false;
};
