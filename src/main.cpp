#include <Arduino.h>
#include <Application/App.hpp>

namespace {
    App app;
}

void setup() {
    app.setup();
}

void loop() {
    app.update();
}