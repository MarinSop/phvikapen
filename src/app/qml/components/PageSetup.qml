import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import PhvikaPen.Ui

ColumnLayout {
    id: root

    property NotebookViewModel notebook: null
    readonly property list<int> backgrounds: [PageOptions.Blank, PageOptions.Lined, PageOptions.Grid, PageOptions.Dotted]
    readonly property list<string> backgroundNames: [qsTr("Blank"), qsTr("Lined"), qsTr("Squares"), qsTr("Dots")]
    readonly property list<int> papers: [PageOptions.Infinite, PageOptions.A3, PageOptions.A4, PageOptions.A5, PageOptions.Letter, PageOptions.Legal, PageOptions.Custom]
    readonly property list<string> paperNames: [qsTr("Infinite"), qsTr("A3"), qsTr("A4"), qsTr("A5"), qsTr("Letter"), qsTr("Legal"), qsTr("Own size")]

    spacing: 4

    Label {
        text: qsTr("Paper of this section")
    }

    ComboBox {
        id: paperBox

        Layout.fillWidth: true
        currentIndex: root.notebook === null ? -1 : root.papers.indexOf(root.notebook.paper)
        model: root.paperNames
        objectName: "paperBox"

        onActivated: root.notebook.paper = root.papers[paperBox.currentIndex]
    }

    SizeFields {
        Layout.fillWidth: true
        heightMillimetres: root.notebook === null ? 297 : root.notebook.customHeight
        objectName: "pageSize"
        visible: root.notebook !== null && root.notebook.paper === PageOptions.Custom
        widthMillimetres: root.notebook === null ? 210 : root.notebook.customWidth

        onSizeEdited: (wide, tall) => {
            root.notebook.customWidth = wide;
            root.notebook.customHeight = tall;
        }
    }

    ComboBox {
        id: backgroundBox

        Layout.fillWidth: true
        currentIndex: root.notebook === null ? -1 : root.backgrounds.indexOf(root.notebook.background)
        model: root.backgroundNames
        objectName: "backgroundBox"

        onActivated: root.notebook.background = root.backgrounds[backgroundBox.currentIndex]
    }

    MenuSeparator {
        Layout.fillWidth: true
    }

    PageLook {
        Layout.fillWidth: true
        enabled: root.notebook !== null
        lineColor: root.notebook === null ? "transparent" : root.notebook.lineColor
        lineSpacing: root.notebook === null ? 7 : root.notebook.lineSpacing
        lineWidth: root.notebook === null ? 1 : root.notebook.lineWidth
        lined: root.notebook !== null && root.notebook.background === PageOptions.Lined
        marginAt: root.notebook === null ? 25 : root.notebook.marginAt
        marginColor: root.notebook === null ? "transparent" : root.notebook.marginColor
        marginShown: root.notebook !== null && root.notebook.margin
        objectName: "pageLook"
        paperColor: root.notebook === null ? "transparent" : root.notebook.paperColor

        onLineColorPicked: wanted => root.notebook.lineColor = wanted
        onLineSpacingChosen: wanted => root.notebook.lineSpacing = wanted
        onLineWidthChosen: wanted => root.notebook.lineWidth = wanted
        onMarginAtChosen: wanted => root.notebook.marginAt = wanted
        onMarginColorPicked: wanted => root.notebook.marginColor = wanted
        onMarginShownChosen: wanted => root.notebook.margin = wanted
        onPaperColorPicked: wanted => root.notebook.paperColor = wanted
    }

    Switch {
        id: landscapeSwitch

        opacity: enabled ? 1 : 0.4
        checked: root.notebook !== null && root.notebook.orientation === PageOptions.Landscape
        enabled: root.notebook !== null && root.notebook.paper !== PageOptions.Infinite && root.notebook.paper !== PageOptions.Custom
        objectName: "landscapeSwitch"
        text: qsTr("Landscape")

        onToggled: root.notebook.orientation = landscapeSwitch.checked ? PageOptions.Landscape : PageOptions.Portrait
    }

    RowLayout {
        Layout.fillWidth: true
        visible: root.notebook !== null && root.notebook.background !== PageOptions.Blank

        Label {
            text: qsTr("Spacing")
        }

        NumberField {
            Layout.fillWidth: true
            maximum: 30
            minimum: 2
            number: root.notebook === null ? 7 : root.notebook.lineSpacing
            objectName: "spacingField"
            step: 0.5
            suffix: qsTr(" mm")

            onNumberEdited: value => root.notebook.lineSpacing = value
        }
    }
}
