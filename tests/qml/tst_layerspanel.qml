import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function layerList() {
        return findChild(panel, "layerList");
    }

    function nameOfRow(index) {
        const model = notebooks.current.layers;
        return model.data(model.index(index, 0), Qt.UserRole + 2);
    }

    // A mark asks for the other of what it shows, and a change reaches the list before it reaches
    // the line drawn from it, so a mark clicked too early asks for what has already happened. The
    // mark says for itself when it has caught up, and the click is aimed at where the mark stands on
    // the panel rather than at the mark, which the list may be about to draw again.
    function clickWhenItShows(index, name, active) {
        let at = null;
        tryVerify(() => {
            const row = testCase.rowAt(index);
            if (row === null) {
                return false;
            }
            const mark = findChild(row, name);
            if (mark === null || mark.width <= 0 || mark.active !== active) {
                return false;
            }
            const spot = testCase.mapFromItem(mark, mark.width / 2, mark.height / 2);
            if (spot.x < 0 || spot.y < 0 || spot.x >= testCase.width || spot.y >= testCase.height) {
                return false;
            }
            at = spot;
            return true;
        }, 5000, "the mark never came to stand where it could be clicked showing what the line says");
        const tapped = createTemporaryObject(spyComponent, testCase, {
            target: findChild(testCase.rowAt(index), name),
            signalName: "clicked"
        });
        mouseClick(testCase, at.x, at.y);
        tryVerify(() => tapped.count === 1, 4000, "the click never reached " + name);
        // Left where it was, the pointer keeps the mark under it and a hint of what it does comes up
        // over the line, which would take the next click for itself.
        mouseMove(testCase, testCase.width - 1, testCase.height - 1);
    }

    // The list is made to answer for any change still waiting in the model, so that the line handed
    // back is the one that will be standing there when it is clicked, not one about to be drawn again.
    function rowAt(index) {
        testCase.layerList().forceLayout();
        return testCase.layerList().itemAtIndex(index);
    }

    function initTestCase() {
        notebooks.createNotebook("Layered");
        tryCompare(notebooks.current, "loaded", true);
        notebooks.current.addLayer();
        notebooks.current.renameLayer(notebooks.current.activeLayer, "Middle");
        notebooks.current.addLayer();
        notebooks.current.renameLayer(notebooks.current.activeLayer, "Top");
        tryCompare(notebooks.current.layers, "count", 3);
        tryVerify(() => testCase.rowAt(2) !== null);
    }

    function test_a_theListIsReadTopFirst() {
        compare(testCase.nameOfRow(0), "Top");
        compare(testCase.nameOfRow(2), "Layer 1");
    }

    function test_b_carryingALayerDownPutsItThere() {
        const row = testCase.rowAt(0);

        mouseDrag(row, row.width / 2, row.height / 2, 0, row.height * 2.2);

        tryCompare(notebooks.current.layers, "count", 3);
        compare(testCase.nameOfRow(2), "Top", "the layer was not carried to the bottom");
        compare(testCase.nameOfRow(0), "Middle");
    }

    function test_c_carryingALayerUpPutsItBack() {
        const row = testCase.rowAt(2);

        mouseDrag(row, row.width / 2, row.height / 2, 0, -row.height * 2.2);

        compare(testCase.nameOfRow(0), "Top", "the layer was not carried back to the top");
        compare(notebooks.current.errorMessage, "");
    }

    function test_d_theLineDrawnInIsMarkedOut() {
        const row = testCase.rowAt(1);

        mouseClick(row, row.width / 2, row.height / 2);

        tryVerify(() => testCase.nameOfRow(1) === "Middle");
        const model = notebooks.current.layers;
        compare(notebooks.current.activeLayer, model.data(model.index(1, 0), Qt.UserRole + 1));
    }

    function test_e_lockingAndHidingShowOnTheLine() {
        const model = notebooks.current.layers;

        testCase.clickWhenItShows(0, "layerShown0", false);
        tryVerify(() => model.data(model.index(0, 0), Qt.UserRole + 3) === false, 5000, "the layer was not put out of sight");
        testCase.clickWhenItShows(0, "layerLocked0", false);
        tryVerify(() => model.data(model.index(0, 0), Qt.UserRole + 4) === true, 5000, "the layer was not locked");

        testCase.clickWhenItShows(0, "layerShown0", true);
        tryVerify(() => model.data(model.index(0, 0), Qt.UserRole + 3) === true, 5000, "the layer was not shown again");
        testCase.clickWhenItShows(0, "layerLocked0", true);
        tryVerify(() => model.data(model.index(0, 0), Qt.UserRole + 4) === false, 5000, "the layer was not unlocked");
    }

    // Holding the pointer down on a line asks what can be done to it, the same menu a right click
    // asks for. The handler that takes the press is the one that must report the holding: a line
    // that leaves it to the delegate underneath is never told the press was held at all.
    function test_ea_holdingALineAsksWhatCanBeDoneToIt() {
        const row = testCase.rowAt(0);
        const menu = findChild(row, "layerLineMenu");
        verify(menu !== null, "the line has no menu of its own");

        mouseClick(row, row.width / 2, row.height / 2, Qt.RightButton);
        tryVerify(() => menu.opened, 2000, "a right click did not ask what can be done to the layer");
        menu.close();
        tryVerify(() => !menu.opened, 2000);

        mousePress(row, row.width / 2, row.height / 2);
        tryVerify(() => menu.opened, 4000, "holding the pointer down did not ask what can be done to the layer");
        mouseRelease(row, row.width / 2, row.height / 2);
        menu.close();
    }

    function test_f_sendingToALayerIsOffUntilSomethingIsPickedUp() {
        verify(!actions.moveToActiveLayer.enabled, "nothing is in hand, so there is nothing to send");
    }

    // What is picked up can be sent to any layer, or to one made for it, from the menu the paper
    // offers. The lines are made from the list of layers, so a layer added appears among them.
    function test_ga_whatIsPickedCanBeSentToAnyLayer() {
        const menu = createTemporaryObject(layerMenuComponent, testCase, {
            actions: actions
        });
        const model = notebooks.current.layers;

        tryVerify(() => menu.count === model.count + 2, 4000, "the menu does not offer every layer");
        verify(findChild(menu, "moveToNewLayerItem") !== null, "there is no way to send it to a layer of its own");

        const named = [];
        for (let line = 0; line < menu.count; ++line) {
            const item = menu.itemAt(line);
            if (item !== null && item.objectName === "moveToLayerItem") {
                named.push(item.text);
            }
        }
        compare(named, [testCase.nameOfRow(0), testCase.nameOfRow(1), testCase.nameOfRow(2)], "the layers are not offered as the list reads them");
    }

    function test_h_theLineShowsWhatStandsOnIt() {
        tryVerify(() => testCase.rowAt(0) !== null && findChild(testCase.rowAt(0), "layerPreview0") !== null);
        const drawn = findChild(testCase.rowAt(0), "layerPreview0");
        tryVerify(() => drawn.source.toString() !== "", 4000, "the line never asked for a picture of its layer");
    }

    function test_g_theLineSaysHowMuchStandsOnIt() {
        const model = notebooks.current.layers;

        compare(model.data(model.index(0, 0), Qt.UserRole + 5), 0);
    }

    // A line says its name and how much stands on it, and what it says grows with the language and
    // with the machine's own letters. However crowded the line becomes, the marks for hiding and
    // locking must stay where they can be reached.
    function test_i_theMarksStayInReachOnACrowdedLine() {
        const model = notebooks.current.layers;
        const was = testCase.width;
        testCase.width = 170;
        try {
            testCase.clickWhenItShows(0, "layerLocked0", false);
            tryVerify(() => model.data(model.index(0, 0), Qt.UserRole + 4) === true, 5000, "the layer was not locked from a crowded line");

            testCase.clickWhenItShows(0, "layerLocked0", true);
            tryVerify(() => model.data(model.index(0, 0), Qt.UserRole + 4) === false, 5000, "the layer was not unlocked from a crowded line");
        } finally {
            testCase.width = was;
        }
    }

    height: 400
    name: "LayersPanel"
    visible: true
    when: windowShown
    width: 280

    ToolViewModel {
        id: tools
    }

    SettingsViewModel {
        id: settings
    }

    NotebooksViewModel {
        id: notebooks

        directory: temporaryDirectory + "/layers"
    }

    WorkspaceViewModel {
        id: workspace
    }

    MathViewModel {
        id: maths
    }

    TimeKeeperViewModel {
        id: timeKeeper
    }

    RecordingViewModel {
        id: recordings

        notebook: notebooks.current
    }

    ElementsViewModel {
        id: elementLibrary

        directory: notebooks.directory + "/elements"
    }

    AppActions {
        id: actions

        library: elementLibrary
        maths: maths
        sound: recordings
        timeKeeper: timeKeeper
        notebooks: notebooks
        settings: settings
        tools: tools
        workspace: workspace
    }

    LayersPanel {
        id: panel

        actions: actions
        anchors.fill: parent
    }

    Component {
        id: layerMenuComponent

        MoveToLayerMenu {
        }
    }

    Component {
        id: spyComponent

        SignalSpy {
        }
    }
}
