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

    function clickOn(index, name) {
        tryVerify(() => testCase.rowAt(index) !== null && findChild(testCase.rowAt(index), name) !== null);
        const row = testCase.rowAt(index);
        const mark = findChild(row, name);
        const at = row.mapFromItem(mark, mark.width / 2, mark.height / 2);
        mouseClick(row, at.x, at.y);
    }

    function rowAt(index) {
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

        testCase.clickOn(0, "layerShown0");
        testCase.clickOn(0, "layerLocked0");
        tryVerify(() => model.data(model.index(0, 0), Qt.UserRole + 3) === false, 5000, "the layer was not put out of sight");
        tryVerify(() => model.data(model.index(0, 0), Qt.UserRole + 4) === true, 5000, "the layer was not locked");

        testCase.clickOn(0, "layerShown0");
        testCase.clickOn(0, "layerLocked0");
        tryVerify(() => model.data(model.index(0, 0), Qt.UserRole + 3) === true, 5000, "the layer was not shown again");
        tryVerify(() => model.data(model.index(0, 0), Qt.UserRole + 4) === false, 5000, "the layer was not unlocked");
    }

    function test_f_sendingToALayerIsOffUntilSomethingIsPickedUp() {
        verify(!actions.moveToActiveLayer.enabled, "nothing is in hand, so there is nothing to send");
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
}
