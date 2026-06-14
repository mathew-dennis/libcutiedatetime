#pragma once

#include <QObject>
#include <QDateTime>
#include <QStringList>
#include <QDBusInterface>
#include <QQmlEngine>
#include <QJSEngine>

// ─────────────────────────────────────────────────────────────────────────────
//  CutieDateTime
//
//  A lightweight singleton that exposes date/time and timezone controls to QML.
//  It talks to systemd-timedated over D-Bus (org.freedesktop.timedate1).
//
//  Memory note for availableTimezones()
//  ─────────────────────────────────────
//  The method returns a plain QStringList (a value type).  Qt/QML converts it
//  to an ordinary JavaScript array — no QObject is heap-allocated.  The array
//  lives only as long as the QML variable that holds it; when the settings page
//  that uses the timezone picker is destroyed, the JS array is garbage-collected
//  automatically.  Nothing is cached inside the C++ singleton, so there is
//  nothing extra to free when the settings app closes.
// ─────────────────────────────────────────────────────────────────────────────
class CutieDateTime : public QObject {
    Q_OBJECT

    // Live-bindable properties — QML can bind to these with e.g.:
    //   Switch { checked: CutieDateTime.ntpEnabled }
    Q_PROPERTY(bool    ntpEnabled       READ ntpEnabled       NOTIFY ntpEnabledChanged)
    Q_PROPERTY(QString currentTimezone  READ currentTimezone  NOTIFY currentTimezoneChanged)

public:
    explicit CutieDateTime(QObject *parent = nullptr);
    ~CutieDateTime();

    // ── Property accessors ───────────────────────────────────────────────────
    bool    ntpEnabled()      const;
    QString currentTimezone() const;

    // ── Actions called from QML ──────────────────────────────────────────────

    // Enable or disable Network Time Protocol synchronisation.
    // Returns true on success, false on D-Bus error (errorOccurred is emitted).
    Q_INVOKABLE bool setNTP(bool enabled);

    // Set the system timezone. `timezone` must be an IANA zone ID such as
    // "Asia/Dubai" or "Europe/London".  Use availableTimezones() for the
    // full list.  Returns true on success.
    Q_INVOKABLE bool setTimezone(const QString &timezone);

    // Set the system clock to `dateTime` (UTC).  Only works when NTP is off;
    // timedated returns an error otherwise.  Returns true on success.
    Q_INVOKABLE bool setTime(const QDateTime &dateTime);

    // Returns every IANA timezone ID the Qt runtime knows about, sorted
    // alphabetically.  The result is a plain QStringList — QML receives it as
    // a JS array.  Nothing is cached in C++; the data is freed when the QML
    // component (e.g. the timezone picker page) that holds the array goes away.
    Q_INVOKABLE QStringList availableTimezones() const;

    // ── QML singleton plumbing ───────────────────────────────────────────────
    static CutieDateTime *instance();
    static QObject       *provider(QQmlEngine *engine, QJSEngine *scriptEngine);

signals:
    void ntpEnabledChanged();
    void currentTimezoneChanged();

    // Emitted whenever a D-Bus call fails.  Connect from QML to show a toast.
    void errorOccurred(const QString &message);

private slots:
    // Keeps the cached properties in sync when systemd changes them
    // (e.g. another app or systemd itself toggles NTP).
    void onPropertiesChanged(const QString  &interface,
                             const QVariantMap &changed,
                             const QStringList &invalidated);

private:
    // Reads NTP and Timezone from D-Bus and emits signals if they changed.
    void refreshProperties();

    bool    m_ntpEnabled       { false };
    QString m_currentTimezone;

    // Owned by this object (parented in the constructor).
    QDBusInterface *m_timedateIface { nullptr };

    Q_DISABLE_COPY(CutieDateTime)
};
