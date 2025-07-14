#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
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
  void setup() override;
  void update() override;

  void set_soc_sensor(sensor::Sensor *sensor) { this->soc_sensor_ = sensor; }
  void set_vbat_sensor(sensor::Sensor *sensor) { this->vbat_sensor_ = sensor; }
  void set_p_discharge_sensor(sensor::Sensor *sensor) { this->p_discharge_sensor_ = sensor; }

 protected:
  void parse_packet_(const uint8_t *data, size_t length);

  sensor::Sensor *soc_sensor_{nullptr};
  sensor::Sensor *vbat_sensor_{nullptr};
  sensor::Sensor *p_discharge_sensor_{nullptr};
};

}  // namespace luxpowertek
}  // namespace esphome
