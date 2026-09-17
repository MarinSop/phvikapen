# 0004. Where notebook files live

Date: 2026-09-17

## Status

Accepted

## Context

Every notebook is a SQLite database in write-ahead logging mode, so on disk it is a file plus two
companion files that change while the notebook is open. Losing these files means losing
everything the user wrote, so their location has to survive every way the application can be
installed, updated or removed.

On Windows the application is installed per user by Velopack. Its setup program installs into
`%LocalAppData%\PhvikaPen`, replaces the application files on every update, and deletes that whole
folder when the application is uninstalled. Qt's application-local data location on Windows is
that very folder, so notebooks kept there would disappear with an uninstall, and a reinstall to
fix a broken installation would wipe them as well.

The candidates:

- **The application-local data folder.** Where the notebooks were first kept. Deleted on
  uninstall, as described above.
- **The user's Documents folder.** Visible and easy to back up. On Windows 11, however, Documents
  is commonly redirected into OneDrive, and a synchronization client that copies or locks a live
  SQLite database and its write-ahead log in the middle of a write can leave a corrupt file or a
  conflicting copy behind.
- **The roaming application data folder** (`%AppData%\PhvikaPen` on Windows,
  `~/Library/Application Support/PhvikaPen` on macOS). Outside the installation folder, so
  uninstalling and updating leave it alone, which is also what Velopack recommends for data that
  has to outlive the application. It is not synchronized by OneDrive.

## Decision

We will keep notebook files in a `notebooks` folder inside Qt's application data location, the
roaming application data folder on Windows. Logs stay in the application-local data folder, where
losing them on uninstall does no harm.

## Consequences

- Uninstalling, reinstalling and updating the application no longer touch the notebooks.
- The notebooks are not where a user would look for documents. Getting them out, as a backup or as
  PDF export, has to be offered from inside the application, and the application should be able to
  show the folder.
- On a domain-joined machine with roaming profiles, the notebooks would travel with the profile.
  That is not the case on the target device.
- Notebooks written before this change on Windows stay in the old folder. No release has written
  notebooks yet, so nothing is moved automatically.
