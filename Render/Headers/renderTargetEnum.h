#pragma once

enum RENDER_TARGET
{
    RT_FP16,
    RT_SHADOW_MAP,
    RT_DEPTH_BUFFER,
    RT_BACK_BUFFER,
    RT_LAST
};

static const char* RENDER_TARGET_NAME[] = {
    "FP16",
    "SHADOW_MAP",
    "DEPTH_BUFFER",
    "BACK_BUFFER",
    "LAST"
};
