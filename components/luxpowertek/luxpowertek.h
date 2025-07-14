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

struct LuxLogDataRawSection2 {
  int32_t  e_pv_1_all;
  int32_t  e_pv_2_all;
  int32_t  e_pv_3_all;
  int32_t  e_inv_all;
  int32_t  e_rec_all;
  int32_t  e_chg_all;
  int32_t  e_dischg_all;
  int32_t  e_eps_all;
  int32_t  e_to_grid_all;
  int32_t  e_to_user_all;
  uint32_t fault_code;
  uint32_t warning_code;
  int16_t  t_inner;
  int16_t  t_rad_1;
  int16_t  t_rad_2;
  int16_t  t_bat;
  uint16_t _reserved2;
  uint32_t uptime;
};

struct LuxLogDataRawSection3 {
  uint16_t _reserved3;
  int16_t  max_chg_curr;
  int16_t  max_dischg_curr;
  int16_t  charge_volt_ref;
  int16_t  dischg_cut_volt;
  uint8_t  placeholder[20];
  int16_t  bat_status_inv;
  int16_t  bat_count;
  int16_t  bat_capacity;
  int16_t  bat_current;
  int16_t  reg99;
  int16_t  reg100;
  int16_t  max_cell_volt;
  int16_t  min_cell_volt;
  int16_t  max_cell_temp;
  int16_t  min_cell_temp;
  uint16_t _reserved4;
  int16_t  bat_cycle_count;
  uint8_t  _reserved5[14];
  int16_t  p_load2;
};

struct LuxLogDataRawSection4 {
  uint16_t reg120;
  int16_t  gen_input_volt;
  int16_t  gen_input_freq;
  int16_t  gen_power_watt;
  int16_t  gen_power_day;
  int16_t  gen_power_all;
  uint16_t reg126;
  int16_t  eps_L1_volt;
  int16_t  eps_L2_volt;
  int16_t  eps_L1_watt;
  int16_t  eps_L2_watt;
  uint8_t  placeholder[50];
};

struct LuxLogDataRawSection5 {
  uint8_t  _reserved7[20];
  int16_t  p_load_ongrid;
  int16_t  e_load_day;
  int16_t  e_load_all_l;
  uint8_t  _reserved8[54];
};
#pragma pack(pop)

class LuxPowertekComponent : public PollingComponent {
 public:
  void setup() override;
  void loop() override;
  void update() override;

  void set_host(const std::string &host) { host_ = host; }
  void set_port(uint16_t port) { port_ = port; }
  void set_dongle_serial(const std::string &serial) { dongle_serial_ = serial; }
  void set_inverter_serial_number(const std::string &serial) { inverter_serial_ = serial; }

  void set_lux_vbat_sensor(sensor::Sensor *s) { lux_vbat_sensor_ = s; }
  void set_lux_soc_sensor(sensor::Sensor *s) { lux_soc_sensor_ = s; }
  void set_lux_p_discharge_sensor(sensor::Sensor *s) { lux_p_discharge_sensor_ = s; }

 protected:
  void start_communication();
  void disconnect();
  bool send_request(uint16_t bank);
  void process_frame();
  
  size_t build_read_packet(uint8_t *buf, uint16_t start_reg);
  uint16_t crc16_modbus(const uint8_t *data, size_t length);
  void decode_bank0();

  // Raw data storage
  LuxLogDataRawSection1 bank0_{};
  LuxLogDataRawSection2 bank1_{};
  LuxLogDataRawSection3 bank2_{};
  LuxLogDataRawSection4 bank3_{};
  LuxLogDataRawSection5 bank4_{};

  std::string host_;
  uint16_t port_;
  std::string dongle_serial_;
  std::string inverter_serial_;

  WiFiClient client_;

  sensor::Sensor *lux_vbat_sensor_{nullptr};
  sensor::Sensor *lux_soc_sensor_{nullptr};
  sensor::Sensor *lux_p_discharge_sensor_{nullptr};

  enum CommState { STATE_IDLE, STATE_CONNECTED, STATE_WAITING };
  CommState state_{STATE_IDLE};

  uint8_t current_bank_{0};
  std::vector<uint8_t> rx_buffer_;
  uint32_t request_start_ms_{0};
  uint32_t last_byte_ms_{0};
};

}  // namespace luxpowertek
}  // namespace esphome