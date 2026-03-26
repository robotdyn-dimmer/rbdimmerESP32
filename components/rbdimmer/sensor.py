import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    UNIT_HERTZ,
    UNIT_PERCENT,
    DEVICE_CLASS_FREQUENCY,
    STATE_CLASS_MEASUREMENT,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from . import rbdimmer_ns, RBDimmerHub, CONF_RBDIMMER_ID, CONF_PHASE

DEPENDENCIES = ["rbdimmer"]

CONF_AC_FREQUENCY = "ac_frequency"
CONF_LEVEL = "level"
CONF_FIRING_DELAY = "firing_delay"
CONF_LIGHT_ID = "light_id"

ICON_SINE_WAVE = "mdi:sine-wave"
ICON_BRIGHTNESS = "mdi:brightness-percent"
ICON_TIMER = "mdi:timer-outline"

RBDimmerSensor = rbdimmer_ns.class_("RBDimmerSensor", cg.PollingComponent)
RBDimmerLight = rbdimmer_ns.class_("RBDimmerLight")

AC_FREQUENCY_SCHEMA = sensor.sensor_schema(
    unit_of_measurement=UNIT_HERTZ,
    accuracy_decimals=0,
    device_class=DEVICE_CLASS_FREQUENCY,
    state_class=STATE_CLASS_MEASUREMENT,
    icon=ICON_SINE_WAVE,
).extend(
    {
        cv.Optional(CONF_PHASE, default=0): cv.int_range(min=0, max=3),
    }
)

LEVEL_SCHEMA = sensor.sensor_schema(
    unit_of_measurement=UNIT_PERCENT,
    accuracy_decimals=0,
    state_class=STATE_CLASS_MEASUREMENT,
    icon=ICON_BRIGHTNESS,
).extend(
    {
        cv.Required(CONF_LIGHT_ID): cv.use_id(RBDimmerLight),
    }
)

FIRING_DELAY_SCHEMA = sensor.sensor_schema(
    unit_of_measurement="us",
    accuracy_decimals=0,
    icon=ICON_TIMER,
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
).extend(
    {
        cv.Required(CONF_LIGHT_ID): cv.use_id(RBDimmerLight),
    }
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(RBDimmerSensor),
            cv.GenerateID(CONF_RBDIMMER_ID): cv.use_id(RBDimmerHub),
            cv.Optional(CONF_AC_FREQUENCY): AC_FREQUENCY_SCHEMA,
            cv.Optional(CONF_LEVEL): LEVEL_SCHEMA,
            cv.Optional(CONF_FIRING_DELAY): FIRING_DELAY_SCHEMA,
        }
    ).extend(cv.polling_component_schema("60s"))
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_RBDIMMER_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_hub(hub))

    if CONF_AC_FREQUENCY in config:
        freq_conf = config[CONF_AC_FREQUENCY]
        sens = await sensor.new_sensor(freq_conf)
        cg.add(var.set_ac_frequency_sensor(sens))
        cg.add(var.set_ac_frequency_phase(freq_conf.get(CONF_PHASE, 0)))

    if CONF_LEVEL in config:
        level_conf = config[CONF_LEVEL]
        sens = await sensor.new_sensor(level_conf)
        cg.add(var.set_level_sensor(sens))
        light_var = await cg.get_variable(level_conf[CONF_LIGHT_ID])
        cg.add(var.set_level_light(light_var))

    if CONF_FIRING_DELAY in config:
        delay_conf = config[CONF_FIRING_DELAY]
        sens = await sensor.new_sensor(delay_conf)
        cg.add(var.set_firing_delay_sensor(sens))
        light_var = await cg.get_variable(delay_conf[CONF_LIGHT_ID])
        cg.add(var.set_firing_delay_light(light_var))
