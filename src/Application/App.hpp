#pragma once
#define ARDUINOJSON_USE_LONG_LONG 1
#include <vector> //must be defined before KPServer is included, else error
#include <KPController.hpp>
#include <KPFileLoader.hpp>
#include <KPServer.hpp>
#include <Application/Status.hpp>
#include <Application/API.hpp>

#include <Components/Power.hpp>

#include <Valve/Valve.hpp>
#include <Valve/ValveManager.hpp>

#include <Utilities/JsonEncodableDecodable.hpp>

class App : public KPController {
private:
  void setupServerRouting();
public:
  KPFileLoader fileLoader{"file-loader", 10}; //SDCS is 10 for Atmel M0
  KPServer server{"web-server", "subsampler", "ilab_sampler"};

  Power power{"power"};
  Config config{"config.js"}; //config object will be read from SD card in future. For now, hard code.
  Status status;

  template <typename T, typename... Args>
  auto dispatchAPI(Args &&... args) {
      return T{}(*this, std::forward<Args>(args)...);
  }

  
  void setup() override {
    Serial.begin(115200);
    while(!Serial) {};

    addComponent(power);
    randomSeed(now());

    
    addComponent(server);
    server.begin();
    setupServerRouting();

    addComponent(fileLoader);

    status.init(config);
  }

  void update() override {
    KPController::update();
  };
};