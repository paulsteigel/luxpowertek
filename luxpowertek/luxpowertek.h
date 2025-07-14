#pragma once
#include "esphome.h"

namespace esphome {
namespace luxpowertek {

class LuxPowertekComponent : public PollingComponent {
 public:
  LuxPowertekComponent() : PollingComponent(5000) {}  // default 5 s

  void set_host(IPAddress h) { host_ = h; }
  void set_port(uint16_t p) { port_ = p; }

  void set_soc_sensor(sensor::Sensor *s) { soc_sensor_ = s; }
  void set_vbat_sensor(sensor::Sensor *s) { vbat_sensor_ = s; }
  void set_bat_discharge_sensor(sensor::Sensor *s) { bat_discharge_sensor_ = s; }

  void setup() override;
  void update() override;

 protected:
  IPAddress host_;
  uint16_t port_;
  WiFiClient client_;
  sensor::Sensor *soc_sensor_{nullptr};
  sensor::Sensor *vbat_sensor_{nullptr};
  sensor::Sensor *bat_discharge_sensor_{nullptr};

  // ░░ raw packet helpers ░░
  static uint16_t crc16_modbus(const uint8_t *data, size_t len);

  // ─── minimal struct for BANK‑0 (first 20 registers we need) ───
  struct __attribute__((packed)) Bank0Data {
    uint8_t prefix[20];      // we skip straight to needed regs
    uint16_t p_discharge;    // register 2  (W)
    uint16_t p_charge;       // register 3  (W) – not used yet
    uint16_t v_bat;          // register 4  (0.1 V)
    uint8_t  soc;            // register 5  (%, low byte)
    uint8_t  pad;
    // ... rest ignored for now
  };

  void send_request(uint16_t start_reg);
  bool read_packet(std::vector<uint8_t> &buf, uint16_t &start_reg);
  void decode_bank0(const uint8_t *payload, size_t len);
};

}  // namespace luxpowertek
}  // namespace esphome
