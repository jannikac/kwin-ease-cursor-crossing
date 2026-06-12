/*
    SPDX-FileCopyrightText: 2026 jannikac <jannikac@mailbox.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    id: root

    Kirigami.FormLayout {
        QQC2.CheckBox {
            Kirigami.FormData.label: "Cursor crossing:"
            text: "Ease cursor movement between screens with mismatched edges"
            checked: kcm.active
            onToggled: kcm.active = checked
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        QQC2.SpinBox {
            Kirigami.FormData.label: "Edge barrier:"
            enabled: kcm.active
            from: 0
            to: 200
            stepSize: 5
            value: kcm.threshold
            onValueModified: kcm.threshold = value
            textFromValue: (value, locale) => value + " px"
            valueFromText: (text, locale) => parseInt(text)
        }

        QQC2.ComboBox {
            Kirigami.FormData.label: "Crossing position:"
            enabled: kcm.active
            model: [
                "Closest point on the other screen",
                "Proportional position on the other screen"
            ]
            currentIndex: kcm.proportional ? 1 : 0
            onActivated: index => kcm.proportional = (index === 1)
        }
    }
}
