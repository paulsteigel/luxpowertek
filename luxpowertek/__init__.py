import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_HOST,
    CONF_PORT,
    CONF_UPDATE_INTERVAL,
)

CONF_DONGLE_SERIAL = "dongle_serial"
CONF_INVERTER_SN = "inverter_serial_number"

luxpowertek_ns = cg.esphome_ns.namespace("luxpowertek")
LuxPowertekComponent = luxpowertek_ns.class_("LuxPowertekComponent", cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(LuxPowertekComponent),
    cv.Required(CONF_HOST): cv.ipv4,
    cv.Required(CONF_PORT): cv.port,
    cv.Required(CONF_DONGLE_SERIAL): cv.string_strict,
    cv.Required(CONF_INVERTER_SN): cv.string_strict,
    cv.Optional(CONF_UPDATE_INTERVAL, default="20s"): cv.update_interval,
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_host(config[CONF_HOST]))
    cg.add(var.set_port(config[CONF_PORT]))
    cg.add(var.set_dongle_serial(config[CONF_DONGLE_SERIAL]))
    cg.add(var.set_inverter_serial(config[CONF_INVERTER_SN]))
