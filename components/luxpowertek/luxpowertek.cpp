#include "luxpowertek.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"   // for format_hex_pretty

namespace esphome {
namespace luxpowertek {

static const char *const TAG = "luxpowertek";

// ----------------------------------------------------------------------------
//  Setup
// ----------------------------------------------------------------------------
void LuxPowertekComponent::setup() {
  ESP_LOGI(TAG, "LuxPowertek component setup complete");
}

// ----------------------------------------------------------------------------
//  Main poll loop
// ----------------------------------------------------------------------------
void LuxPowertekComponent::update() {
  // --- Only bank‑0 (0x0000) is needed for the three sensors ---
  const uint16_t bank = 0x0000;

  ESP_LOGD(TAG, "TX Bank %u", bank);
  this->send_request(bank);

  std::vector<uint8_t> response;
  if (!this->receive_packet(response)) {
    ESP_LOGW(TAG, "No valid response received");
    return;
  }

  // Header (0x1A bytes) + translated + data
  constexpr size_t HEADER_SIZE = 0x1A;
  if (response.size() < HEADER_SIZE + sizeof(LuxLogDataRawSection1)) {
    ESP_LOGW(TAG, "Packet too short: %d bytes", response.size());
    return;
  }

  const uint8_t *data = response.data() + HEADER_SIZE;
  const auto *raw = reinterpret_cast<const LuxLogDataRawSection1 *>(data);

  // ------------------------------------------------------------------
  //  Decode & publish
  // ------------------------------------------------------------------
  float vbat = raw->v_bat / 10.0f;          // tenths of volt
  float soc  = raw->soc;                    // %
  float pdis = raw->p_discharge;            // W

  ESP_LOGD(TAG,
           "Decoded ►  Vbat=%.1f V  SOC=%.0f %%  P_discharge=%.0f W",
           vbat, soc, pdis);

  if (vbat_sensor_ != nullptr)
    vbat_sensor_->publish_state(vbat);

  if (soc_sensor_ != nullptr)
    soc_sensor_->publish_state(soc);

  if (p_discharge_sensor_ != nullptr)
    p_discharge_sensor_->publish_state(pdis);
}

// ----------------------------------------------------------------------------
//  Build & send a single request
// ----------------------------------------------------------------------------
void LuxPowertekComponent::send_request(uint16_t start_address) {
  std::vector<uint8_t> tx;

  auto add16 = [&](uint16_t v) {
    tx.push_back(v >> 8);
    tx.push_back(v & 0xFF);
  };

  tx.push_back(0xA1);              // Prefix
  add16(0x1A);                     // Protocol ver
  add16(0x20);                     // Length
  tx.push_back(0x00);              // Addr
  tx.push_back(0x01);              // Function

  tx.push_back(0xC2);                                  // ID
  tx.insert(tx.end(), dongle_serial_.begin(), dongle_serial_.end());

  tx.push_back(0x12);
  tx.push_back(0x00);
  tx.push_back(0x01);

  tx.push_back(0x04);                                  // inverter SN len
  tx.insert(tx.end(), inverter_serial_.begin(), inverter_serial_.end());

  add16(start_address);          // register start
  add16(0x28);                   // 40 words (80 bytes)

  uint16_t crc = 0;
  for (auto b : tx) crc += b;
  add16(crc);

  client_.connect(host_.c_str(), port_);
  client_.write(tx.data(), tx.size());

  ESP_LOGD(TAG, "TX %s", format_hex_pretty(tx).c_str());
}

// ----------------------------------------------------------------------------
//  Receive helper
// ----------------------------------------------------------------------------
bool LuxPowertekComponent::receive_packet(std::vector<uint8_t> &buf) {
  constexpr uint32_t TIMEOUT_MS = 1000;
  uint32_t start = millis();

  while (client_.connected() && (millis() - start) < TIMEOUT_MS) {
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
