#include "luxpowertek.h"
#include "esphome/core/log.h"

namespace esphome {
namespace luxpowertek {

static const char *const TAG = "luxpowertek";

uint16_t LuxPowertekComponent::crc16_modbus(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  while (len--) {
    crc ^= *data++;
    for (uint8_t i = 0; i < 8; i++)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}

void LuxPowertekComponent::setup() {
  ESP_LOGI(TAG, "Connecting to inverter at %s:%u", host_.c_str(), port_); 
}

void LuxPowertekComponent::send_request(uint16_t start_reg) {
  uint8_t pkt[38];
  uint8_t *p = pkt;
  *p++ = 0xA1; *p++ = 0x1A; *p++ = 0x02; *p++ = 0x00;
  *p++ = 0x20; *p++ = 0x00; *p++ = 0x01; *p++ = 0xC2;

  for (char c : dongle_) *p++ = c;
  while (p < pkt + 18) *p++ = 0x00;

  *p++ = 0x12; *p++ = 0x00;  // data_len
  uint8_t *df = p;

  *p++ = 0x01; *p++ = 0x04;  // action_write / read_input
  for (char c : inv_serial_) *p++ = c;
  while (p < df + 12 + 10) *p++ = 0x00;

  *p++ = start_reg & 0xFF; *p++ = start_reg >> 8;
  *p++ = 0x28; *p++ = 0x00;

  uint16_t crc = crc16_modbus(df, 16);
  *p++ = crc & 0xFF; *p++ = crc >> 8;

  if (!client_.connect(host_.c_str(), port_)) {
    ESP_LOGE(TAG, "Failed to connect");
    return;
  }

  client_.write(pkt, 38);
  ESP_LOGD(TAG, "TX Bank %u", start_reg);
}

bool LuxPowertekComponent::read_packet(std::vector<uint8_t> &buf, uint16_t &start_reg) {
  uint32_t t0 = millis();
  while (client_.available() == 0 && millis() - t0 < 1000)
    delay(10);
  while (client_.available())
    buf.push_back(client_.read());
  if (buf.size() < 35) return false;
  start_reg = buf[32] | (buf[33] << 8);
  return true;
}

void LuxPowertekComponent::decode_bank0(const uint8_t *pl, size_t len) {
  if (len < 8) return;
  Bank0Data b{};
  memcpy(&b, pl, sizeof(b));

  float vbat = b.v_bat / 10.0f;
  float soc = b.soc;
  float pdis = b.p_discharge;

  if (sensors_["vbat"]) sensors_["vbat"]->publish_state(vbat);
  if (sensors_["soc"]) sensors_["soc"]->publish_state(soc);
  if (sensors_["p_discharge"]) sensors_["p_discharge"]->publish_state(pdis);

  ESP_LOGI(TAG, "Decoded → Vbat=%.1f V  SOC=%.0f%%  Pdis=%.0f W", vbat, soc, pdis);
}

void LuxPowertekComponent::update() {
  if (!client_.connect(host_.c_str(), port_)) {
    ESP_LOGE(TAG, "Failed to connect");
  }

  send_request(0);
  std::vector<uint8_t> rx;
  uint16_t reg = 0;
  if (read_packet(rx, reg) && reg == 0) {
    decode_bank0(rx.data() + 35, 80);
  }

  client_.stop();
}

}  // namespace luxpowertek
}  // namespace esphome
