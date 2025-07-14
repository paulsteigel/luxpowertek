import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_HOST, CONF_PORT, CONF_UPDATE_INTERVAL

AUTO_LOAD = ["sensor"]
CODEOWNERS = ["@your‑github"]

luxpowertek_ns = cg.esphome_ns.namespace("luxpowertek")
LuxPowertekComponent = luxpowertek_ns.class_(
    "LuxPowertekComponent", cg.PollingComponent
)

CONF_SOC            = "soc_sensor"
CONF_VBAT           = "vbat_sensor"
CONF_BAT_DISCHARGE  = "bat_discharge_sensor"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LuxPowertekComponent),
        cv.Required(CONF_HOST): cv.ipv4,
        cv.Required(CONF_PORT): cv.port,
        cv.Optional(CONF_UPDATE_INTERVAL, default="5s"): cv.update_interval,
        cv.Required(CONF_SOC): sensor.sensor_schema(
            unit_of_measurement="%",
            accuracy_decimals=0,
            device_class="battery",
        ),
        cv.Required(CONF_VBAT): sensor.sensor_schema(
            unit_of_measurement="V",
            accuracy_decimals=1,
            device_class="voltage",
        ),
        cv.Required(CONF_BAT_DISCHARGE): sensor.sensor_schema(
            unit_of_measurement="W",
            accuracy_decimals=0,
            device_class="power",
        ),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_host(config[CONF_HOST]))
    cg.add(var.set_port(config[CONF_PORT]))

    sens_soc = await sensor.new_sensor(config[CONF_SOC])
    cg.add(var.set_soc_sensor(sens_soc))

    sens_vbat = await sensor.new_sensor(config[CONF_VBAT])
    cg.add(var.set_vbat_sensor(sens_vbat))

    sens_bdis = await sensor.new_sensor(config[CONF_BAT_DISCHARGE])
    cg.add(var.set_bat_discharge_sensor(sens_bdis))
