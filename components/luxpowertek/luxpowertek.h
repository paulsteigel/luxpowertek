#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include <WiFiClient.h>
#include <vector>

namespace esphome {
namespace luxpowertek {

#pragma pack(push, 1)

struct LuxLogDataRawSection1 {
  uint16_t status;
  int16_t  v_pv_1;
  int16_t  v_pv_2;
  int16_t  v_pv_3;
  int16_t  v_bat;
  uint8_t  soc;
  uint8_t  soh;
  uint16_t internal_fault;
  int16_t  p_pv_1;
  int16_t  p_pv_2;
  int16_t  p_pv_3;
  int16_t  p_charge;
  int16_t  p_discharge;
  int16_t  v_ac_r;
  int16_t  v_ac_s;
  int16_t  v_ac_t;
  int16_t  f_ac;
  int16_t  p_inv;
  int16_t  p_rec;
  int16_t  rms_current;
  int16_t  pf;
  int16_t  v_eps_r;
  int16_t  v_eps_s;
  int16_t  v_eps_t;
  int16_t  f_eps;
  int16_t  p_to_eps;
  int16_t  apparent_eps_power;
  int16_t  p_to_grid;
  int16_t  p_to_user;
  int16_t  e_pv_1_day;
  int16_t  e_pv_2_day;
  int16_t  e_pv_3_day;
  int16_t  e_inv_day;
  int16_t  e_rec_day;
  int16_t  e_chg_day;
  int16_t  e_dischg_day;
  int16_t  e_eps_day;
  int16_t  e_to_grid_day;
  int16_t  e_to_user_day;
  int16_t  v_bus_1;
  int16_t  v_bus_2;
};

#pragma pack(pop)

class LuxPowertekComponent : public PollingComponent {
 public:
  void loop() override;
  void setup() override;
  void update() override;

  // TCP configuration
  void set_host(const std::string &host) { host_ = host; }
  void set_port(uint16_t port) { port_ = port; }
  void set_dongle_serial(const std::string &serial) { dongle_serial_ = serial; }
  void set_inverter_serial_number(const std::string &serial) { inverter_serial_ = serial; }

  // Sensor registration
  void set_vbat_sensor(sensor::Sensor *s) { vbat_sensor_ = s; }
  void set_soc_sensor(sensor::Sensor *s) { soc_sensor_ = s; }
  void set_p_discharge_sensor(sensor::Sensor *s) { p_discharge_sensor_ = s; }

 protected:
  // Communication
  void start_communication();
  void disconnect();
  size_t build_read_packet(uint8_t *buf, uint16_t start_reg, uint16_t qty_reg);
  uint16_t crc16_modbus(const uint8_t *data, size_t length);
  void decode_bank0(const uint8_t *data);
  
  // State machine
  enum CommState {
    STATE_IDLE,
    STATE_CONNECTED,
    STATE_WAITING
  };

  bool send_request(uint16_t start_address);
  void process_frame_();

  std::string host_;
  uint16_t port_;
  std::string dongle_serial_;
  std::string inverter_serial_;

  WiFiClient client_;

  // Sensor handles
  sensor::Sensor *vbat_sensor_{nullptr};
  sensor::Sensor *soc_sensor_{nullptr};
  sensor::Sensor *p_discharge_sensor_{nullptr};

  std::vector<uint8_t> rx_buffer_;
  uint32_t request_start_ms_{0};
  uint32_t last_byte_ms_{0};
};

}  // namespace luxpowertek
}  // namespace esphome
