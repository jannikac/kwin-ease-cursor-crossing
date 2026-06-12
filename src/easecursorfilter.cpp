/*
    SPDX-FileCopyrightText: 2026 jannikac <jannikac@mailbox.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "easecursorfilter.h"
#include "easecursor_debug.h"

#include "core/output.h"
#include "input_event.h"
#include "main.h"
#include "pointer_input.h"
#include "workspace.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <cmath>

using namespace KWin;

// KWin clamps the cursor to 1 logical px inside the output edge (confineToBoundingBox)
static constexpr qreal s_pinEpsilon = 1.5;
// how far inside the destination output the cursor lands after a warp
static constexpr qreal s_landingMargin = 2.0;

static qreal distanceToInterval(qreal value, qreal lo, qreal hi)
{
    if (value < lo) {
        return lo - value;
    }
    if (value > hi) {
        return value - hi;
    }
    return 0.0;
}

EaseCursorCrossingFilter::EaseCursorCrossingFilter()
    : KWin::Plugin()
    , KWin::InputEventFilter(InputFilterOrder::ButtonRebind)
{
    loadConfig();
    m_configWatcher = KConfigWatcher::create(kwinApp()->config());
    connect(m_configWatcher.get(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group) {
        if (group.name() == QLatin1String("EaseCursorCrossing")) {
            loadConfig();
        }
    });
    input()->installInputEventFilter(this);
}

EaseCursorCrossingFilter::~EaseCursorCrossingFilter()
{
    input()->uninstallInputEventFilter(this);
}

void EaseCursorCrossingFilter::loadConfig()
{
    const KConfigGroup group = kwinApp()->config()->group(QStringLiteral("EaseCursorCrossing"));
    m_active = group.readEntry(QStringLiteral("Active"), true);
    m_threshold = std::max(0.0, group.readEntry(QStringLiteral("Threshold"), 25.0));
    const QString mode = group.readEntry(QStringLiteral("Mode"), QStringLiteral("nearest"));
    m_mode = mode.compare(QLatin1String("proportional"), Qt::CaseInsensitive) == 0 ? Mode::Proportional : Mode::Nearest;
    qCDebug(KWIN_EASECURSOR) << "config loaded, active:" << m_active << "threshold:" << m_threshold << "mode:" << (m_mode == Mode::Proportional ? "proportional" : "nearest");
}

void EaseCursorCrossingFilter::reset()
{
    m_pushDir = QPointF();
    m_accumulated = 0.0;
}

bool EaseCursorCrossingFilter::pointerMotion(PointerMotionEvent *event)
{
    if (!m_active) {
        return false;
    }
    // ignore our own warps and synthetic absolute motion
    if (event->warp || event->delta.isNull()) {
        reset();
        return false;
    }
    // never fight a pointer constraint held by an application
    if (input()->pointer()->isConstrained()) {
        reset();
        return false;
    }

    const QPointF pos = event->position;
    LogicalOutput *output = workspace()->outputAt(pos);
    if (!output) {
        reset();
        return false;
    }
    const QRectF geo = output->geometryF();

    // which edge is the cursor pinned against while still being pushed outward?
    QPointF dir;
    if (event->delta.x() > 0 && pos.x() >= geo.right() - s_pinEpsilon) {
        dir.setX(1);
    } else if (event->delta.x() < 0 && pos.x() <= geo.left() + s_pinEpsilon) {
        dir.setX(-1);
    }
    if (event->delta.y() > 0 && pos.y() >= geo.bottom() - s_pinEpsilon) {
        dir.setY(1);
    } else if (event->delta.y() < 0 && pos.y() <= geo.top() + s_pinEpsilon) {
        dir.setY(-1);
    }
    if (dir.x() != 0 && dir.y() != 0) {
        // pinned in a corner: follow the dominant motion axis
        if (std::abs(event->delta.x()) >= std::abs(event->delta.y())) {
            dir.setY(0);
        } else {
            dir.setX(0);
        }
    }
    if (dir.isNull()) {
        reset();
        return false;
    }

    // if an output sits right beyond this edge, KWin crosses on its own (or the
    // soft edge barrier is intentionally holding the cursor) — only act on deadzones
    const QPointF probe = pos + dir * (s_pinEpsilon + 1.0);
    const auto outputs = workspace()->outputs();
    for (const LogicalOutput *other : outputs) {
        if (QRectF(other->geometryF()).contains(probe)) {
            reset();
            return false;
        }
    }

    if (dir != m_pushDir) {
        m_pushDir = dir;
        m_accumulated = 0.0;
    }
    m_accumulated += std::abs(dir.x() != 0 ? event->delta.x() : event->delta.y());
    if (m_accumulated < m_threshold) {
        return false;
    }

    if (tryEase(pos, dir, geo)) {
        reset();
    } else {
        // nothing on that side at all; don't re-evaluate on every event
        m_accumulated = 0.0;
    }
    return false;
}

bool EaseCursorCrossingFilter::tryEase(const QPointF &pos, const QPointF &outwardDir, const QRectF &sourceGeo)
{
    const bool horizontal = outwardDir.x() != 0;

    // nearest output on the far side of the pinned edge
    LogicalOutput *best = nullptr;
    qreal bestPerpGap = 0.0;
    qreal bestParDist = 0.0;
    const auto outputs = workspace()->outputs();
    for (LogicalOutput *candidate : outputs) {
        const QRectF g = candidate->geometryF();
        qreal perpGap;
        if (horizontal) {
            perpGap = outwardDir.x() > 0 ? g.left() - sourceGeo.right() : sourceGeo.left() - g.right();
        } else {
            perpGap = outwardDir.y() > 0 ? g.top() - sourceGeo.bottom() : sourceGeo.top() - g.bottom();
        }
        if (perpGap < -0.5) {
            continue;
        }
        const qreal parDist = horizontal
            ? distanceToInterval(pos.y(), g.top(), g.bottom())
            : distanceToInterval(pos.x(), g.left(), g.right());
        if (!best || perpGap < bestPerpGap - 0.5 || (perpGap < bestPerpGap + 0.5 && parDist < bestParDist)) {
            best = candidate;
            bestPerpGap = perpGap;
            bestParDist = parDist;
        }
    }
    if (!best) {
        return false;
    }

    const QRectF g = best->geometryF();
    QPointF target;
    if (horizontal) {
        target.setX(outwardDir.x() > 0 ? g.left() + s_landingMargin : g.right() - s_landingMargin);
        qreal y = pos.y();
        if (m_mode == Mode::Proportional) {
            y = g.top() + (pos.y() - sourceGeo.top()) / sourceGeo.height() * g.height();
        }
        target.setY(std::clamp(y, g.top() + s_landingMargin, g.bottom() - s_landingMargin));
    } else {
        target.setY(outwardDir.y() > 0 ? g.top() + s_landingMargin : g.bottom() - s_landingMargin);
        qreal x = pos.x();
        if (m_mode == Mode::Proportional) {
            x = g.left() + (pos.x() - sourceGeo.left()) / sourceGeo.width() * g.width();
        }
        target.setX(std::clamp(x, g.left() + s_landingMargin, g.right() - s_landingMargin));
    }

    qCDebug(KWIN_EASECURSOR) << "easing cursor across deadzone from" << pos << "to" << target << "on" << best->name();
    input()->pointer()->warp(target);
    return true;
}

#include "moc_easecursorfilter.cpp"
