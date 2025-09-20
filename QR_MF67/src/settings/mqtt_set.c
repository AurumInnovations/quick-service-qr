#include "../app_def.h"
#include "admin_set_func.h"
#include "mqtt_set.h"

static void _mqttSet_Paint(int value)
{
    gui_begin_batch_paint();
    gui_clear_dc();
    gui_text_out(0, GUI_LINE_TOP(0), value == 1 ? "MQTT:On" : "MQTT:Off");
    gui_page_op_paint("Cancel", "OK");
    gui_end_batch_paint();
}

void MqttSet_Show()
{
    st_gui_message pMsg;
    int nOpen = get_setting_int(MQTT_HOST_OPEN);
    _mqttSet_Paint(nOpen);

    while(1) {
        if (gui_get_message(&pMsg, 100) == 0) {
            if (pMsg.message_id == GUI_KEYPRESS) {
                switch(pMsg.wparam) {
                    case GUI_KEY_UP:
                    case GUI_KEY_DOWN:
                        nOpen = 1 - nOpen;
                        _mqttSet_Paint(nOpen);
                        break;
                    case GUI_KEY_OK:
                        set_setting_int(MQTT_HOST_OPEN, nOpen);
                    case GUI_KEY_QUIT:
                        return;
                }
            }
        }
    }
}
