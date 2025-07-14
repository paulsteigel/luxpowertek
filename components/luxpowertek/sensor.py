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
    STATE_CLASS_MEASUREMENT,
    CONF_UPDATE_INTERVAL,
)
from . import LUXPOWERTEK_COMPONENT_SCHEMA, CONF_LUXPOWERTEK_ID

luxpowertek_ns = cg.esphome_ns.namespace("luxpowertek")
LuxPowertekComponent = luxpowertek_ns.class_("LuxPowertekComponent", cg.Component)

LuxPowertekSensor = luxpowertek_ns.class_("LuxPowertekSensor", sensor.Sensor)

SENSOR_TYPES = {
    # System sensors
    "lux_soc": sensor.sensor_schema(unit_of_measurement=UNIT_PERCENT, device_class=DEVICE_CLASS_BATTERY, state_class=STATE_CLASS_MEASUREMENT, accuracy_decimals=0),
    "lux_vbat": sensor.sensor_schema(unit_of_measurement=UNIT_VOLT, device_class=DEVICE_CLASS_VOLTAGE, state_class=STATE_CLASS_MEASUREMENT, accuracy_decimals=1, icon="mdi:battery"),
    "lux_p_discharge": sensor.sensor_schema(unit_of_measurement=UNIT_WATT, device_class=DEVICE_CLASS_POWER, state_class=STATE_CLASS_MEASUREMENT, accuracy_decimals=0, icon="mdi:battery-minus"),    
}

CONFIG_SCHEMA = cv.All(
    LUXPOWERTEK_COMPONENT_SCHEMA.extend(
        {
            **{cv.Optional(key): schema for key, schema in SENSOR_TYPES.items()},
            cv.Optional(CONF_UPDATE_INTERVAL, default="20s"): cv.update_interval,
        }
    ),    
    cv.has_at_least_one_key(*SENSOR_TYPES.keys()),
)

async def to_code(config):
    hub = await cg.get_variable(config[CONF_LUXPOWERTEK_ID])
    sens = await sensor.new_sensor(config)
    cg.add(hub.register_sensor(config[CONF_TYPE], sens))
