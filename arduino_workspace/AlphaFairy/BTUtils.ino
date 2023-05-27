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

// TODO: this class can be break down into base class with virtual functions in case more than on camera vendor is supported
class BTCamera {
private:
    BLEAddress* pServerAddress = nullptr;
    bool req_connect = false;
    BLERemoteService* pRemoteService = nullptr;
    BLERemoteCharacteristic* pCmdCharacteristic = nullptr;
    BLERemoteCharacteristic* pStatusCharacteristic = nullptr;
public:
    // TODO: fix this after figure out how to do callback
    uint16_t status = 0;
    bool connected = false;
    BTCamera() {}
    ~BTCamera() {
        disconnect();
        delete pServerAddress;
    }
    void set_addr(BLEAddress addr) {
        pServerAddress = new BLEAddress(addr);
    }
    void set_req_connect() {
        // TODO: mutex or test-and-set instruction required here
        req_connect = true;
    }
    void clear_req_connect() {
        // TODO: mutex or test-and-set instruction required here
        req_connect = false;
    }
    bool is_req_connect() {
        return req_connect;
    }
    bool connect() {
        Serial.print("Forming a connection to ");
        Serial.println(pServerAddress->toString().c_str());
        BLEClient* pClient = BLEDevice::createClient();
        Serial.println(" - Created client");
        if (pClient->connect(*pServerAddress))
            Serial.println(" - Connected to server");

        pRemoteService = pClient->getService(serviceUUID);
        if (pRemoteService == nullptr) {
            Serial.print("Failed to find our service UUID: ");
            Serial.println(serviceUUID.toString().c_str());
            return 1;
        }
        Serial.println(" - Found our service");

        pCmdCharacteristic = pRemoteService->getCharacteristic(commandUUID);
        if (pCmdCharacteristic == nullptr) {
            Serial.print("Failed to find command UUID: ");
            Serial.println(commandUUID.toString().c_str());
            return 1;
        }
        Serial.println(" - Found our command interafce");

        pStatusCharacteristic = pRemoteService->getCharacteristic(statusUUID);
        if (pStatusCharacteristic == nullptr) {
            // Serial.print("Failed to find status UUID: ");
            // Serial.println(statusUUID.toString().c_str());
            return 1;
        }
        // TODO: Not sure how to do it in c++....
        // pStatusCharacteristic->registerForNotify(this->notifyCallback);
        // Serial.println(" - subscribe to status notification");
        connected = true;

        shutter();
        return 0;
    }
    void disconnect() {
        if (connected) {
            // bt_disconnect();
            connected = false;
        }
    }
    void shutter() {
        Serial.println("shutter");
        uint8_t shu[2] = {0x01, 0x06};  // Shutter Half Up
        uint8_t shd[2] = {0x01, 0x07};  // Shutter Half Down
        uint8_t sfu[2] = {0x01, 0x08};  // Shutter Fully Up
        uint8_t sfd[2] = {0x01, 0x09};  // Shutter Fully Down
        pCmdCharacteristic->writeValue(shd, 2, true);
        pCmdCharacteristic->writeValue(sfd, 2, true);
        pCmdCharacteristic->writeValue(sfu, 2, true);
        pCmdCharacteristic->writeValue(shu, 2, true);
    }
    void rec() {
        Serial.println("rec");
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
    // BLEDevice::setSecurityCallbacks(new MySecurity());
    // TODO: set up bt tx power

    // No need to setup PIN/Passkey/YesNo yet
    // BLEDevice::setSecurityCallbacks(new MySecurity());
    // BLESecurity* pSecurity = new BLESecurity();
    // pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_BOND_MITM);
    // pSecurity->setCapability(ESP_IO_CAP_NONE);
    // pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

    bt_scan();
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        constexpr std::array<uint8_t, 4> CAMERA_MANUFACTURER_LOOKUP = {
            0x2D, 0x01, 0x03, 0x00};
        auto mfd = advertisedDevice.getManufacturerData();
        bool is_sony_cam =
            std::equal(CAMERA_MANUFACTURER_LOOKUP.begin(),
                       CAMERA_MANUFACTURER_LOOKUP.end(), mfd.begin());
        // TODO: can also add check for it is ready for pairing and display on screen
        if (is_sony_cam) {
            Serial.print("Found a Sony Camera");
            Serial.println(advertisedDevice.getName().c_str());
            BLEDevice::getScan()->stop();
            bt_camera.set_addr(advertisedDevice.getAddress());
            bt_camera.set_req_connect();
        }
    }
};

void bt_scan() 
{
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(30);
    // TODO: is this blocking? if not, when bt_scan is called multiple times will it cause memory leak?
}

void bt_poll()
{
    // TODO: for camera in bt_cameras:
    // in case we support multiple cameras
    if (bt_camera.is_req_connect()) {
        Serial.println("Requested Connection.");
        if (!bt_camera.connect()) {
            // Serial.println("We are now connected to the Camera.");
            bt_camera.clear_req_connect();
        } else {
            Serial.println("We have failed to connect to the camera");
        };
    }

    if (bt_camera.connected) {
        if (bt_req_shutter) {
            // TODO: mutex here? or any test-add-set api available?
            bt_req_shutter = false;
            bt_camera.shutter();
        }

        if (bt_req_rec) {
            bt_req_rec = false;
            bt_camera.rec();
            // Serial.print("recording started ");
        }
    }
}
