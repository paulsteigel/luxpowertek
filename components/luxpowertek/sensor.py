import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_POWER,
    UNIT_PERCENT,
    UNIT_VOLT,
    UNIT_WATT,
)

luxpowertek_ns = cg.esphome_ns.namespace("luxpowertek")
LuxPowertekComponent = luxpowertek_ns.class_("LuxPowertekComponent", cg.Component)

LuxPowertekSensor = luxpowertek_ns.class_("LuxPowertekSensor", sensor.Sensor)

CONF_TYPE = "type"
TYPES = ["soc", "vbat", "p_discharge"]

CONFIG_SCHEMA = sensor.sensor_schema().extend({
    cv.Required(CONF_TYPE): cv.one_of(*TYPES),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    hub = await cg.get_variable(config[CONF_ID])
    sens = await sensor.new_sensor(config)
    cg.add(hub.register_sensor(config[CONF_TYPE], sens))
