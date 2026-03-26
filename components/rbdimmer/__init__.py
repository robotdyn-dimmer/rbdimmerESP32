import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components.esp32 import add_idf_component
from esphome.const import CONF_ID, CONF_FREQUENCY

CODEOWNERS = ["@dev-rbdimmer"]
DEPENDENCIES = ["esp32"]
MULTI_CONF = True
AUTO_LOAD = ["sensor", "select"]

CONF_RBDIMMER_ID = "rbdimmer_id"
CONF_PHASES = "phases"
CONF_PHASE = "phase"
CONF_ZERO_CROSS_PIN = "zero_cross_pin"

rbdimmer_ns = cg.esphome_ns.namespace("rbdimmer")
RBDimmerHub = rbdimmer_ns.class_("RBDimmerHub", cg.Component)

PHASE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_PHASE): cv.int_range(min=0, max=3),
        cv.Required(CONF_ZERO_CROSS_PIN): pins.internal_gpio_input_pin_number,
        cv.Optional(CONF_FREQUENCY, default=0): cv.int_range(min=0, max=65),
    }
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(RBDimmerHub),
            cv.Optional(CONF_PHASES): cv.ensure_list(PHASE_SCHEMA),
            cv.Optional(CONF_ZERO_CROSS_PIN): pins.internal_gpio_input_pin_number,
            cv.Optional(CONF_FREQUENCY, default=0): cv.int_range(min=0, max=65),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.has_at_least_one_key(CONF_PHASES, CONF_ZERO_CROSS_PIN),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_PHASES in config:
        for phase_conf in config[CONF_PHASES]:
            cg.add(
                var.add_phase(
                    phase_conf[CONF_PHASE],
                    phase_conf[CONF_ZERO_CROSS_PIN],
                    phase_conf.get(CONF_FREQUENCY, 0),
                )
            )
    else:
        cg.add(
            var.add_phase(
                0,
                config[CONF_ZERO_CROSS_PIN],
                config.get(CONF_FREQUENCY, 0),
            )
        )

    add_idf_component(
        name="rbdimmerESP32",
        repo="https://github.com/robotdyn-dimmer/rbdimmerESP32.git",
        ref="v2.0.0",
    )
