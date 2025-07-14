# components/luxpowertek/__init__.py
import esphome.codegen as cg
import esphome.config_validation as cv
#from esphome.core import CORE, coroutine_with_priority
from esphome.const import CONF_ID

DEPENDENCIES = ["wifi"]
AUTO_LOAD = ["sensor", "text_sensor"] # Make sure text_sensor is also loaded
MULTI_CONF = True

luxpowertek_ns = cg.esphome_ns.namespace("luxpowertek")
LuxpowertekComponent = luxpowertek_ns.class_("LuxpowertekComponent", cg.PollingComponent)

CONF_LUXPOWERTEK_ID = "luxpowertek_id"

LUXPOWERTEK_COMPONENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_LUXPOWERTEK_ID): cv.use_id(LuxpowertekComponent),
    }
)

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

    # --- Corrected Code Generation ---
    # We pass the string directly from the config.
    # The C++ side will handle validation and conversion.
    dongle_serial = config["dongle_serial"]
    if len(dongle_serial) != 10:
        raise cv.Invalid("dongle_serial must be 10 characters long")
    cg.add(var.set_dongle_serial(dongle_serial))

    inverter_serial = config["inverter_serial_number"]
    if len(inverter_serial) != 10:
        raise cv.Invalid("inverter_serial_number must be 10 characters long")
    # This call must match the function name in the .h file exactly.
    cg.add(var.set_inverter_serial_number(inverter_serial))
