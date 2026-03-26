import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID, ENTITY_CATEGORY_CONFIG

from . import rbdimmer_ns, CONF_RBDIMMER_ID

DEPENDENCIES = ["rbdimmer"]

CONF_CURVE = "curve"
CONF_LIGHT_ID = "light_id"

RBDimmerCurveSelect = rbdimmer_ns.class_(
    "RBDimmerCurveSelect", select.Select, cg.Component
)
RBDimmerLight = rbdimmer_ns.class_("RBDimmerLight")

CURVE_OPTIONS = ["LINEAR", "RMS", "LOG"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_CURVE): select.select_schema(
            RBDimmerCurveSelect,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon="mdi:chart-bell-curve",
        ).extend(
            {
                cv.Required(CONF_LIGHT_ID): cv.use_id(RBDimmerLight),
            }
        ),
    }
)


async def to_code(config):
    if CONF_CURVE in config:
        curve_conf = config[CONF_CURVE]
        sel = await select.new_select(curve_conf, options=CURVE_OPTIONS)
        await cg.register_component(sel, curve_conf)

        light_var = await cg.get_variable(curve_conf[CONF_LIGHT_ID])
        cg.add(sel.set_light(light_var))
