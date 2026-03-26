import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome import pins
from esphome.const import (
    CONF_OUTPUT_ID,
    CONF_GAMMA_CORRECT,
    CONF_DEFAULT_TRANSITION_LENGTH,
)

from . import rbdimmer_ns, RBDimmerHub, CONF_RBDIMMER_ID, CONF_PHASE

DEPENDENCIES = ["rbdimmer"]

RBDimmerLight = rbdimmer_ns.class_("RBDimmerLight", light.LightOutput, cg.Component)

CONF_PIN = "pin"
CONF_CURVE = "curve"

CURVE_OPTIONS = {
    "linear": 0,
    "rms": 1,
    "logarithmic": 2,
}

CONFIG_SCHEMA = (
    light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(RBDimmerLight),
            cv.GenerateID(CONF_RBDIMMER_ID): cv.use_id(RBDimmerHub),
            cv.Required(CONF_PIN): pins.internal_gpio_output_pin_number,
            cv.Optional(CONF_PHASE, default=0): cv.int_range(min=0, max=3),
            cv.Optional(CONF_CURVE, default="rms"): cv.enum(CURVE_OPTIONS, lower=True),
            cv.Optional(CONF_GAMMA_CORRECT, default=1.0): cv.positive_float,
            cv.Optional(
                CONF_DEFAULT_TRANSITION_LENGTH, default="1s"
            ): cv.positive_time_period_milliseconds,
        }
    ).extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)

    hub = await cg.get_variable(config[CONF_RBDIMMER_ID])
    cg.add(var.set_hub(hub))
    cg.add(var.set_pin(config[CONF_PIN]))
    cg.add(var.set_phase(config[CONF_PHASE]))
    cg.add(var.set_curve(config[CONF_CURVE]))
