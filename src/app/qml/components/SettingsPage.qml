import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One page of the settings: a column that scrolls when it grows past the window.
ScrollView {
    id: root

    default property alias content: column.data

    contentWidth: availableWidth

    ColumnLayout {
        id: column

        spacing: 12
        width: root.availableWidth
    }
}
