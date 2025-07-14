#include "luxpowertek.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace luxpowertek {

static const char *const TAG = "luxpowertek";

void LuxPowertekComponent::setup() {
  this->state_ = STATE_IDLE;
  this->rx_buffer_.reserve(128);
}

void LuxPowertekComponent::update() {
  if (this->state_ == STATE_IDLE) {
    this->current_bank_ = 0;
    this->start_communication();
  }
}

void LuxPowertekComponent::loop() {
  switch (this->state_) {
    case STATE_CONNECTING:
      if (this->client_.connected()) {
        ESP_LOGD(TAG, "Connected to %s:%d", this->host_.c_str(), this->port_);
        this->state_ = STATE_SENDING;
        this->last_byte_ms_ = millis();
      } else if (millis() - this->request_start_ms_ > 15000) {
        ESP_LOGE(TAG, "Connection timeout");
        this->disconnect();
        this->state_ = STATE_IDLE;
      }
      break;

    case STATE_SENDING:
      if (this->send_request(this->current_bank_ * 40)) {
        this->state_ = STATE_WAITING_RESPONSE;
        this->request_start_ms_ = millis();
      } else {
        ESP_LOGE(TAG, "Send failed, disconnecting");
        this->disconnect();
        this->state_ = STATE_IDLE;
      }
      break;

    case STATE_WAITING_RESPONSE:
      // Handle incoming data
      while (this->client_.available()) {
        uint8_t c = this->client_.read();
        this->rx_buffer_.push_back(c);
        this->last_byte_ms_ = millis();
      }

      // Process if we have data and it's been 50ms since last byte
      if (!this->rx_buffer_.empty() && (millis() - this->last_byte_ms_ > 50)) {
        this->state_ = STATE_PROCESSING;
      }

      // Handle timeout
      if (millis() - this->request_start_ms_ > 15000) {
        ESP_LOGW(TAG, "Bank %d response timeout", this->current_bank_);
        this->disconnect();
        this->state_ = STATE_IDLE;
      }
      break;

    case STATE_PROCESSING:
      this->process_frame();
      this->rx_buffer_.clear();
      break;

  }
}

void LuxPowertekComponent::start_communication() {
  this->state_ = STATE_CONNECTING;
  ESP_LOGD(TAG, "Starting communication for bank %d", this->current_bank_);
  this->disconnect();
  
  if (!this->client_.connect(this->host_.c_str(), this->port_)) {
    ESP_LOGE(TAG, "Connection failed!");
    this->state_ = STATE_IDLE;
    return;
  }

  this->state_ = STATE_CONNECTED;  // Use the new state
  this->request_start_ms_ = millis();
  this->last_byte_ms_ = millis();
  
  if (!this->send_request(this->current_bank_ * 40)) {
    ESP_LOGE(TAG, "Sending request failed");
    this->disconnect();
    this->state_ = STATE_IDLE;
  }
}

bool LuxPowertekComponent::send_request(uint16_t start_address) {  
  uint8_t request[38];
  size_t len = this->build_read_packet(request, start_address);

  if (this->client_.write(request, len) != len) {
    ESP_LOGE(TAG, "Send failed!");
    return false;
  }

  this->state_ = STATE_WAITING;
  this->request_start_ms_ = millis();
  this->rx_buffer_.clear();
  
  ESP_LOGD(TAG, "Request sent for bank %d (start: %d)", 
           this->current_bank_, start_address);
  return true;
}

size_t LuxPowertekComponent::build_read_packet(uint8_t *buf, uint16_t start_reg) {
  uint8_t *p = buf;
  *p++ = 0xA1; *p++ = 0x1A;          // prefix
  *p++ = 0x02; *p++ = 0x00;          // proto
  *p++ = 0x20; *p++ = 0x00;          // frameLen = 32
  *p++ = 0x01;                       // unknown, always 1
  *p++ = 0xC2;                       // TRANSLATED_DATA

  // Copy dongle serial (10 bytes)
  for (uint8_t i = 0; i < 10; i++) *p++ = this->dongle_serial_[i];
  *p++ = 0x12; *p++ = 0x00;          // dataLen = 18

  uint8_t *df = p;                   // data-frame starts
  *p++ = 0x01;                       // ACTION_WRITE (read mode)
  *p++ = 0x04;                       // READ_INPUT
  // Copy inverter serial (10 bytes)
  for (uint8_t i = 0; i < 10; i++) *p++ = this->inverter_serial_[i];
  *p++ = start_reg & 0xFF; *p++ = start_reg >> 8;
  *p++ = 0x28; *p++ = 0x00;          // register count: 40 (0x28)

  uint16_t crc = this->crc16_modbus(df, 16);
  *p++ = crc & 0xFF; *p++ = crc >> 8;

  return p - buf;
}

uint16_t LuxPowertekComponent::crc16_modbus(const uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void LuxPowertekComponent::process_frame() {
  // Minimum frame size: header + length + data section
  if (this->rx_buffer_.size() < 22) {
    ESP_LOGD(TAG, "Frame too short: %d bytes", this->rx_buffer_.size());
    return;
  }

  // Check frame header
  if (this->rx_buffer_[0] != 0xA1 || this->rx_buffer_[1] != 0x1A) {
    ESP_LOGD(TAG, "Invalid frame header: %02X %02X", 
             this->rx_buffer_[0], this->rx_buffer_[1]);
    return;
  }

  // Extract frame length
  uint16_t frame_len = this->rx_buffer_[4] | (this->rx_buffer_[5] << 8);
  if (this->rx_buffer_.size() < frame_len + 6) {
    ESP_LOGD(TAG, "Incomplete frame: expected %d, got %d", 
             frame_len + 6, this->rx_buffer_.size());
    return;
  }

  // Extract data section
  uint16_t data_len = this->rx_buffer_[18] | (this->rx_buffer_[19] << 8);
  if (data_len < 18) {
    ESP_LOGD(TAG, "Invalid data length: %d", data_len);
    return;
  }

  const uint8_t *data_frame = &this->rx_buffer_[20];
  
  // Verify CRC
  uint16_t crc_calc = this->crc16_modbus(data_frame, data_len - 2);
  uint16_t crc_recv = data_frame[data_len - 2] | (data_frame[data_len - 1] << 8);
  
  if (crc_calc != crc_recv) {
    ESP_LOGD(TAG, "CRC mismatch: calc=%04X, recv=%04X", crc_calc, crc_recv);
    return;
  }

  // Process valid frame
  uint8_t func = data_frame[1];
  uint16_t start_reg = data_frame[12] | (data_frame[13] << 8);
  uint8_t byte_count = data_frame[14];
  const uint8_t *values = &data_frame[15];

  ESP_LOGD(TAG, "Received bank %d: func=%02X, start_reg=%d, bytes=%d", 
           this->current_bank_, func, start_reg, byte_count);

  // Only process valid responses
  if (func != 0x04 || byte_count < 80) {
    ESP_LOGW(TAG, "Unexpected response format");
    return;
  }

  // Store data in appropriate struct
  switch (this->current_bank_) {
    case 0:
      memcpy(&bank0_, values, sizeof(bank0_));
      break;
    case 1:
      memcpy(&bank1_, values, sizeof(bank1_));
      break;
    case 2:
      memcpy(&bank2_, values, sizeof(bank2_));
      break;
    case 3:
      memcpy(&bank3_, values, sizeof(bank3_));
      break;
    case 4:
      memcpy(&bank4_, values, sizeof(bank4_));
      break;
    default:
      ESP_LOGE(TAG, "Invalid bank index: %d", this->current_bank_);
      break;
  }

  // Move to next bank or finish
  if (this->current_bank_ < 4) {
    this->current_bank_++;
    this->send_request(this->current_bank_ * 40);
  } else {
    ESP_LOGD(TAG, "All banks received");
    this->decode_bank0();
    this->disconnect();
    this->state_ = STATE_IDLE;
  }
}

void LuxPowertekComponent::decode_bank0() {
  // Convert from big-endian to host byte order
  auto be_to_host = [](auto &value) {
    uint8_t *bytes = reinterpret_cast<uint8_t*>(&value);
    for (size_t i = 0; i < sizeof(value)/2; i++) {
      std::swap(bytes[i], bytes[sizeof(value)-1-i]);
    }
  };

  // Convert all values in the struct
  be_to_host(bank0_.status);
  be_to_host(bank0_.v_pv_1);
  be_to_host(bank0_.v_pv_2);
  be_to_host(bank0_.v_pv_3);
  be_to_host(bank0_.v_bat);
  be_to_host(bank0_.internal_fault);
  be_to_host(bank0_.p_pv_1);
  be_to_host(bank0_.p_pv_2);
  be_to_host(bank0_.p_pv_3);
  be_to_host(bank0_.p_charge);
  be_to_host(bank0_.p_discharge);
  // Continue for all struct members as needed...

  // Bank0 sensors
  publish_state_("lux_battery_voltage", bank0_.v_bat / 10.0f);
  publish_state_("lux_battery_percent", static_cast<float>(bank0_.soc));
  publish_state_("lux_battery_discharge", static_cast<float>(bank0_.p_discharge));

  ESP_LOGD(TAG, "Decoded: lux_battery_voltage=%.1fV, lux_battery_percent=%d%%, lux_battery_discharge=%dW", 
           bank0_.v_bat / 10.0f, bank0_.soc, bank0_.p_discharge);
}

void LuxPowertekComponent::disconnect() {
  if (this->client_.connected()) {
    this->client_.stop();
    ESP_LOGD(TAG, "Disconnected");
  }
}

}  // namespace luxpowertek
}  // namespace esphome