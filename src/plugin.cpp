#include "plugin.h"

void CutieDateTimePlugin::registerTypes(const char *uri)
{
    // CutieDateTime is a singleton — one instance for the whole app.
    // QML usage:  import Cutie.Datetime 0.1
    //             CutieDateTime.setNTP(true)
    qmlRegisterSingletonType<CutieDateTime>(
        uri, 1, 0, "CutieDateTime",
        &CutieDateTime::provider);
}
