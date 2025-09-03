#include <assert.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/usb/usb_device.h>
#include <soc.h>

#include "target.h"
#include "bootutil/bootutil_log.h"

BOOT_LOG_MODULE_DECLARE(mcuboot);

#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <mgmt/mcumgr/transport/smp_internal.h>

#include "alif_ble.h"
#include "gap_le.h"
#include "gapc_le.h"
#include "gapc_sec.h"
#include "gapm.h"
#include "gapm_le.h"
#include "gapm_le_adv.h"
#include "co_endian.h"
#include "gatt_db.h"
#include "se_service.h"

#include "filesystem.h"
#include "le_main.h"

#include "io/io.h"

void boot_ble_start(void)
{
    __ASSERT(filesystem_init() == 0, "filesystem_init failed");
    __ASSERT(le_init() == 0, "le_init failed");
    le_adv_start();

    bool _reset = false, _reset_wait = false;

#ifdef CONFIG_BOOT_BLE_DFU_ENTRANCE_GPIO
    le_update_activity();
    uint32_t interval = 0;
    uint32_t last_printed_second = 0;
    uint32_t current_second = 0;

    while (1)
    {
        k_sleep(K_MSEC(50));
        if (le_is_connected())
        {
            le_update_activity();
        }
        
        interval = k_uptime_get_32() - le_get_last_activity();
        if (interval > CONFIG_BOOT_BLE_DFU_IDLE_TIMEOUT * 1000)
        {
            BOOT_LOG_INF("No activity detected for %u seconds, exiting DFU mode", CONFIG_BOOT_BLE_DFU_IDLE_TIMEOUT);
            io_led_set(0);
            sys_reboot(SYS_REBOOT_COLD);
        }
        
        current_second = interval / 1000;
        if (current_second != last_printed_second) {
            printf("interval: %d s\r", current_second);
            last_printed_second = current_second;
        }

        if (read_button() == 0)
        {
            if (_reset)
            {
                BOOT_LOG_INF("Resetting device");
                sys_reboot(SYS_REBOOT_COLD);
            }
            if (!_reset_wait)
            {
                BOOT_LOG_INF("Waiting for button to be pressed");
                _reset_wait = true;
            }
        }
        if (read_button() == 1)
        {
            if (_reset_wait)
            {
                BOOT_LOG_INF("Waiting for button to be released");
                _reset = true;
            }
        }
    }
#else
    while (1)
    {
        k_sleep(K_MSEC(1000));
    }
#endif
}
