#pragma once

#include <QDebug>
#include <QtQuick>
#include <QtQml/qqml.h>
#include <QtQml/QQmlExtensionPlugin>

#include "cutiedatetime.h"

class CutieDateTimePlugin : public QQmlExtensionPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid FILE "cutiedatetime.json")

public:
    explicit CutieDateTimePlugin() {}
    void registerTypes(const char *uri) override;
};
