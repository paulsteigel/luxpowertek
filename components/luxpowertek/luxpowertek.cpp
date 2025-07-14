#include "luxpowertek.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"   // format_hex_pretty()

namespace esphome {
namespace luxpowertek {

static const char *const TAG = "luxpowertek";

static constexpr size_t HEADER_LEN = 0x1A;        // prefix → before raw data
static constexpr uint16_t WORDS_TO_READ = 0x28;   // 40 words (80 bytes)
static constexpr uint32_t RX_TIMEOUT_MS = 1000;   // 1 s

// -----------------------------------------------------------------------------
//  Setup
// -----------------------------------------------------------------------------
void LuxPowertekComponent::setup() {
  ESP_LOGI(TAG, "LuxPowertek component setup complete");
}

// -----------------------------------------------------------------------------
//  Polling loop
// -----------------------------------------------------------------------------
void LuxPowertekComponent::update() {
  const uint16_t bank_start = 0x0000;   // Only bank‑0 for now

  ESP_LOGD(TAG, "TX Bank 0");
  this->send_request(bank_start);

  std::vector<uint8_t> rx;
  if (!this->receive_packet(rx)) {
    ESP_LOGW(TAG, "No valid response received");
    return;
  }

  if (rx.size() < HEADER_LEN + sizeof(LuxLogDataRawSection1)) {
    ESP_LOGW(TAG, "Packet too short: %u bytes", (unsigned) rx.size());
    return;
  }

  const uint8_t *data = rx.data() + HEADER_LEN;
  const auto *raw = reinterpret_cast<const LuxLogDataRawSection1 *>(data);

  // Decode & publish
  float vbat = raw->v_bat / 10.0f;      // tenths → volts
  float soc  = raw->soc;                // %
  float pdis = raw->p_discharge;        // W

  ESP_LOGD(TAG,
           "Decoded ►  Vbat=%.1f V  SOC=%.0f %%  P_dis=%.0f W",
           vbat, soc, pdis);

  if (vbat_sensor_ != nullptr)
    vbat_sensor_->publish_state(vbat);
  if (soc_sensor_ != nullptr)
    soc_sensor_->publish_state(soc);
  if (p_discharge_sensor_ != nullptr)
    p_discharge_sensor_->publish_state(pdis);
}

// -----------------------------------------------------------------------------
//  Build & send a request frame
// -----------------------------------------------------------------------------
void LuxPowertekComponent::send_request(uint16_t start_address) {
  std::vector<uint8_t> tx;

  auto add16 = [&](uint16_t v) {
    tx.push_back(v >> 8);
    tx.push_back(v & 0xFF);
  };

  tx.push_back(0xA1);            // Prefix
  add16(0x1A);                   // Protocol ver
  add16(0x20);                   // Length (fixed for log request)
  tx.push_back(0x00);            // Address
  tx.push_back(0x01);            // Function

  tx.push_back(0xC2);            // LuxPower fixed byte
  tx.insert(tx.end(), dongle_serial_.begin(), dongle_serial_.end());

  tx.push_back(0x12);
  tx.push_back(0x00);
  tx.push_back(0x01);

  tx.push_back(0x04);            // SN length
  tx.insert(tx.end(), inverter_serial_.begin(), inverter_serial_.end());

  add16(start_address);          // Register start
  add16(WORDS_TO_READ);          // 40 words

  // ------------------------------------------------------------------
  //  Lux checksum: LOW‑byte of simple sum, plus dummy HIGH byte = 0x00
  // ------------------------------------------------------------------
  uint8_t crc = 0;
  for (auto b : tx) crc += b;

  tx.push_back(0x00);   // HIGH byte (Lux ignores)
  tx.push_back(crc);    // LOW  byte

  ESP_LOGD(TAG, "TX Frame: %s", format_hex_pretty(tx).c_str());

  client_.connect(host_.c_str(), port_);
  client_.write(tx.data(), tx.size());
}

// -----------------------------------------------------------------------------
//  Read response into buffer
// -----------------------------------------------------------------------------
bool LuxPowertekComponent::receive_packet(std::vector<uint8_t> &buf) {
  uint32_t start = millis();

  while (client_.connected() && (millis() - start) < RX_TIMEOUT_MS) {
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
