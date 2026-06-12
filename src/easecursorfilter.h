/*
    SPDX-FileCopyrightText: 2026 jannikac <jannikac@mailbox.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "input.h"
#include "plugin.h"

#include <KConfigWatcher>

#include <QPointF>
#include <QRectF>

class EaseCursorCrossingFilter : public KWin::Plugin, public KWin::InputEventFilter
{
    Q_OBJECT

public:
    explicit EaseCursorCrossingFilter();
    ~EaseCursorCrossingFilter() override;

    bool pointerMotion(KWin::PointerMotionEvent *event) override;

private:
    enum class Mode {
        Nearest, // cross at the closest valid point on the destination edge
        Proportional, // map the position along the source edge proportionally onto the destination edge
    };

    void loadConfig();
    void reset();
    bool tryEase(const QPointF &pos, const QPointF &outwardDir, const QRectF &sourceGeo);

    KConfigWatcher::Ptr m_configWatcher;
    bool m_active = true;
    qreal m_threshold = 25.0;
    Mode m_mode = Mode::Nearest;

    // outward push direction while the cursor is pinned in a deadzone, one axis only
    QPointF m_pushDir;
    qreal m_accumulated = 0.0;
};
