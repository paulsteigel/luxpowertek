import esphome.codegen as cg
import esphome.config_validation as cv

luxpowertek_ns = cg.esphome_ns.namespace("luxpowertek")
LuxPowertekComponent = luxpowertek_ns.class_("LuxPowertekComponent", cg.Component)

CONF_LUXPOWERTEK_ID = "luxpowertek_id"

LUXPOWERTEK_COMPONENT_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_LUXPOWERTEK_ID): cv.use_id(LuxPowertekComponent),
})
