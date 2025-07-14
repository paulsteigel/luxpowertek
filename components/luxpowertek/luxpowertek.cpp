#include "luxpowertek.h"
#include "esphome/core/log.h"

namespace esphome {
namespace luxpowertek {

static const char *const TAG = "luxpowertek";

void LuxPowertekComponent::setup() {
  ESP_LOGI(TAG, "Connecting to inverter at %s:%u", this->host_.c_str(), this->port_);
  // Normally you'd connect/setup here if not polling
}

void LuxPowertekComponent::update() {
  ESP_LOGD(TAG, "TX Bank 0");
  if (!this->client_.connect(this->host_.c_str(), this->port_)) {
    ESP_LOGW(TAG, "Connection to inverter failed");
    return;
  }

  // Dummy data simulation for now
  if (this->battery_discharge_sensor != nullptr)
    this->battery_discharge_sensor->publish_state(123.4);

  if (this->battery_voltage_sensor != nullptr)
    this->battery_voltage_sensor->publish_state(51.6);

  if (this->soc_sensor != nullptr)
    this->soc_sensor->publish_state(82);
}

void LuxPowertekComponent::send_request(uint16_t bank) {
  ESP_LOGD(TAG, "Sending request for bank %u", bank);
  if (!this->client_.connect(this->host_.c_str(), this->port_)) {
    ESP_LOGW(TAG, "Connection to inverter failed");
    return;
  }

  // Example: You can send bytes over TCP here
  // this->client_.write(...);
}

void LuxPowertekComponent::set_inverter_serial_number(const std::string &serial) {
  this->inverter_serial_ = serial;
  ESP_LOGI(TAG, "Inverter Serial Number Set: %s", this->inverter_serial_.c_str());
}

}  // namespace luxpowertek
}  // namespace esphome
