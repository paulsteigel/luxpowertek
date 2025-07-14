import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    UNIT_PERCENT,
    UNIT_VOLT,
    UNIT_WATT,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_POWER,
    STATE_CLASS_MEASUREMENT,
    CONF_ID,
)

#from . import luxpowertek_ns, CONF_LUXPOWERTEK_ID, LUXPOWERTEK_COMPONENT_SCHEMA
from .const import luxpowertek_ns, CONF_LUXPOWERTEK_ID, LUXPOWERTEK_COMPONENT_SCHEMA

LuxPowertekComponent = luxpowertek_ns.class_("LuxPowertekComponent", cg.Component)

SENSOR_TYPES = {
    "lux_soc": sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        device_class=DEVICE_CLASS_BATTERY,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=0,
    ),
    "lux_vbat": sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=1,
    ),
    "lux_p_discharge": sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=0,
    ),
}

CONFIG_SCHEMA = LUXPOWERTEK_COMPONENT_SCHEMA.extend({
    cv.GenerateID(CONF_ID): cv.declare_id(LuxPowertekComponent),
    **{cv.Optional(key): schema for key, schema in SENSOR_TYPES.items()}
})


async def to_code(config):
    hub = await cg.get_variable(config[CONF_LUXPOWERTEK_ID])

    for key in SENSOR_TYPES:
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(hub, sens))
            #cg.add(hub.register_sensor(key, sens))
