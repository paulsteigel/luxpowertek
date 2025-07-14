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

  // Parse only if response is long enough
  if (response.size() < 0x1A + sizeof(LuxLogDataRawSection1)) {
    ESP_LOGW(TAG, "Packet too short: %d bytes", response.size());
    return;
  }

  const uint8_t *data = response.data() + 0x1A;
  auto *raw = reinterpret_cast<const LuxLogDataRawSection1 *>(data);

  float vbat = raw->v_bat / 10.0f;
  float soc = raw->soc;
  float p_dis = raw->p_discharge;

  ESP_LOGD(TAG, "Decoded ► Vbat=%.1f V, SOC=%.0f %%, P_discharge=%.0f W", vbat, soc, p_dis);

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
    tx.push_back((val >> 8) & 0xFF);
    tx.push_back(val & 0xFF);
  };

  tx.push_back(0xA1);                 // Prefix
  add16(0x1A);                        // Protocol Version
  add16(0x20);                        // Length
  tx.push_back(0x00);                // Address
  tx.push_back(0x01);                // Function code

  tx.push_back(0xC2);                // Device ID
  tx.insert(tx.end(), dongle_serial_.begin(), dongle_serial_.end());

  tx.push_back(0x12);                // Translated Data marker
  tx.push_back(0x00);                // Device type
  tx.push_back(0x01);                // Device Function (log)

  tx.push_back(0x04);                // Inverter SN Length
  tx.insert(tx.end(), inverter_serial_.begin(), inverter_serial_.end());

  add16(start_address);              // Start Register
  add16(0x28);                       // Length: 0x28 (40 registers = 80 bytes)

  // Add checksum (simple sum of all bytes)
  uint16_t crc = 0;
  for (uint8_t b : tx)
    crc += b;

  tx.push_back((crc >> 8) & 0xFF);
  tx.push_back(crc & 0xFF);

  ESP_LOGD(TAG, "TX Frame: %s", format_hex_pretty(tx).c_str());

  // Attempt connection
  if (!client_.connect(host_.c_str(), port_)) {
    ESP_LOGW(TAG, "Connection to inverter failed");
    return;
  }

  client_.write(tx.data(), tx.size());
  client_.flush();  // Ensure data is sent
  delay(200);       // Allow inverter time to respond
}

bool LuxPowertekComponent::receive_packet(std::vector<uint8_t> &buf) {
  uint32_t start = millis();
  constexpr uint32_t timeout = 2000;

  while ((millis() - start) < timeout) {
    while (client_.available()) {
      int byte = client_.read();
      buf.push_back(byte);
    }

    if (!buf.empty()) {
      ESP_LOGD(TAG, "RX Partial: %s", format_hex_pretty(buf).c_str());
    }

    delay(10);  // keep polling
  }

  if (buf.empty()) {
    ESP_LOGW(TAG, "No data received at all");
    return false;
  }

  if (buf[0] != 0xA1 || buf[4] != 0x05) {
    ESP_LOGW(TAG, "Invalid response header: %s", format_hex_pretty(buf).c_str());
    return false;
  }

  ESP_LOGD(TAG, "RX raw %s", format_hex_pretty(buf).c_str());
  return true;
}


}  // namespace luxpowertek
}  // namespace esphome
