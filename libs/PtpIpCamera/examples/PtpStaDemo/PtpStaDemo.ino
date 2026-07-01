#include <PtpIpCamera.h>
#include <PtpIpSonyAlphaCamera.h>

PtpIpSonyAlphaCamera ptpcam((char*)"ALPHA-FAIRY", NULL); // declare with name and random GUID
int wifi_last_status;

void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    wifi_last_status = WiFi.status(); // remember pre-init status
    WiFi.begin("DIRECT-qIR1:DSC-RX0M2", "7CBcqf62"); // connect to camera
    Serial.println("Hello World. PTP Demo. WiFi attempting connection...");
}

void loop()
{
    ptpcam.task(); // performs all required polling and tasks for the PTP connection

    int status = WiFi.status();
    if (status == WL_CONNECTED)
    {
        IPAddress gateway = WiFi.gatewayIP();
        IPAddress localIp = WiFi.localIP();
        if (gateway != 0 && localIp != 0 && wifi_last_status != status && ptpcam.canNewConnect()) // new connect
        {
            wifi_last_status = status;
            Serial.print("WiFi Connected: ");
            Serial.println(gateway);
            ptpcam.begin((uint32_t)gateway); // start the handshake
        }
    }
    else
    {
        wifi_last_status = status;
    }

    if (Serial.available()) // check if user has input command from serial port
    {
        char c = Serial.read();
        if (c == 'x') // this is a command from serial port
        {
            Serial.println("Command: Shoot Photo");
            if (ptpcam.isOperating()) // check connection
            {
                ptpcam.cmd_Shoot(200); // execute the command
            }
            else
            {
                Serial.println("Error: Camera Not Connected");
            }
        }
        else if (c == '[' || c == ']') // command to change aperture
        {
            change_aperture(c == '[' ? -1 : +1);
        }
        else if (c != '\r' && c != '\n')
        {
            Serial.printf("Unknown Command: %c\r\n", c);
        }
    }
}

void change_aperture(int dir)
{
    if (ptpcam.table_aperture != NULL) // validate table exists
    {
        uint32_t tbl_len = ptpcam.table_aperture[0];
        if (tbl_len > 0) // validate table has entries
        {
            uint16_t* aperture_table = (uint16_t*)&(ptpcam.table_aperture[1]); // table is declared as 32 bit but actually uses 16 bit entries starting after the first 32 bit entry
            if (ptpcam.has_property(SONYALPHA_PROPCODE_Aperture)) // validate camera has aperture data
            {
                // get the current aperture
                uint16_t current_aperture = ptpcam.get_property(SONYALPHA_PROPCODE_Aperture);
                float current_aperture_f = current_aperture;
                Serial.printf("Current aperture: 0x%04X = %0.1f\r\n", current_aperture, current_aperture_f / 100.0f);
                int cur_idx = -1, i;
                // look in the table for the matching entry
                for (i = 0; i < tbl_len; i++)
                {
                    uint16_t x = aperture_table[i];
                    if (x == current_aperture) {
                        // found match
                        cur_idx = i;
                        break;
                    }
                }

                if (cur_idx >= 0)
                {
                    // match found
                    // find the next idx in the table
                    int next_idx = cur_idx + dir;
                    // limit range to somewhere within the table
                    if (next_idx < 0) {
                        next_idx = 0;
                    }
                    else if (next_idx >= tbl_len) {
                        next_idx = tbl_len - 1;
                    }

                    // find the next entry in the table
                    uint16_t next_aperture = aperture_table[next_idx];
                    float next_aperture_f = next_aperture;
                    Serial.printf("Next aperture[%u -> %u]: 0x%04X = %0.1f\r\n", cur_idx, next_idx, next_aperture, next_aperture_f / 100.0f);

                    // finally send the command, knowing that the parameter is valid
                    ptpcam.cmd_ApertureSet(next_aperture);
                }
                else
                {
                    Serial.println("Error: No aperture matches in table");
                }
            }
            else
            {
                Serial.println("Error: No aperture data available");
            }
        }
        else
        {
            Serial.println("Error: Aperture table has 0 entries");
        }
    }
    else
    {
        Serial.println("Error: No aperture table available");
    }
}
