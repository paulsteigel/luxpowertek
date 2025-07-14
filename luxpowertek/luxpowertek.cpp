#include "luxpowertek.h"
#include "esphome/core/log.h"

namespace esphome {
namespace luxpowertek {

static const char *const TAG = "luxpowertek";

static const char DONGLE_ID[10]   = "BA3250069";    // change!
static const char INV_SERIAL[10]  = "3253631886";   // change!

// ────────────────────────────────────────────────────────────────
uint16_t LuxPowertekComponent::crc16_modbus(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  while (len--) {
    crc ^= *data++;
    for (uint8_t i = 0; i < 8; i++)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}

// ────────────────────────────────────────────────────────────────
void LuxPowertekComponent::setup() {
  ESP_LOGI(TAG, "LuxPowertek component init → %s:%u",
           host_.toString().c_str(), port_);
}

// ─── build / send one 38‑byte request ───────────────────────────
void LuxPowertekComponent::send_request(uint16_t start_reg) {
  uint8_t pkt[38];
  uint8_t *p = pkt;
  // header
  *p++ = 0xA1; *p++ = 0x1A;
  *p++ = 0x02; *p++ = 0x00;
  *p++ = 0x20; *p++ = 0x00;
  *p++ = 0x01;         // unknown
  *p++ = 0xC2;         // translated_data
  for (uint8_t i = 0; i < 10; i++) *p++ = DONGLE_ID[i];
  *p++ = 0x12; *p++ = 0x00;  // data_len = 18
  // data frame (18 B)
  uint8_t *df = p;
  *p++ = 0x01;  // action_write
  *p++ = 0x04;  // read_input
  for (uint8_t i = 0; i < 10; i++) *p++ = INV_SERIAL[i];
  *p++ = start_reg & 0xFF; *p++ = start_reg >> 8;
  *p++ = 0x28;           // qty_lo  (40 regs)
  *p++ = 0x00;           // qty_hi
  uint16_t crc = crc16_modbus(df, 16);
  *p++ = crc & 0xFF; *p++ = crc >> 8;

  if (!client_.connected()) client_.connect(host_, port_);
  client_.write(pkt, 38);
  ESP_LOGD(TAG, "TX 38B start_reg=%u", start_reg);
}

// ─── read one frame (blocking ~1 s) ─────────────────────────────
bool LuxPowertekComponent::read_packet(std::vector<uint8_t> &buf,
                                       uint16_t &start_reg) {
  uint32_t t0 = millis();
  while (client_.available() == 0 && millis() - t0 < 1000)
    yield();

  while (client_.available())
    buf.push_back(client_.read());

  if (buf.size() < 26) return false;          // too small
  start_reg = buf[32] | (buf[33] << 8);       // inside data‑frame
  ESP_LOGD(TAG, "RX %zuB start_reg=%u", buf.size(), start_reg);
  return true;
}

// ─── PollingComponent → update() every interval ────────────────
void LuxPowertekComponent::update() {
  // only BANK‑0 for now
  if (!client_.connect(host_, port_)) {
    ESP_LOGW(TAG, "TCP connect failed");
    return;
  }

  send_request(0);  // bank‑0
  std::vector<uint8_t> rx;
  uint16_t reg = 0;
  if (!read_packet(rx, reg)) {
    ESP_LOGW(TAG, "No packet");
    client_.stop();
    return;
  }
  if (reg == 0)
    decode_bank0(rx.data() + 35, 80);  // payload offset
  client_.stop();
}

// ─── decode BANK‑0 subset ───────────────────────────────────────
void LuxPowertekComponent::decode_bank0(const uint8_t *pl, size_t len) {
  if (len < sizeof(Bank0Data)) {
    ESP_LOGW(TAG, "Payload too short");
    return;
  }
  const Bank0Data *b = reinterpret_cast<const Bank0Data *>(pl - 20);
  float vbat = b->v_bat / 10.0f;
  float soc  = b->soc;
  float p_dis = b->p_discharge;

  if (vbat_sensor_) vbat_sensor_->publish_state(vbat);
  if (soc_sensor_)  soc_sensor_->publish_state(soc);
  if (bat_discharge_sensor_) bat_discharge_sensor_->publish_state(p_dis);

  ESP_LOGI(TAG, "Decoded → Vbat=%.1f V  SOC=%.0f%%  Pdis=%.0f W",
           vbat, soc, p_dis);
}

}  // namespace luxpowertek
}  // namespace esphome
