#include "luxpowertek.h"
#include "esphome/core/log.h"

namespace esphome {
namespace luxpowertek {

static const char *const TAG = "luxpowertek";

void LuxPowertekComponent::setup() {
  ESP_LOGI(TAG, "LuxPowertek setup complete");
}

void LuxPowertekComponent::update() {
  ESP_LOGD(TAG, "Polling LuxPowertek inverter...");

  // Simulated: replace this with your actual TCP receive buffer
  uint8_t example_data[sizeof(LuxLogDataRawSection1)] = {0};
  // Fill with real data in actual implementation

  // Parse as if the full raw section 1 arrived
  this->parse_packet_(example_data, sizeof(LuxLogDataRawSection1));
}

void LuxPowertekComponent::parse_packet_(const uint8_t *data, size_t length) {
  if (length < sizeof(LuxLogDataRawSection1)) {
    ESP_LOGW(TAG, "Data too short: %u bytes", (unsigned) length);
    return;
  }

  auto *raw = reinterpret_cast<const LuxLogDataRawSection1 *>(data);

  // SOC
  if (this->soc_sensor_ != nullptr) {
    this->soc_sensor_->publish_state(raw->soc);
    ESP_LOGD(TAG, "Published SOC: %u%%", raw->soc);
  }

  // Battery voltage
  if (this->vbat_sensor_ != nullptr) {
    float vbat = raw->v_bat / 10.0f;
    this->vbat_sensor_->publish_state(vbat);
    ESP_LOGD(TAG, "Published Vbat: %.1fV", vbat);
  }

  // Battery discharge
  if (this->p_discharge_sensor_ != nullptr) {
    this->p_discharge_sensor_->publish_state(raw->p_discharge);
    ESP_LOGD(TAG, "Published Battery Discharge: %dW", raw->p_discharge);
  }
}

}  // namespace luxpowertek
}  // namespace esphome
