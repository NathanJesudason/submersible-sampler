#pragma once
#include <array>

#include <KPFoundation.hpp>
#include <KPState.hpp>
#include <KPStateMachine.hpp>

#include <Application/Config.hpp>
#include <Utilities/JsonFileLoader.hpp>
#include <Valve/ValveStatus.hpp>
#include <Valve/ValveObserver.hpp>

class Status : public JsonDecodable,
               public JsonEncodable,
               public Printable {
public:
    std::vector<int> valves;
    int currentValve   = -1;
    float pressure     = 0;
    float temperature  = 0;
    float barometric   = 0;
    float waterVolume  = 0;
    float waterDepth   = 0;
    float waterFlow    = 0;
    float sampleVolume = 0;

    float maxPressure = 0;

    bool preventShutdown = false;

    const char * currentStateName = nullptr;
    const char * currentTaskName  = nullptr;

    Status() = default;
    // Status(const Status &) = delete;
    // Status & operator=(const Status &) = delete;

    /** ────────────────────────────────────────────────────────────────────────────
     *  @brief Initialize status from user config
     *
     *  @param config Config object containing meta data of the system
     *  ──────────────────────────────────────────────────────────────────────────── */
    void init(Config & config) {
        valves.resize(config.numberOfValves);
        memcpy(valves.data(), config.valves, sizeof(int) * config.numberOfValves);
    }

private:


public:
    /** ────────────────────────────────────────────────────────────────────────────
     *  @brief Override_Mode_Pin is connected to an external switch which is active low.
     *  Override_Mode_Pin is connected to an external switch which is active low.
     *
     *  @return bool true if machine is in programming mode, false otherwise
     *  ──────────────────────────────────────────────────────────────────────────── */
    static bool isProgrammingMode() {
#ifdef LIVE
        return analogRead(HardwarePins::SHUTDOWN_OVERRIDE) <= 100;
#endif
        return true;
    }

#pragma region JSONDECODABLE
    static const char * decoderName() {
        return "Status";
    }

    static constexpr size_t decodingSize() {
        return ProgramSettings::STATUS_JSON_BUFFER_SIZE;
    }

    /** ────────────────────────────────────────────────────────────────────────────
     *  @brief May be used to resume operation in future versions. For now,
     *  status file is used to save valves status for next startup.
     *
     *  @param source
     *  ──────────────────────────────────────────────────────────────────────────── */
    void decodeJSON(const JsonVariant & source) override {
        const JsonArrayConst & source_valves = source[StatusKeys::VALVES].as<JsonArrayConst>();
        valves.resize(source_valves.size());
        copyArray(source_valves, valves.data(), valves.size());
    }

#pragma endregion JSONDECODABLE
#pragma region JSONENCODABLE
    static const char * encoderName() {
        return "Status";
    }

    static constexpr size_t encodingSize() {
        return ProgramSettings::STATUS_JSON_BUFFER_SIZE;
    }

    bool encodeJSON(const JsonVariant & dest) const override {
        using namespace StatusKeys;
        JsonArray doc_valves = dest.createNestedArray(VALVES);
        copyArray(valves.data(), valves.size(), doc_valves);

        // clang-format off
		return dest[VALVES_COUNT].set(valves.size()) 
			&& dest[SENSOR_PRESSURE].set(pressure)
			&& dest[SENSOR_TEMP].set(temperature) 
			&& dest[SENSOR_BARO].set(barometric)
			&& dest[SENSOR_VOLUME].set(waterVolume) 
			&& dest[SENSOR_DEPTH].set(waterDepth)
			&& dest[SENSOR_FLOW].set(waterFlow) 
			&& dest[CURRENT_TASK].set(currentTaskName)
			&& dest[CURRENT_STATE].set(currentStateName) 
            && dest[LOW_BATTERY].set(false)
            && dest[SAMPLE_VOLUME].set(sampleVolume);
        // clang-format on
    }

#pragma endregion JSONENCODABLE
#pragma region PRINTABLE
    size_t printTo(Print & printer) const override {
        StaticJsonDocument<encodingSize()> doc;
        JsonVariant object = doc.to<JsonVariant>();
        encodeJSON(object);
        return serializeJsonPretty(object, printer);
    }
#pragma endregion PRINTABLE
};