#include "luxpowertek.h"
#include "esphome/core/log.h"

namespace esphome {
namespace luxpowertek {

static const char *const TAG = "luxpowertek";

// ========== Setup ==========
void LuxPowertekComponent::setup() {
  ESP_LOGI(TAG, "LuxPowertek component setup complete");
}

// ========== Update ==========
void LuxPowertekComponent::update() {
  ESP_LOGD(TAG, "TX Bank 0");
  this->send_request(0x0000);

  std::vector<uint8_t> response;
  if (!this->receive_packet(response)) {
    ESP_LOGW(TAG, "No valid response received");
    return;
  }

  // Must be enough to contain section1
  const size_t data_offset = 0x1A;
  if (response.size() < data_offset + sizeof(LuxLogDataRawSection1)) {
    ESP_LOGW(TAG, "Response too short: %d bytes", response.size());
    return;
  }

  const uint8_t *data = response.data() + data_offset;
  auto *raw = reinterpret_cast<const LuxLogDataRawSection1 *>(data);

  float soc = raw->soc;
  float vbat = raw->v_bat / 10.0f;
  float pdis = raw->p_discharge;

  ESP_LOGD(TAG, "Decoded: SOC=%.0f%% Vbat=%.1fV PDis=%.0fW", soc, vbat, pdis);

  if (soc_sensor_ != nullptr)
    soc_sensor_->publish_state(soc);

  if (vbat_sensor_ != nullptr)
    vbat_sensor_->publish_state(vbat);

  if (p_discharge_sensor_ != nullptr)
    p_discharge_sensor_->publish_state(pdis);
}

// ========== Send Request ==========
void LuxPowertekComponent::send_request(uint16_t start_address) {
  std::vector<uint8_t> tx;

  auto add16 = [&](uint16_t val) {
    tx.push_back(val >> 8);
    tx.push_back(val & 0xFF);
  };

  tx.push_back(0xA1);               // Prefix
  add16(0x001A);                    // Protocol Version
  add16(0x0020);                    // Packet Length
  tx.push_back(0x00);              // Address
  tx.push_back(0x01);              // Function

  tx.push_back(0xC2);              // ID Prefix
  tx.insert(tx.end(), dongle_serial_.begin(), dongle_serial_.end());

  tx.push_back(0x12);              // Translated Data
  tx.push_back(0x00);              // Device Type
  tx.push_back(0x01);              // Function = Log Read

  tx.push_back(0x04);              // Inverter SN length
  tx.insert(tx.end(), inverter_serial_.begin(), inverter_serial_.end());

  add16(start_address);            // Start address
  add16(0x28);                     // Length (40 words = 80 bytes)

  // CRC - simple sum of bytes
  uint16_t crc = 0;
  for (auto b : tx)
    crc += b;

  tx.push_back((crc >> 8) & 0xFF);
  tx.push_back(crc & 0xFF);

  // Send over TCP
  if (!client_.connect(host_.c_str(), port_)) {
    ESP_LOGW(TAG, "TCP connect failed to %s:%u", host_.c_str(), port_);
    return;
  }

  client_.write(tx.data(), tx.size());
  ESP_LOGD(TAG, "TX Frame: %s", format_hex_pretty(tx).c_str());
  delay(100);  // allow time for inverter to respond
}

// ========== Receive Packet ==========
bool LuxPowertekComponent::receive_packet(std::vector<uint8_t> &buf) {
  constexpr uint32_t timeout_ms = 1000;
  uint32_t start_time = millis();

  while (millis() - start_time < timeout_ms) {
    while (client_.available()) {
      buf.push_back(client_.read());
    }
    if (!buf.empty())
      break;

    delay(10);
  }

  if (buf.empty())
    return false;

  ESP_LOGD(TAG, "RX Raw: %s", format_hex_pretty(buf).c_str());
  return true;
}

}  // namespace luxpowertek
}  // namespace esphome
