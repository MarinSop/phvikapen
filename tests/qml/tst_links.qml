import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    property int notebookCount: 0

    function openNotebook(path) {
        const canvas = createTemporaryObject(canvasComponent, testCase);
        const notebook = createTemporaryObject(notebookComponent, testCase, {
            canvas: canvas,
            notebookPath: path
        });
        tryCompare(notebook, "loaded", true);
        return notebook;
    }

    function newNotebookPath() {
        notebookCount += 1;
        return temporaryDirectory + "/links-" + notebookCount + ".phvika";
    }

    function draw(notebook, fromX, fromY) {
        const canvas = notebook.canvas;
        mousePress(canvas, fromX, fromY);
        mouseMove(canvas, fromX + 20, fromY + 10, -1, Qt.LeftButton);
        mouseMove(canvas, fromX + 40, fromY + 20, -1, Qt.LeftButton);
        mouseRelease(canvas, fromX + 40, fromY + 20);
    }

    function test_a_aLinkOutOfTheApplicationIsMadeAndFound() {
        const notebook = openNotebook(newNotebookPath());

        notebook.addLink("https://example.org", false, "Somewhere");

        compare(notebook.links.length, 1);
        const link = notebook.links[0];
        compare(link.toPage, false);
        compare(link.where, "https://example.org");
        compare(link.label, "Somewhere");
        verify(link.reachable);
    }

    function test_b_aLinkThatIsNotSafeIsRefused() {
        const notebook = openNotebook(newNotebookPath());

        notebook.addLink("file:///etc/passwd", false, "");

        compare(notebook.links.length, 0);
        verify(notebook.errorMessage !== "", "nothing was said about why");
    }

    function test_c_aLinkToAPageGoesThere() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addPage();
        tryCompare(notebook, "pageCount", 2);
        const pages = notebook.pagesToLinkTo;
        compare(pages.length, 2);
        const first = pages[0].pageId;
        const second = pages[1].pageId;
        notebook.currentPage = 1;
        tryCompare(notebook, "currentPage", 1);

        notebook.addLink(first, true, "Back to the first");

        compare(notebook.links.length, 1);
        const linkId = notebook.links[0].linkId;

        notebook.followLink(linkId);

        tryCompare(notebook, "currentPage", 0, 2000, "the link did not go to the page it names");
        verify(second !== first);
    }

    function test_d_aLinkGoingOutTellsTheWindowRatherThanOpeningIt() {
        const notebook = openNotebook(newNotebookPath());
        const told = createTemporaryObject(spyComponent, testCase, {
            target: notebook,
            signalName: "goingOut"
        });
        notebook.addLink("https://example.org", false, "");
        const linkId = notebook.links[0].linkId;

        notebook.followLink(linkId);

        compare(told.count, 1);
        compare(told.signalArguments[0][0].toString(), "https://example.org");
    }

    function test_e_aLinkIsFoundUnderThePlaceItStands() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addLink("https://example.org", false, "");
        const link = notebook.links[0];

        const found = notebook.linkUnder(link.columnX + 2, link.columnY + 2);

        compare(found.linkId, link.linkId);
        const nothing = notebook.linkUnder(link.columnX - 500, link.columnY - 500);
        compare(nothing.linkId, undefined);
    }

    function test_f_aLinkIsChangedAndTakenAway() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addLink("https://example.org", false, "First");
        const linkId = notebook.links[0].linkId;

        notebook.changeLink(linkId, "https://example.com", false, "Second");

        compare(notebook.links[0].where, "https://example.com");
        compare(notebook.links[0].label, "Second");

        notebook.removeLink(linkId);

        compare(notebook.links.length, 0);
    }

    function test_g_makingALinkIsUndone() {
        const notebook = openNotebook(newNotebookPath());

        notebook.addLink("https://example.org", false, "");
        compare(notebook.links.length, 1);

        notebook.undo();

        compare(notebook.links.length, 0);

        notebook.redo();

        compare(notebook.links.length, 1);
    }

    function test_h_aLinkIsThereAgainWhenTheNotebookIsOpenedAgain() {
        const path = newNotebookPath();
        {
            const first = openNotebook(path);
            first.addLink("https://example.org", false, "Kept");
            compare(first.links.length, 1);
            first.destroy();
            wait(0);
        }

        const again = openNotebook(path);

        tryVerify(() => again.links.length === 1, 4000, "the link was not kept");
        compare(again.links[0].label, "Kept");
        compare(again.links[0].where, "https://example.org");
    }

    function test_i_aLinkIsPutOverWhatIsPickedUp() {
        const notebook = openNotebook(newNotebookPath());
        notebook.writeDown("Linked words", {}, false);
        const boxId = notebook.pickedText;
        verify(boxId !== "", "there is nothing to put a link over");
        const over = notebook.areaOfWhatIsPicked();
        verify(over.width > 0);

        notebook.addLink("https://example.org", false, "");

        compare(notebook.links.length, 1);
        const link = notebook.links[0];
        verify(Math.abs(link.columnX - over.columnX) < 1, "the link did not go over the words");
        verify(Math.abs(link.width - over.width) < 1);
    }

    function test_j_aLinkTapIsFollowedWhileThePenIsInHand() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addLink("https://example.org", false, "");
        const layer = createTemporaryObject(layerComponent, testCase, {
            canvas: notebook.canvas,
            notebook: notebook
        });
        const told = createTemporaryObject(spyComponent, testCase, {
            target: layer,
            signalName: "followed"
        });
        tryVerify(() => findChild(layer, "link_" + notebook.links[0].linkId) !== null);
        const patch = findChild(layer, "link_" + notebook.links[0].linkId);

        // The pen is what the reader is holding; a tap on a link is still a link.
        mouseClick(patch, patch.width / 2, patch.height / 2);

        compare(told.count, 1, "the link was not followed while the pen was in hand");
        compare(told.signalArguments[0][0], notebook.links[0].linkId);
    }

    function test_k_aDragThatStartsOnALinkIsStillAStroke() {
        const notebook = openNotebook(newNotebookPath());
        notebook.addLink("https://example.org", false, "");
        const layer = createTemporaryObject(layerComponent, testCase, {
            canvas: notebook.canvas,
            notebook: notebook
        });
        const told = createTemporaryObject(spyComponent, testCase, {
            target: layer,
            signalName: "followed"
        });
        tryVerify(() => findChild(layer, "link_" + notebook.links[0].linkId) !== null);
        const patch = findChild(layer, "link_" + notebook.links[0].linkId);

        mousePress(patch, 4, 4);
        mouseMove(patch, 40, 40);
        mouseRelease(patch, 40, 40);

        compare(told.count, 0, "a drag off the link should not follow it");
    }

    height: 400
    name: "Links"
    visible: true
    when: windowShown
    width: 400

    Component {
        id: canvasComponent

        InkCanvas {
            height: testCase.height
            width: testCase.width
        }
    }

    Component {
        id: notebookComponent

        NotebookViewModel {
        }
    }

    Component {
        id: layerComponent

        LinkLayer {
            height: testCase.height
            width: testCase.width
        }
    }

    Component {
        id: spyComponent

        SignalSpy {
        }
    }
}
