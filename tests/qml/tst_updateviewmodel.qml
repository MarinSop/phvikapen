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

    function test_nothingIsInstalledUntilSomethingWasFound() {
        const updates = createTemporaryObject(updateComponent, testCase);
        const restarts = createTemporaryObject(signalSpyComponent, testCase, {
            target: updates,
            signalName: "restartWanted"
        });

        updates.install();

        compare(updates.state, UpdateViewModel.Idle);
        compare(restarts.count, 0);
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
