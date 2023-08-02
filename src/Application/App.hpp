#pragma once
#include <vector> //must be defined before KPServer is included, else error
#include <KPController.hpp>
#include <KPFileLoader.hpp>
#include <KPServer.hpp>


class App : public KPController {
private:
  void setupServerRouting();
public:
  KPFileLoader fileLoader{"file-loader", 10}; //SDCS is 10 for Atmel M0
  KPServer server{"web-server", "subsampler", "ilab_sampler"};
  void setup() override {
    Serial.begin(115200);
    while(!Serial) {};
    addComponent(server);
    server.begin();
  }

  void update() override {
    KPController::update();
  };
};