#include "luxpowertek.h"
#include "esphome/core/log.h"

namespace esphome {
namespace luxpowertek {

static const char *const TAG = "luxpowertek";

void LuxPowertekComponent::setup() {
  ESP_LOGI(TAG, "LuxPowertek component setup complete");
}

void LuxPowertekComponent::update() {
  ESP_LOGD(TAG, "TX Bank 0");
  this->send_request(0x0000);

  std::vector<uint8_t> response;
  if (!this->receive_packet(response)) {
    ESP_LOGW(TAG, "No valid response received");
    return;
  }

  if (response.size() < 0x1A + sizeof(LuxLogDataRawSection1)) {
    ESP_LOGW(TAG, "Packet too short: %d bytes", response.size());
    return;
  }

  const uint8_t *data = response.data() + 0x1A;
  auto *raw = reinterpret_cast<const LuxLogDataRawSection1 *>(data);

  float vbat = raw->v_bat / 10.0f;
  float soc = raw->soc;
  float p_dis = raw->p_discharge;

  ESP_LOGD(TAG, "Decoded ►  Vbat=%.1f V  SOC=%.0f%%  P_dis=%.0f W", vbat, soc, p_dis);

  if (this->vbat_sensor_ != nullptr)
    this->vbat_sensor_->publish_state(vbat);
  if (this->soc_sensor_ != nullptr)
    this->soc_sensor_->publish_state(soc);
  if (this->p_discharge_sensor_ != nullptr)
    this->p_discharge_sensor_->publish_state(p_dis);
}

void LuxPowertekComponent::send_request(uint16_t start_address) {
  std::vector<uint8_t> tx;

  auto add16 = [&](uint16_t val) {
    tx.push_back(val >> 8);
    tx.push_back(val & 0xFF);
  };

  tx.push_back(0xA1);                  // Prefix
  add16(0x1A);                         // Protocol version
  add16(0x20);                         // Length
  tx.push_back(0x00);                 // Address
  tx.push_back(0x01);                 // Function (request)

  tx.push_back(0xC2);                 // ID for LuxPower?
  tx.insert(tx.end(), dongle_serial_.begin(), dongle_serial_.end());

  tx.push_back(0x12);                 // Translated
  tx.push_back(0x00);                 // Device type?
  tx.push_back(0x01);                 // Function (Log)

  tx.push_back(0x04);                 // Length of inverter serial
  tx.insert(tx.end(), inverter_serial_.begin(), inverter_serial_.end());

  add16(start_address);
  add16(0x28);                        // 40 words = 80 bytes

  uint16_t crc = 0;
  for (auto b : tx)
    crc += b;

  tx.push_back((crc >> 8) & 0xFF);
  tx.push_back(crc & 0xFF);

  client_.connect(host_.c_str(), port_);
  client_.write(tx.data(), tx.size());

  ESP_LOGD(TAG, "TX %s", format_hex_pretty(tx).c_str());
}

bool LuxPowertekComponent::receive_packet(std::vector<uint8_t> &buf) {
  constexpr uint32_t timeout_ms = 1000;
  uint32_t start = millis();

  while (client_.connected() && (millis() - start) < timeout_ms) {
    while (client_.available()) {
      buf.push_back(client_.read());
    }
    delay(10);
  }

  if (buf.empty())
    return false;

  ESP_LOGD(TAG, "RX raw %s", format_hex_pretty(buf).c_str());
  return true;
}

}  // namespace luxpowertek
}  // namespace esphome
