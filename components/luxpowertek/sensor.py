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
YAML_TO_C_NAMES = {
    # System sensors
    "lux_firmware_version": "lux_firmware_version",
    "lux_inverter_model": "lux_inverter_model",
    "lux_status_text": "lux_status_text",
    "lux_battery_status_text": "lux_battery_status_text",
    "lux_data_last_received_time": "lux_data_last_received_time",
    "inverter_serial_number": "inverter_serial_number",   
    
    # Section1 sensors
    "lux_current_solar_voltage_1": "lux_current_solar_voltage_1",
    "lux_current_solar_voltage_2": "lux_current_solar_voltage_2",
    "lux_current_solar_voltage_3": "lux_current_solar_voltage_3",
    "lux_battery_voltage": "lux_battery_voltage",
    "lux_battery_percent": "lux_battery_percent",
    "soh": "soh",
    "lux_internal_fault": "lux_internal_fault",
    "lux_current_solar_output_1": "lux_current_solar_output_1",
    "lux_current_solar_output_2": "lux_current_solar_output_2",
    "lux_current_solar_output_3": "lux_current_solar_output_3",
    "lux_battery_charge": "lux_battery_charge",
    "lux_battery_discharge": "lux_battery_discharge",
    "lux_grid_voltage_r": "lux_grid_voltage_r",
    "lux_grid_voltage_s": "lux_grid_voltage_s",
    "lux_grid_voltage_t": "lux_grid_voltage_t",
    "lux_grid_frequency_live": "lux_grid_frequency_live",
    "lux_grid_voltage_live": "lux_grid_voltage_live",
    "lux_power_from_inverter_live": "lux_power_from_inverter_live",
    "lux_power_to_inverter_live": "lux_power_to_inverter_live",
    "lux_power_current_clamp": "lux_power_current_clamp",
    "grid_power_factor": "grid_power_factor",
    "eps_voltage_r": "eps_voltage_r",
    "eps_voltage_s": "eps_voltage_s",
    "eps_voltage_t": "eps_voltage_t",
    "eps_frequency": "eps_frequency",
    "lux_power_to_eps": "lux_power_to_eps",
    #"apparent_eps_power": "apparent_eps_power",
    "lux_power_to_grid_live": "lux_power_to_grid_live",
    "lux_power_from_grid_live": "lux_power_from_grid_live",
    "lux_daily_solar_array_1": "lux_daily_solar_array_1",
    "lux_daily_solar_array_2": "lux_daily_solar_array_2",
    "lux_daily_solar_array_3": "lux_daily_solar_array_3",
    "lux_power_from_inverter_daily": "lux_power_from_inverter_daily",
    "lux_power_to_inverter_daily": "lux_power_to_inverter_daily",
    "lux_daily_battery_charge": "lux_daily_battery_charge",
    "lux_daily_battery_discharge": "lux_daily_battery_discharge",
    "lux_power_to_eps_daily": "lux_power_to_eps_daily",
    "lux_power_to_grid_daily": "lux_power_to_grid_daily",
    "lux_power_from_grid_daily": "lux_power_from_grid_daily",
    "bus1_voltage": "bus1_voltage",
    "bus2_voltage": "bus2_voltage",
    "lux_current_solar_output": "lux_current_solar_output",
    "lux_daily_solar": "lux_daily_solar",
    "lux_power_to_home": "lux_power_to_home",
    "lux_battery_flow": "lux_battery_flow",
    "lux_grid_flow": "lux_grid_flow",
    "lux_home_consumption_live": "lux_home_consumption_live",
    "lux_home_consumption": "lux_home_consumption",
    
    # Section2 sensors
    "lux_total_solar_array_1": "lux_total_solar_array_1",
    "lux_total_solar_array_2": "lux_total_solar_array_2",
    "lux_total_solar_array_3": "lux_total_solar_array_3",
    "lux_power_from_inverter_total": "lux_power_from_inverter_total",
    "lux_power_to_inverter_total": "lux_power_to_inverter_total",
    "lux_total_battery_charge": "lux_total_battery_charge",
    "lux_total_battery_discharge": "lux_total_battery_discharge",
    "lux_power_to_eps_total": "lux_power_to_eps_total",
    "lux_power_to_grid_total": "lux_power_to_grid_total",
    "lux_power_from_grid_total": "lux_power_from_grid_total",
    "lux_fault_code": "lux_fault_code",
    "lux_warning_code": "lux_warning_code",
    "lux_internal_temp": "lux_internal_temp",
    "lux_radiator1_temp": "lux_radiator1_temp",
    "lux_radiator2_temp": "lux_radiator2_temp",
    "lux_battery_temperature_live": "lux_battery_temperature_live",
    "lux_uptime": "lux_uptime",
    "lux_total_solar": "lux_total_solar",
    "lux_home_consumption_total": "lux_home_consumption_total",
    
    # Section3 sensors
    "lux_bms_limit_charge": "lux_bms_limit_charge",
    "lux_bms_limit_discharge": "lux_bms_limit_discharge",
    "charge_voltage_ref": "charge_voltage_ref",
    "discharge_cutoff_voltage": "discharge_cutoff_voltage",
    "battery_status_inv": "battery_status_inv",
    "lux_battery_count": "lux_battery_count",
    "lux_battery_capacity_ah": "lux_battery_capacity_ah",
    "lux_battery_current": "lux_battery_current",
    "max_cell_volt": "max_cell_volt",
    "min_cell_volt": "min_cell_volt",
    "max_cell_temp": "max_cell_temp",
    "min_cell_temp": "min_cell_temp",
    "lux_battery_cycle_count": "lux_battery_cycle_count",
    "lux_home_consumption_2_live": "lux_home_consumption_2_live",
    #"lux_home_consumption_2_live_alias": "lux_home_consumption_2_live_alias",
    
    # Section4 sensors
    "lux_current_generator_voltage": "lux_current_generator_voltage",
    "lux_current_generator_frequency": "lux_current_generator_frequency",
    "lux_current_generator_power": "lux_current_generator_power",
    "lux_current_generator_power_daily": "lux_current_generator_power_daily",
    "lux_current_generator_power_all": "lux_current_generator_power_all",
    "lux_current_eps_L1_voltage": "lux_current_eps_L1_voltage",
    "lux_current_eps_L2_voltage": "lux_current_eps_L2_voltage",
    "lux_current_eps_L1_watt": "lux_current_eps_L1_watt",
    "lux_current_eps_L2_watt": "lux_current_eps_L2_watt",
    
    # Section5 sensors
    "p_load_ongrid": "p_load_ongrid",
    "e_load_day": "e_load_day",
    "e_load_all_l": "e_load_all_l",
}

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
    hub = await cg.get_variable(config[CONF_LUXPOWER_SNA_ID])
    
    for yaml_key, c_name in YAML_TO_C_NAMES.items():
        if yaml_key in config:
            conf = config[yaml_key]
            
            # Handle text sensors
            if yaml_key in ["lux_firmware_version", "lux_inverter_model", 
                            "lux_status_text", "lux_battery_status_text","inverter_serial_number"]:
                sens = await text_sensor.new_text_sensor(conf)
            # Handle regular sensors
            else:
                sens = await sensor.new_sensor(conf)
                
            cg.add(getattr(hub, f"set_{c_name}_sensor")(sens))