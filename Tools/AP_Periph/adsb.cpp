/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
  AP_Periph ADSB support. This is designed to talk to a Ping ADSB
  module over the UART
 */

#include <AP_HAL/AP_HAL_Boards.h>
#include "AP_Periph.h"

#ifdef HAL_PERIPH_ENABLE_ADSB

#include <AP_SerialManager/AP_SerialManager.h>

extern const AP_HAL::HAL &hal;

# if !HAL_GCS_ENABLED

#include "include/mavlink/v2.0/protocol.h"
#include "include/mavlink/v2.0/mavlink_types.h"
#include "include/mavlink/v2.0/all/mavlink.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#include "include/mavlink/v2.0/mavlink_helpers.h"
#pragma GCC diagnostic pop

#endif

/*
  init ADSB support
 */
void AP_Periph_FW::adsb_init(void)
{
    if (g.adsb_baudrate > 0) {
        auto *uart = hal.serial(g.adsb_port);
        if (uart == nullptr) {
            return;
        }
        uart->begin(AP_SerialManager::map_baudrate(g.adsb_baudrate), 256, 256);
    }
}

static mavlink_message_t chan_buffer;
mavlink_message_t* mavlink_get_channel_buffer(uint8_t chan) {
    return &chan_buffer;
}

static mavlink_status_t chan_status;
mavlink_status_t* mavlink_get_channel_status(uint8_t chan) {
    return &chan_status;
}

/*
  update ADSB subsystem
 */
void AP_Periph_FW::adsb_update(void)
{
    if (g.adsb_baudrate <= 0) {
        return;
    }

    auto *uart = hal.serial(g.adsb_port);
    if (uart == nullptr) {
        return;
    }

    // look for incoming MAVLink ADSB_VEHICLE packets
    const uint16_t nbytes = uart->available();
    for (uint16_t i=0; i<nbytes; i++) {
        const uint8_t c = (uint8_t)uart->read();

        // Try to get a new message
        if (mavlink_parse_char(MAVLINK_COMM_0, c, &adsb.msg, &adsb.status)) {
            if (adsb.msg.msgid == MAVLINK_MSG_ID_ADSB_VEHICLE) {
                // decode and send as UAVCAN TrafficReport
                static mavlink_adsb_vehicle_t msg;
                mavlink_msg_adsb_vehicle_decode(&adsb.msg, &msg);
                can_send_ADSB(msg);
            }
        }
    }
}

#endif // HAL_PERIPH_ENABLE_ADSB
