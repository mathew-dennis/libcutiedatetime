#include "cutiedatetime.h"

#include <QDBusConnection>
#include <QDBusReply>
#include <QDBusMessage>
#include <QDBusError>
#include <QTimeZone>
#include <QDebug>

// ── D-Bus constants for org.freedesktop.timedate1 ────────────────────────────
static constexpr char TIMEDATE_SERVICE[]  = "org.freedesktop.timedate1";
static constexpr char TIMEDATE_PATH[]     = "/org/freedesktop/timedate1";
static constexpr char TIMEDATE_IFACE[]    = "org.freedesktop.timedate1";
static constexpr char PROPERTIES_IFACE[]  = "org.freedesktop.DBus.Properties";

// ── Construction / destruction ───────────────────────────────────────────────

CutieDateTime::CutieDateTime(QObject *parent)
    : QObject(parent)
{
    qDebug() << "module - CutieDateTime : loaded.";

    // Create the D-Bus proxy.  Parented to this so it is freed with us.
    m_timedateIface = new QDBusInterface(
        TIMEDATE_SERVICE, TIMEDATE_PATH, TIMEDATE_IFACE,
        QDBusConnection::systemBus(), this);

    if (!m_timedateIface->isValid()) {
        qWarning() << "module - CutieDateTime : could not connect to"
                   << TIMEDATE_SERVICE << "—" << m_timedateIface->lastError().message();
    }

    // Subscribe to PropertiesChanged so our cached values stay live when
    // another process (or systemd itself) alters the time settings.
    QDBusConnection::systemBus().connect(
        TIMEDATE_SERVICE,
        TIMEDATE_PATH,
        PROPERTIES_IFACE,
        "PropertiesChanged",
        this,
        SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));

    // Populate m_ntpEnabled and m_currentTimezone from the current D-Bus state.
    refreshProperties();
}

CutieDateTime::~CutieDateTime() {
    qDebug() << "module - CutieDateTime : unloaded.";
}

// ── Property accessors ────────────────────────────────────────────────────────

bool    CutieDateTime::ntpEnabled()      const { return m_ntpEnabled;      }
QString CutieDateTime::currentTimezone() const { return m_currentTimezone; }

// ── Private helpers ───────────────────────────────────────────────────────────

// Reads the NTP and Timezone properties from D-Bus using GetAll to avoid two
// round-trips.  Only emits signals when a value actually changed.
void CutieDateTime::refreshProperties() {
    // Build a GetAll call on the Properties interface
    QDBusMessage req = QDBusMessage::createMethodCall(
        TIMEDATE_SERVICE, TIMEDATE_PATH, PROPERTIES_IFACE, "GetAll");
    req << QString(TIMEDATE_IFACE);

    QDBusMessage reply = QDBusConnection::systemBus().call(req);

    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "module - CutieDateTime - refreshProperties() failed:"
                   << reply.errorMessage();
        return;
    }

    if (reply.arguments().isEmpty()) return;

    const QVariantMap props = qdbus_cast<QVariantMap>(reply.arguments().at(0));

    // -- NTP --
    if (props.contains("NTP")) {
        bool newNtp = props.value("NTP").toBool();
        if (newNtp != m_ntpEnabled) {
            m_ntpEnabled = newNtp;
            emit ntpEnabledChanged();
        }
    }

    // -- Timezone --
    if (props.contains("Timezone")) {
        QString newTz = props.value("Timezone").toString();
        if (newTz != m_currentTimezone) {
            m_currentTimezone = newTz;
            emit currentTimezoneChanged();
        }
    }

    qDebug() << "module - CutieDateTime - refreshProperties() :"
             << "NTP =" << m_ntpEnabled << "| TZ =" << m_currentTimezone;
}

// Slot: keep the cache in sync when systemd broadcasts a change.
void CutieDateTime::onPropertiesChanged(const QString     &interface,
                                        const QVariantMap &changed,
                                        const QStringList &invalidated)
{
    Q_UNUSED(interface)
    Q_UNUSED(invalidated)

    if (changed.contains("NTP")) {
        bool newNtp = changed.value("NTP").toBool();
        if (newNtp != m_ntpEnabled) {
            m_ntpEnabled = newNtp;
            emit ntpEnabledChanged();
        }
    }

    if (changed.contains("Timezone")) {
        QString newTz = changed.value("Timezone").toString();
        if (newTz != m_currentTimezone) {
            m_currentTimezone = newTz;
            emit currentTimezoneChanged();
        }
    }
}

// ── Public actions ────────────────────────────────────────────────────────────

// Toggle Network Time Protocol sync.
// interactive = false: polkit must already grant the permission; no pop-up.
bool CutieDateTime::setNTP(bool enabled) {
    if (!m_timedateIface->isValid()) {
        emit errorOccurred("D-Bus interface unavailable");
        return false;
    }

    QDBusReply<void> reply = m_timedateIface->call("SetNTP", enabled, /*interactive=*/false);

    if (!reply.isValid()) {
        qWarning() << "module - CutieDateTime - setNTP() failed:"
                   << reply.error().message();
        emit errorOccurred(reply.error().message());
        return false;
    }

    qDebug() << "module - CutieDateTime - setNTP() : NTP set to" << enabled;
    return true;
}

// Set the IANA timezone (e.g. "Asia/Dubai", "Europe/London").
bool CutieDateTime::setTimezone(const QString &timezone) {
    if (!m_timedateIface->isValid()) {
        emit errorOccurred("D-Bus interface unavailable");
        return false;
    }

    QDBusReply<void> reply = m_timedateIface->call("SetTimezone", timezone, /*interactive=*/false);

    if (!reply.isValid()) {
        qWarning() << "module - CutieDateTime - setTimezone() failed:"
                   << reply.error().message();
        emit errorOccurred(reply.error().message());
        return false;
    }

    qDebug() << "module - CutieDateTime - setTimezone() : timezone set to" << timezone;
    return true;
}

// Set the system clock.  NTP must be disabled first; timedated rejects this
// call while NTP is active and errorOccurred will be emitted.
// `dateTime` should be in UTC.
bool CutieDateTime::setTime(const QDateTime &dateTime) {
    if (!m_timedateIface->isValid()) {
        emit errorOccurred("D-Bus interface unavailable");
        return false;
    }

    // timedated expects microseconds since the Unix epoch (UTC).
    const qint64 usecsSinceEpoch = dateTime.toMSecsSinceEpoch() * 1000LL;

    // relative = false: absolute time.  interactive = false.
    QDBusReply<void> reply = m_timedateIface->call(
        "SetTime", usecsSinceEpoch, /*relative=*/false, /*interactive=*/false);

    if (!reply.isValid()) {
        qWarning() << "module - CutieDateTime - setTime() failed:"
                   << reply.error().message();
        emit errorOccurred(reply.error().message());
        return false;
    }

    qDebug() << "module - CutieDateTime - setTime() : time set to"
             << dateTime.toString(Qt::ISODate);
    return true;
}

// ── Timezone list ─────────────────────────────────────────────────────────────
//
// Returns a QStringList of IANA timezone IDs.
//
// Why there is nothing to free:
//   - This function has *no* member variable.  The QList<QByteArray> built by
//     QTimeZone::availableTimeZoneIds() lives on the stack for the duration of
//     this call and is released when the function returns.
//   - The QStringList `result` is returned by value; Qt uses implicit sharing
//     (copy-on-write) so handing it to the QML engine costs only a ref-count
//     bump, not a deep copy.
//   - QML receives the list as a JavaScript array.  When the QML component that
//     holds the array (the timezone picker page) is destroyed, the JS engine
//     garbage-collects it — no explicit delete or cleanup needed.
//
QStringList CutieDateTime::availableTimezones() const {
    QStringList result;

    const QList<QByteArray> ids = QTimeZone::availableTimeZoneIds();
    result.reserve(ids.size());

    for (const QByteArray &id : ids)
        result.append(QString::fromLatin1(id));

    result.sort(Qt::CaseInsensitive);

    qDebug() << "module - CutieDateTime - availableTimezones() :"
             << result.size() << "zones returned to QML";

    return result;  // returned by value — no heap QObject is created
}

// ── Singleton plumbing ────────────────────────────────────────────────────────

// The static instance lives for the entire process lifetime (same as the
// settings app process).  The QML engine never tries to delete a singleton
// returned this way.
CutieDateTime *CutieDateTime::instance() {
    static CutieDateTime inst;
    return &inst;
}

QObject *CutieDateTime::provider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    return CutieDateTime::instance();
}
