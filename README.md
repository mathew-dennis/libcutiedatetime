# libcutiedatetime

A Cutie Shell Qt6/QML module that exposes system date/time, timezone, and NTP
controls to the settings UI via **`org.freedesktop.timedate1`** (systemd-timedated).

---

## QML API

```qml
import Cutie.Datetime 0.1
```

### Properties (live-bindable)

| Property | Type | Description |
|---|---|---|
| `ntpEnabled` | `bool` | Whether NTP sync is currently active |
| `currentTimezone` | `string` | Active IANA timezone ID, e.g. `"Asia/Dubai"` |

### Methods

| Method | Returns | Description |
|---|---|---|
| `setNTP(bool enabled)` | `bool` | Enable / disable NTP sync |
| `setTimezone(string tz)` | `bool` | Set timezone by IANA ID |
| `setTime(date dt)` | `bool` | Set clock manually (NTP must be off) |
| `availableTimezones()` | `string[]` | Sorted list of all IANA zone IDs |

### Signal

```qml
onErrorOccurred(string message)  // emitted when a D-Bus call fails
```

---

## Usage example

```qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import Cutie.Datetime 0.1

Page {
    // ── NTP toggle ────────────────────────────────────────────────────────
    Switch {
        text: "Sync time automatically"
        checked: CutieDateTime.ntpEnabled
        onToggled: CutieDateTime.setNTP(checked)
    }

    // ── Manual time picker ────────────────────────────────────────────────
    Button {
        enabled: !CutieDateTime.ntpEnabled
        text: "Apply time"
        onClicked: CutieDateTime.setTime(myDatePicker.selectedDate)
    }

    // ── Timezone list ─────────────────────────────────────────────────────
    // availableTimezones() is called once when this page is created.
    // The resulting JS array is held by `tzModel` and freed automatically
    // when this Page component is destroyed (i.e. when the settings app
    // navigates away from the timezone page or is closed).
    ListView {
        model: CutieDateTime.availableTimezones()   // JS array, no QObject
        delegate: ItemDelegate {
            text: modelData
            highlighted: modelData === CutieDateTime.currentTimezone
            onClicked: CutieDateTime.setTimezone(modelData)
        }
    }

    // ── Error handling ────────────────────────────────────────────────────
    Connections {
        target: CutieDateTime
        function onErrorOccurred(msg) {
            toastMessage.text = msg
            toastMessage.open()
        }
    }
}
```

---

## Memory model for `availableTimezones()`

`availableTimezones()` returns a **`QStringList` by value** — Qt converts this
to a JavaScript array before it reaches QML.  No `QObject` is heap-allocated,
and nothing is cached inside the C++ singleton.

```
C++ call stack                QML JS engine
─────────────────────         ──────────────────────────────
availableTimezones()
  QTimeZone::availableTimeZoneIds()  →  stack-allocated QList
  builds QStringList on stack
  returns by value (CoW refcount)  →  converted to JS array
  stack frame released                 held by the QML variable
                                       │
                              Page destroyed / app closed
                                       │
                                       ▼
                              JS GC collects the array — done
```

No explicit `destroy()` or `delete` is needed.  The data lives exactly as
long as the QML component that holds it.

---

## Polkit

`SetNTP`, `SetTimezone`, and `SetTime` are called with `interactive = false`,
meaning polkit will not show an authentication dialog.  The call succeeds
immediately if the calling user already has the required privilege, and fails
with a polkit error otherwise.

On Droidian the compositor user is typically granted these privileges via a
polkit rule under `/etc/polkit-1/rules.d/`.  A minimal rule:

```javascript
// /etc/polkit-1/rules.d/10-cutie-timedate.rules
polkit.addRule(function(action, subject) {
    if (action.id === "org.freedesktop.timedate1.set-ntp"        ||
        action.id === "org.freedesktop.timedate1.set-timezone"   ||
        action.id === "org.freedesktop.timedate1.set-time") {
        if (subject.isInGroup("sudo") || subject.local) {
            return polkit.Result.YES;
        }
    }
});
```

---

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

### Debian package

```bash
dpkg-buildpackage -us -uc -b
```
