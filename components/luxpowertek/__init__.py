import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

luxpowertek_ns = cg.esphome_ns.namespace("luxpowertek")
LuxPowertekComponent = luxpowertek_ns.class_("LuxPowertekComponent", cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(LuxPowertekComponent),
    cv.Required("host"): cv.ipv4,
    cv.Required("port"): cv.port,
    cv.Required("dongle_serial"): cv.string_strict,
    cv.Required("inverter_serial_number"): cv.string_strict,
    cv.Optional("update_interval", default="20s"): cv.update_interval,
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_host(config["host"]))
    cg.add(var.set_port(config["port"]))
    cg.add(var.set_dongle_serial(config["dongle_serial"]))
    cg.add(var.set_inverter_serial(config["inverter_serial_number"]))
