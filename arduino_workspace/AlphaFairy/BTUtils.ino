#include "AlphaFairy.h"
#include "BLEDevice.h"

// set how many cameras we can connect to at the same time
// #define MAX_N_CAMS 4

static BLEUUID serviceUUID("8000ff00-ff00-ffff-ffff-ffffffffffff");
static BLEUUID commandUUID("0000ff01-0000-1000-8000-00805f9b34fb");
static BLEUUID statusUUID("0000ff02-0000-1000-8000-00805f9b34fb");
static bool doConnect = false;
static bool connected = false;

static bool bt_req_shutter;
static bool bt_req_rec;
// TODO: add more bt_req_* for zoom/focus, etc

// TODO: this class can be break down into base class with virtual functions in
// case more than on camera vendor is supported
class BTCamera
{
private:
    bool req_connect = false;
    BLEAddress *pServerAddress = nullptr;
    BLERemoteService *pRemoteService = nullptr;
    BLERemoteCharacteristic *pCmdCharacteristic = nullptr;
    BLERemoteCharacteristic *pStatusCharacteristic = nullptr;

public:
    // TODO: fix this after figure out how to do callback
    uint16_t status = 0;
    bool connected = false;
    BTCamera() {}
    ~BTCamera()
    {
        disconnect();
        if (pServerAddress) {delete pServerAddress;}
        if (pRemoteService) {delete pRemoteService;}
        if (pCmdCharacteristic) {delete pCmdCharacteristic;}
        if (pStatusCharacteristic) {delete pStatusCharacteristic;}
    }
    void set_addr(BLEAddress addr) { pServerAddress = new BLEAddress(addr); }
    void set_req_connect()
    {
        // TODO: mutex or test-and-set instruction required here
        req_connect = true;
    }
    void clear_req_connect()
    {
        // TODO: mutex or test-and-set instruction required here
        req_connect = false;
    }
    bool is_req_connect() { return req_connect; }
    bool connect()
    {
        log_i("Forming a connection to %s", pServerAddress->toString().c_str());
        BLEClient *pClient = BLEDevice::createClient();
        if (pClient->connect(*pServerAddress))
            log_i(" - Connected");

        pRemoteService = pClient->getService(serviceUUID);
        if (pRemoteService == nullptr)
        {
            log_i("Failed to find our service UUID: %s", serviceUUID.toString().c_str());
            return 1;
        }
        log_i(" - Found our service");

        pCmdCharacteristic = pRemoteService->getCharacteristic(commandUUID);
        if (pCmdCharacteristic == nullptr)
        {
            log_i("Failed to find command UUID: %s", commandUUID.toString().c_str());
            return 1;
        }
        log_i(" - Found our command interafce");

        pStatusCharacteristic = pRemoteService->getCharacteristic(statusUUID);
        if (pStatusCharacteristic == nullptr)
        {
            log_i("Failed to find status UUID: %s", statusUUID.toString().c_str());
            return 1;
        }
        // TODO: Not sure how to do it in c++....
        // pStatusCharacteristic->registerForNotify(this->notifyCallback);
        // log_i(" - subscribe to status notification");
        connected = true;

        return 0;
    }
    void disconnect()
    {
        if (connected)
        {
            // bt_disconnect();
            connected = false;
        }
    }
    void shutter()
    {
        log_i("shutter");
        uint8_t shu[2] = {0x01, 0x06}; // Shutter Half Up
        uint8_t shd[2] = {0x01, 0x07}; // Shutter Half Down
        uint8_t sfu[2] = {0x01, 0x08}; // Shutter Fully Up
        uint8_t sfd[2] = {0x01, 0x09}; // Shutter Fully Down
        pCmdCharacteristic->writeValue(shd, 2, true);
        pCmdCharacteristic->writeValue(sfd, 2, true);
        pCmdCharacteristic->writeValue(sfu, 2, true);
        pCmdCharacteristic->writeValue(shu, 2, true);
    }
    void rec()
    {
        log_i("rec");
        uint8_t rec[2] = {0x01, 0x0f};
        uint8_t ret[2] = {0x01, 0x0e};
        pCmdCharacteristic->writeValue(rec, 2, true);
        pCmdCharacteristic->writeValue(ret, 2, true);
    }
};

// static BTCamera bt_cameras[MAX_N_CAMS];
static BTCamera bt_camera;

void bt_init()
{
    BLEDevice::init("Alpha Fairy");
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);

    bt_scan();
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        // sony id bytes
        constexpr std::array<uint8_t, 4> CAMERA_MANUFACTURER_LOOKUP = {0x2D, 0x01,
                                                                       0x03, 0x00};
        auto mfd = advertisedDevice.getManufacturerData();
        bool is_sony_cam =
            std::equal(CAMERA_MANUFACTURER_LOOKUP.begin(),
                       CAMERA_MANUFACTURER_LOOKUP.end(), mfd.begin());

        if (is_sony_cam)
        {
            log_i("Found a Sony Camera");
            log_i("%s", advertisedDevice.getName().c_str());
            BLEDevice::getScan()->stop();
            bt_camera.set_addr(advertisedDevice.getAddress());
            bt_camera.set_req_connect();
        }
    }
};

void bt_scan()
{
    BLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(2, true);
}

void bt_poll()
{
    if (bt_camera.is_req_connect())
    {
        log_i("Requested Connection.");
        if (!bt_camera.connect())
        {
            log_i("We are now connected to the Camera.");
            bt_camera.clear_req_connect();
        }
        else
        {
            log_i("We have failed to connect to the camera");
        };
    }

    if (bt_camera.connected)
    {
        if (bt_req_shutter)
        {
            // TODO: mutex here? or any test-add-set api available?
            bt_req_shutter = false;
            bt_camera.shutter();
        }

        if (bt_req_rec)
        {
            bt_req_rec = false;
            bt_camera.rec();
            // Serial.print("recording started ");
        }
    }
}