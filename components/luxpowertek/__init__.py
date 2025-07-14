import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from .const import luxpowertek_ns, LuxPowertekComponent

DEPENDENCIES = ["wifi"]

luxpowertek_ns = cg.esphome_ns.namespace("luxpowertek")
LuxpowertekComponent = luxpowertek_ns.class_("LuxPowertekComponent", cg.PollingComponent)

CONF_LUXPOWERTEK_ID = "luxpowertek_id"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LuxpowertekComponent),
            cv.Required("host"): cv.string,
            cv.Required("port"): cv.port,
            cv.Required("dongle_serial"): cv.string,
            cv.Required("inverter_serial_number"): cv.string,
        }
    )
    .extend(cv.polling_component_schema("20s"))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_host(config["host"]))
    cg.add(var.set_port(config["port"]))
    cg.add(var.set_dongle_serial(config["dongle_serial"]))
    cg.add(var.set_inverter_serial_number(config["inverter_serial_number"]))
