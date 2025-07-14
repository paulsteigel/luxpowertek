#include "luxpowertek.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace luxpowertek {

static const char *const TAG = "luxpowertek";

void LuxPowertekComponent::setup() {
  this->state_ = STATE_IDLE;
  this->rx_buffer_.reserve(512);
}

void LuxPowertekComponent::update() {
  if (this->state_ == STATE_IDLE) {
    this->start_communication();
  } else if (this->state_ == STATE_WAITING && 
             millis() - this->request_start_ms_ > 2000) {
    ESP_LOGW(TAG, "Response timeout, restarting");
    this->disconnect();
    this->state_ = STATE_IDLE;
  }
}

void LuxPowertekComponent::start_communication() {
  ESP_LOGD(TAG, "Starting communication");
  this->disconnect();
  
  if (!this->client_.connect(this->host_.c_str(), this->port_)) {
    ESP_LOGE(TAG, "Connection failed!");
    this->state_ = STATE_IDLE;
    return;
  }

  this->state_ = STATE_CONNECTED;
  this->request_start_ms_ = millis();
  this->send_request(0);  // Start with bank 0
}

void LuxPowertekComponent::send_request(uint16_t start_address) {
  uint8_t request[38];
  size_t len = this->build_read_packet(request, start_address, 40);
  
  this->client_.write(request, len);
  this->state_ = STATE_WAITING;
  this->request_start_ms_ = millis();
  this->last_byte_ms_ = millis();
  this->rx_buffer_.clear();
  
  ESP_LOGD(TAG, "Sent request for bank %d", start_address / 40);
}

size_t LuxPowertekComponent::build_read_packet(uint8_t *buf, uint16_t start_reg, uint16_t qty_reg) {
  uint8_t *p = buf;
  *p++ = 0xA1; *p++ = 0x1A;          // prefix
  *p++ = 0x02; *p++ = 0x00;          // proto
  *p++ = 0x20; *p++ = 0x00;          // frameLen = 32
  *p++ = 0x01;                       // unknown, always 1
  *p++ = 0xC2;                       // TRANSLATED_DATA

  for (uint8_t i = 0; i < 10; i++) *p++ = this->dongle_serial_[i];
  *p++ = 0x12; *p++ = 0x00;          // dataLen = 18

  uint8_t *df = p;                   // data-frame starts
  *p++ = 0x01;                       // ACTION_WRITE (read mode)
  *p++ = 0x04;                       // READ_INPUT
  for (uint8_t i = 0; i < 10; i++) *p++ = this->inverter_serial_[i];
  *p++ = start_reg & 0xFF; *p++ = start_reg >> 8;
  *p++ = qty_reg & 0xFF; *p++ = qty_reg >> 8;

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

void LuxPowertekComponent::loop() {
  if (this->state_ == STATE_IDLE || !this->client_.connected()) {
    return;
  }

  // Handle incoming data
  while (this->client_.available()) {
    uint8_t c = this->client_.read();
    this->rx_buffer_.push_back(c);
    this->last_byte_ms_ = millis();
  }

  // Process if we have data and it's been 50ms since last byte
  if (!this->rx_buffer_.empty() && (millis() - this->last_byte_ms_ > 50)) {
    this->process_frame();
    this->rx_buffer_.clear();
  }

  // Handle timeout
  if (this->state_ == STATE_WAITING && 
      millis() - this->request_start_ms_ > 2000) {
    ESP_LOGW(TAG, "Timeout waiting for response");
    this->disconnect();
    this->state_ = STATE_IDLE;
  }
}

void LuxPowertekComponent::process_frame() {
  if (this->rx_buffer_.size() < 6) {
    ESP_LOGD(TAG, "Frame too short");
    return;
  }

  // Check frame header
  if (this->rx_buffer_[0] != 0xA1 || this->rx_buffer_[1] != 0x1A) {
    ESP_LOGD(TAG, "Invalid frame header");
    return;
  }

  uint16_t frame_len = (this->rx_buffer_[4] | (this->rx_buffer_[5] << 8)) + 6;
  if (this->rx_buffer_.size() < frame_len) {
    ESP_LOGD(TAG, "Incomplete frame");
    return;
  }

  // Extract data section
  uint16_t data_len = this->rx_buffer_[18] | (this->rx_buffer_[19] << 8);
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

  ESP_LOGD(TAG, "Received frame: func=%02X, start_reg=%d, bytes=%d", 
           func, start_reg, byte_count);

  // Only process bank 0 for the requested sensors
  if (func == 0x04 && start_reg == 0 && byte_count >= 80) {
    this->decode_bank0(values);
  }

  // Move to next bank
  uint8_t current_bank = start_reg / 40;
  if (current_bank < 4) {
    this->send_request((current_bank + 1) * 40);
  } else {
    ESP_LOGD(TAG, "All banks processed");
    this->disconnect();
    this->state_ = STATE_IDLE;
  }
}

void LuxPowertekComponent::decode_bank0(const uint8_t *data) {
  auto read_uint16 = [](const uint8_t *p) { return p[0] | (p[1] << 8); };
  auto read_int16 = [](const uint8_t *p) { 
    int16_t value = p[0] | (p[1] << 8);
    return value;
  };

  // Battery voltage (register 4)
  float vbat = read_int16(&data[4*2]) / 10.0f;
  if (this->vbat_sensor_) {
    this->vbat_sensor_->publish_state(vbat);
  }

  // State of Charge (register 5, first byte)
  uint8_t soc = data[10];
  if (this->soc_sensor_) {
    this->soc_sensor_->publish_state(soc);
  }

  // Discharge power (register 13)
  int16_t p_discharge = read_int16(&data[13*2]);
  if (this->p_discharge_sensor_) {
    this->p_discharge_sensor_->publish_state(p_discharge);
  }

  ESP_LOGD(TAG, "Decoded: Vbat=%.1fV, SOC=%d%%, P_discharge=%dW", 
           vbat, soc, p_discharge);
}

void LuxPowertekComponent::disconnect() {
  if (this->client_.connected()) {
    this->client_.stop();
  }
}

}  // namespace luxpowertek
}  // namespace esphome