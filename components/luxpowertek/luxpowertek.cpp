#include "luxpowertek.h"
#include "esphome/core/log.h"

namespace esphome {
namespace luxpowertek {

static const char *const TAG = "luxpowertek";

constexpr uint16_t WORDS_TO_READ   = 0x28;   // 40 registers = 80 bytes
constexpr size_t   HEADER_SIZE     = 0x1A;   // bytes before raw bank
constexpr uint32_t RESPONSE_TIMEOUT_MS = 3000;    // 3 s to finish frame
constexpr uint32_t QUIET_GAP_MS    = 100;   // quiet gap to assume frame end

// ----------------------------------------------------------------------------
//  Simple state‑machine
// ----------------------------------------------------------------------------
enum CommState { STATE_IDLE, STATE_WAITING };

void LuxPowertekComponent::setup() {
  ESP_LOGI(TAG, "LuxPowertek component setup complete");
  this->state_ = STATE_IDLE;
}

void LuxPowertekComponent::update() {
  const uint16_t bank_start = 0x0000;   // only Bank‑0 for now

  switch (this->state_) {
    // ----------------------------------------------------------
    // 1) Idle  →  Send request
    // ----------------------------------------------------------
    case STATE_IDLE: {
      this->rx_buffer_.clear();
      if (!this->send_request(bank_start)) {
        ESP_LOGW(TAG, "Send failed");
        return;
      }
      this->request_start_ms_ = millis();
      this->last_byte_ms_ = millis();
      this->state_ = STATE_WAITING;
      break;
    }

    // ----------------------------------------------------------
    // 2) Waiting  →  Collect data non‑blocking
    // ----------------------------------------------------------
    case STATE_WAITING: {
      while (client_.available()) {
        int b = client_.read();
        this->rx_buffer_.push_back(static_cast<uint8_t>(b));
        this->last_byte_ms_ = millis();
      }

      // If we got bytes and they stopped coming for QUIET_GAP_MS → process
      if (!rx_buffer_.empty() &&
          (millis() - this->last_byte_ms_) > QUIET_GAP_MS) {

        this->process_frame_();
        client_.stop();
        this->state_ = STATE_IDLE;
        break;
      }

      // Timeout
      if ((millis() - this->request_start_ms_) > RESPONSE_TIMEOUT_MS) {
        ESP_LOGW(TAG, "No response (timeout)");
        client_.stop();
        this->state_ = STATE_IDLE;
      }
      break;
    }
  }
}

// ----------------------------------------------------------------------------
//  Build & send request  (returns true if TCP connect ok)
// ----------------------------------------------------------------------------
bool LuxPowertekComponent::send_request(uint16_t start_address) {
  std::vector<uint8_t> tx;

  auto add16 = [&](uint16_t v) {
    tx.push_back(v >> 8);
    tx.push_back(v & 0xFF);
  };

  tx.push_back(0xA1);
  add16(0x1A);          // protocol
  add16(0x20);          // length fixed
  tx.push_back(0x00);
  tx.push_back(0x01);

  tx.push_back(0xC2);
  tx.insert(tx.end(), dongle_serial_.begin(), dongle_serial_.end());

  tx.push_back(0x12);
  tx.push_back(0x00);
  tx.push_back(0x01);

  tx.push_back(0x04);
  tx.insert(tx.end(), inverter_serial_.begin(), inverter_serial_.end());

  add16(start_address);
  add16(WORDS_TO_READ);

  uint16_t crc = 0;
  for (auto b : tx) crc += b;
  tx.push_back((crc >> 8) & 0xFF);
  tx.push_back(crc & 0xFF);

  client_.stop();  // ensure closed
  if (!client_.connect(host_.c_str(), port_)) {
    ESP_LOGW(TAG, "TCP connect to %s:%u failed", host_.c_str(), port_);
    return false;
  }

  client_.write(tx.data(), tx.size());
  ESP_LOGD(TAG, "TX Frame: %s", format_hex_pretty(tx).c_str());
  return true;
}

// ----------------------------------------------------------------------------
//  Process a complete frame in rx_buffer_
// ----------------------------------------------------------------------------
void LuxPowertekComponent::process_frame_() {
  if (rx_buffer_.size() < HEADER_SIZE + sizeof(LuxLogDataRawSection1)) {
    ESP_LOGW(TAG, "Frame too short (%u bytes)", (unsigned) rx_buffer_.size());
    return;
  }

  if (rx_buffer_[0] != 0xA1) {
    ESP_LOGW(TAG, "Bad prefix 0x%02X", rx_buffer_[0]);
    return;
  }

  // Basic sanity on response type (byte 4 should be 0x05 for reply)
  if (rx_buffer_.size() >= 5 && rx_buffer_[4] != 0x05) {
    ESP_LOGW(TAG, "Unexpected response type 0x%02X", rx_buffer_[4]);
    return;
  }

  const uint8_t *data = rx_buffer_.data() + HEADER_SIZE;
  auto *raw = reinterpret_cast<const LuxLogDataRawSection1 *>(data);

  float vbat = raw->v_bat / 10.0f;
  float soc  = raw->soc;
  float pdis = raw->p_discharge;

  ESP_LOGD(TAG, "Decoded ► Vbat=%.1f V  SOC=%.0f %%  P_dis=%.0f W", vbat, soc, pdis);

  if (vbat_sensor_)        vbat_sensor_->publish_state(vbat);
  if (soc_sensor_)         soc_sensor_->publish_state(soc);
  if (p_discharge_sensor_) p_discharge_sensor_->publish_state(pdis);
}

// ----------------------------------------------------------------------------
//  Receive helper is no longer needed (logic moved into state machine)
// ----------------------------------------------------------------------------

}  // namespace luxpowertek
}  // namespace esphome
