#pragma once
#include "esphome/components/sensor/sensor.h"
#include "esphome.h"
#include <map>
#include <string>

namespace esphome {
namespace luxpowertek {

class LuxPowertekComponent : public PollingComponent {
 public:
  LuxPowertekComponent() : PollingComponent(20000) {}

  void set_host(const std::string &h) { this->host_ = h; }
  void set_port(uint16_t p) { port_ = p; }
  void set_dongle_serial(const std::string &s) { dongle_ = s; }
  void set_inverter_serial(const std::string &s) { inv_serial_ = s; }

  void setup() override;
  void update() override;

  void register_sensor(const std::string &type, sensor::Sensor *s) {
    sensors_[type] = s;
  }

 protected:
  std::string host_;
  uint16_t port_;
  std::string dongle_;
  std::string inv_serial_;
  WiFiClient client_;
  std::map<std::string, sensor::Sensor *> sensors_;

  struct __attribute__((packed)) Bank0Data {
    uint16_t p_discharge;
    uint16_t p_charge;
    uint16_t v_bat;  // ×0.1
    uint8_t  soc;
  };

  void send_request(uint16_t start_reg);
  bool read_packet(std::vector<uint8_t> &buf, uint16_t &start_reg);
  void decode_bank0(const uint8_t *pl, size_t len);
  static uint16_t crc16_modbus(const uint8_t *data, size_t len);
};

}  // namespace luxpowertek
}  // namespace esphome
