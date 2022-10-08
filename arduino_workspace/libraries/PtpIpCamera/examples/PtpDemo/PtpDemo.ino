#include <PtpIpCamera.h>
#include <PtpIpSonyAlphaCamera.h>

PtpIpSonyAlphaCamera ptpcam((char*)"ALPHA-FAIRY", NULL); // declare with name and random GUID

void setup()
{
    Serial.begin(115200);
    WiFi.softAP("fairywifi", "1234567890"); // start WiFi AP
    Serial.println("Hello World. PTP Demo. WiFi AP initialized, waiting for client...");
}

uint32_t check_for_wifi_client()
{
    // get a list of WiFi clients, and see which one has been assigned a valid IP
    wifi_sta_list_t          wifi_sta_list;
    tcpip_adapter_sta_list_t adapter_sta_list;
    memset(&wifi_sta_list   , 0, sizeof(wifi_sta_list   ));
    memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));
    esp_wifi_ap_get_sta_list  (&wifi_sta_list);
    tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);
    int i;
    for (i = 0; i < adapter_sta_list.num; i++) {
        if (adapter_sta_list.sta[i].ip.addr != 0) {
            return adapter_sta_list.sta[i].ip.addr;
        }
    }
    return 0;
}

void loop()
{
    static uint32_t client_ip = 0;
    uint32_t check_ip;

    ptpcam.task(); // performs all required polling and tasks for the PTP connection

    if (client_ip == 0) // has not connected yet
    {
        check_ip = check_for_wifi_client();
        if (check_ip != 0) // new WiFi client
        {
            client_ip = check_ip;
            Serial.print("New WiFi Client: ");
            Serial.println(IPAddress(client_ip));
            ptpcam.begin(client_ip);
        }
    }

    if (Serial.available()) // check if user has input command from serial port
    {
        char c = Serial.read();
        if (c == 'x') // this is a command from serial port
        {
            Serial.println("Command: Shoot Photo");
            ptpcam.cmd_Shoot(200); // execute the command
        }
    }
}
