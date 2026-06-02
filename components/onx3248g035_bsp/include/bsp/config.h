#pragma once

/*
 * Temporary PrintSphere-compatible BSP profile for ONX3248G035.
 *
 * Build/Release must select exactly one BSP component that exports bsp/*.
 * Do not compile this ONX compatibility profile together with the Waveshare
 * BSP component, because both provide the same public header and symbol names.
 */

#define BSP_CONFIG_NO_GRAPHIC_LIB 0
