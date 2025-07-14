#pragma once

#include "esphome.h"
#include "esphome/components/sensor/sensor.h"
#include <WiFiClient.h>

namespace esphome {
namespace luxpowertek {

class LuxPowertekComponent : public PollingComponent {
 public:
  LuxPowertekComponent() = default;
  void setup() override;
  void update() override;

  void set_host(const std::string &host) { this->host_ = host; }
  void set_port(uint16_t port) { this->port_ = port; }
  void set_inverter_serial_number(const std::string &serial);

  sensor::Sensor *battery_discharge_sensor{nullptr};
  sensor::Sensor *battery_voltage_sensor{nullptr};
  sensor::Sensor *soc_sensor{nullptr};

 protected:
  void send_request(uint16_t bank);

  std::string host_;   // Hostname or IP string
  uint16_t port_;      // TCP port

  std::string inverter_serial_;  // Serial number of the inverter
  WiFiClient client_;
};

}  // namespace luxpowertek
}  // namespace esphome
