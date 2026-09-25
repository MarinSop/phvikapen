import QtQuick
import QtTest
import PhvikaPen.Ui

TestCase {
    id: testCase

    function test_anUninstalledApplicationHasNoUpdates() {
        const updates = createTemporaryObject(updateComponent, testCase);
        const failures = createTemporaryObject(signalSpyComponent, testCase, {
            target: updates,
            signalName: "failed"
        });
        compare(updates.state, UpdateViewModel.Idle);

        updates.check();

        tryCompare(updates, "state", UpdateViewModel.Unavailable);
        compare(updates.busy, false);
        compare(updates.version, "");
        compare(failures.count, 1);
        verify(failures.signalArguments[0][0] !== "");
    }

    function test_nothingIsGotUntilSomethingWasFound() {
        const updates = createTemporaryObject(updateComponent, testCase);
        const restarts = createTemporaryObject(signalSpyComponent, testCase, {
            target: updates,
            signalName: "restartWanted"
        });

        updates.get();
        updates.restartNow();

        compare(updates.state, UpdateViewModel.Idle);
        compare(updates.howFarAlong, 0);
        compare(restarts.count, 0);
    }

    function test_aVersionPutAsideIsNotOfferedAgain() {
        const updates = createTemporaryObject(updateComponent, testCase);
        compare(updates.skippedVersion, "", "nothing is put aside to begin with");
        verify(!updates.worthOffering, "with nothing found there is nothing to offer");

        updates.skippedVersion = "9.9.9";

        compare(updates.skippedVersion, "9.9.9");
        const later = createTemporaryObject(updateComponent, testCase);
        compare(later.skippedVersion, "9.9.9", "what was put aside is remembered");

        later.skippedVersion = "";
    }

    function test_askingForTestVersionsLooksAgainSomewhereElse() {
        const updates = createTemporaryObject(updateComponent, testCase);
        const changes = createTemporaryObject(signalSpyComponent, testCase, {
            target: updates,
            signalName: "testVersionsChanged"
        });
        compare(updates.testVersions, false);

        updates.check();
        tryCompare(updates, "state", UpdateViewModel.Unavailable);
        updates.testVersions = true;

        compare(changes.count, 1);
        updates.check();
        tryCompare(updates, "state", UpdateViewModel.Unavailable);
        compare(updates.testVersions, true);
    }

    function test_lookingTwiceAtOnceDoesNotStartTwoSearches() {
        const updates = createTemporaryObject(updateComponent, testCase);
        const changes = createTemporaryObject(signalSpyComponent, testCase, {
            target: updates,
            signalName: "stateChanged"
        });

        updates.check();
        compare(updates.state, UpdateViewModel.Looking);
        updates.check();

        tryCompare(updates, "state", UpdateViewModel.Unavailable);
        compare(changes.count, 2);
    }

    name: "UpdateViewModel"

    Component {
        id: signalSpyComponent

        SignalSpy {
        }
    }

    Component {
        id: updateComponent

        UpdateViewModel {
        }
    }
}
