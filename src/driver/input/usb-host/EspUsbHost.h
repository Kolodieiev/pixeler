#ifndef ESP_USB_HOST_H
#define ESP_USB_HOST_H

#include <sdkconfig.h>

#ifndef CONFIG_IDF_TARGET_ESP32

#include <Arduino.h>
#include <class/hid/hid.h>
#include <usb/usb_host.h>

#include <functional>

#ifndef ESP_USB_HOST_ENABLE_MSC
#define ESP_USB_HOST_ENABLE_MSC 0
#endif

#if ESP_USB_HOST_ENABLE_MSC
#include <FS.h>
#endif

#if __has_include(<rom/usb/usb_common.h>)
#include <rom/usb/usb_common.h>
#else
#define USB_DEVICE_DESC 0x01
#define USB_CONFIGURATION_DESC 0x02
#define USB_STRING_DESC 0x03
#define USB_INTERFACE_DESC 0x04
#define USB_ENDPOINT_DESC 0x05
#define USB_INTERFACE_ASSOC_DESC 0x0B
#define USB_HID_DESC 0x21
#define USB_HID_REPORT_DESC 0x22
#endif

enum EspUsbHostKeyboardLayout : uint16_t
{
  ESP_USB_HOST_KEYBOARD_LAYOUT_EN_US = 0x0409,
  ESP_USB_HOST_KEYBOARD_LAYOUT_UK_UA = 0x0422,
};

enum EspUsbHostPort : uint8_t
{
  ESP_USB_HOST_PORT_DEFAULT = 0,
  ESP_USB_HOST_PORT_HIGH_SPEED,
  ESP_USB_HOST_PORT_FULL_SPEED,
};

static constexpr uint8_t ESP_USB_HOST_MOUSE_LEFT = 0x01;
static constexpr uint8_t ESP_USB_HOST_MOUSE_RIGHT = 0x02;
static constexpr uint8_t ESP_USB_HOST_MOUSE_MIDDLE = 0x04;
static constexpr uint8_t ESP_USB_HOST_MOUSE_BACK = 0x08;
static constexpr uint8_t ESP_USB_HOST_MOUSE_FORWARD = 0x10;

static constexpr uint8_t ESP_USB_HOST_HID_REPORT_TYPE_INPUT = 0x01;
static constexpr uint8_t ESP_USB_HOST_HID_REPORT_TYPE_OUTPUT = 0x02;
static constexpr uint8_t ESP_USB_HOST_HID_REPORT_TYPE_FEATURE = 0x03;

static constexpr uint8_t ESP_USB_HOST_KEYBOARD_LED_NUM_LOCK = 0x01;
static constexpr uint8_t ESP_USB_HOST_KEYBOARD_LED_CAPS_LOCK = 0x02;
static constexpr uint8_t ESP_USB_HOST_KEYBOARD_LED_SCROLL_LOCK = 0x04;

static constexpr uint8_t ESP_USB_HOST_HID_REPORT_ID_KEYBOARD = 0x01;
static constexpr uint8_t ESP_USB_HOST_HID_REPORT_ID_MOUSE = 0x02;

static constexpr uint8_t ESP_USB_HOST_ANY_ADDRESS = 0xff;
static constexpr size_t ESP_USB_HOST_MAX_DEVICES = 4;
static constexpr size_t ESP_USB_HOST_MAX_INTERFACES = 4;
static constexpr size_t ESP_USB_HOST_MAX_ENDPOINTS = 6;
static constexpr uint32_t ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS = 5000;

struct EspUsbHostConfig
{
  uint32_t taskStackSize = 4096;
  UBaseType_t taskPriority = 5;
  BaseType_t taskCore = tskNO_AFFINITY;
  EspUsbHostPort port = ESP_USB_HOST_PORT_DEFAULT;
};

struct EspUsbHostDeviceInfo
{
  const char* manufacturer = "";
  const char* product = "";
  const char* serial = "";
  usb_speed_t speed = USB_SPEED_FULL;
  uint16_t vid = 0;
  uint16_t pid = 0;
  uint16_t usbVersion = 0;
  uint16_t deviceVersion = 0;
  uint16_t configurationTotalLength = 0;
  uint8_t address = 0;
  uint8_t parentAddress = 0;
  uint8_t portId = 0;
  uint8_t deviceClass = 0;
  uint8_t deviceSubClass = 0;
  uint8_t deviceProtocol = 0;
  uint8_t maxPacketSize0 = 0;
  uint8_t configurationValue = 0;
  uint8_t configurationAttributes = 0;
  uint8_t configurationMaxPower = 0;
  uint8_t configurationInterfaceCount = 0;
  bool supported = false;
  bool isHub = false;
};

struct EspUsbHostHIDReportData
{
  const char* manufacturer = "";
  const char* product = "";
  const char* serial = "";
  const uint8_t* rawData = nullptr;
  const uint8_t* reportData = nullptr;
  size_t rawLength = 0;
  size_t reportLength = 0;
  uint16_t vid = 0;
  uint16_t pid = 0;
};

struct EspUsbHostKeyboardEvent : EspUsbHostHIDReportData
{
  uint16_t unicode = 0;
  char utf8[4] = {};
  uint8_t address = 0;
  uint8_t interfaceNumber = 0;
  uint8_t keycode = 0;
  uint8_t ascii = 0;
  uint8_t modifiers = 0;
  bool pressed = false;
  bool released = false;
  bool numLock = false;
  bool capsLock = false;
  bool scrollLock = false;
};

struct EspUsbHostMouseEvent : EspUsbHostHIDReportData
{
  int16_t x = 0;
  int16_t y = 0;
  int16_t wheel = 0;
  uint8_t address = 0;
  uint8_t interfaceNumber = 0;
  uint8_t buttons = 0;
  uint8_t previousButtons = 0;
  bool moved = false;
  bool buttonsChanged = false;
};

#if ESP_USB_HOST_ENABLE_MSC
struct EspUsbHostMscInquiry
{
  char product[17] = {};
  char vendor[9] = {};
  char revision[5] = {};
  uint8_t peripheralDeviceType = 0;
  bool removable = false;
};

struct EspUsbHostMscSense
{
  uint8_t responseCode = 0;
  uint8_t senseKey = 0;
  uint8_t additionalSenseCode = 0;
  uint8_t additionalSenseQualifier = 0;
};

struct EspUsbHostMscBlockDeviceInfo
{
  uint64_t blockCount = 0;
  uint64_t capacityBytes = 0;
  uint32_t blockSize = 0;
  uint8_t address = 0;
  uint8_t interfaceNumber = 0;
  uint8_t lun = 0;
  uint8_t maxLun = 0;
};
#endif

void espUsbHostPrint(const EspUsbHostDeviceInfo& device, Print& out = Serial);

class EspUsbHost
{
public:
  using DeviceCallback = std::function<void(const EspUsbHostDeviceInfo&)>;
  using KeyboardCallback = std::function<void(const EspUsbHostKeyboardEvent&, void*)>;
  using MouseCallback = std::function<void(const EspUsbHostMouseEvent&, void*)>;

  EspUsbHost();
  ~EspUsbHost();

  bool begin();
  bool begin(const EspUsbHostConfig& config);
  void end();
  bool ready() const;

  void onDeviceConnected(DeviceCallback callback);
  void onDeviceDisconnected(DeviceCallback callback);
  void onKeyboard(KeyboardCallback callback, void* arg = nullptr);
  void onMouse(MouseCallback callback, void* arg = nullptr);

  void setKeyboardLayout(EspUsbHostKeyboardLayout layout);
  EspUsbHostKeyboardLayout getKeyboardLayout() const;

  bool sendSetProtocol(uint8_t interfaceNumber, uint8_t address);
  bool sendHIDReport(uint8_t interfaceNumber,
                     uint8_t reportType,
                     uint8_t reportId,
                     const uint8_t* data,
                     size_t length,
                     uint8_t address = ESP_USB_HOST_ANY_ADDRESS);
  bool setKeyboardLeds(bool numLock,
                       bool capsLock,
                       bool scrollLock,
                       uint8_t address = ESP_USB_HOST_ANY_ADDRESS);
  bool getKeyboardNumLock(uint8_t address = ESP_USB_HOST_ANY_ADDRESS) const;
  bool getKeyboardCapsLock(uint8_t address = ESP_USB_HOST_ANY_ADDRESS) const;
  bool getKeyboardScrollLock(uint8_t address = ESP_USB_HOST_ANY_ADDRESS) const;

#if ESP_USB_HOST_ENABLE_MSC
  bool mscReady(uint8_t address = ESP_USB_HOST_ANY_ADDRESS) const;
  bool mscInquiry(EspUsbHostMscInquiry& inquiry,
                  uint8_t address = ESP_USB_HOST_ANY_ADDRESS,
                  uint32_t timeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS);
  bool mscRequestSense(EspUsbHostMscSense& sense,
                       uint8_t address = ESP_USB_HOST_ANY_ADDRESS,
                       uint32_t timeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS);
  bool mscLastSense(EspUsbHostMscSense& sense,
                    uint8_t address = ESP_USB_HOST_ANY_ADDRESS) const;
  bool mscMaxLun(uint8_t& maxLun,
                 uint8_t address = ESP_USB_HOST_ANY_ADDRESS,
                 uint32_t timeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS);
  bool mscSelectLun(uint8_t lun,
                    uint8_t address = ESP_USB_HOST_ANY_ADDRESS,
                    uint32_t timeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS);
  bool mscGetBlockDeviceInfo(EspUsbHostMscBlockDeviceInfo& info,
                             uint8_t address = ESP_USB_HOST_ANY_ADDRESS,
                             uint32_t timeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS);
  bool mscTestUnitReady(uint8_t address = ESP_USB_HOST_ANY_ADDRESS,
                        uint32_t timeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS);
  bool mscWaitReady(uint8_t address = ESP_USB_HOST_ANY_ADDRESS,
                    uint32_t readyTimeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS,
                    uint32_t commandTimeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS);
  bool mscCapacity64(uint64_t& blockCount,
                     uint32_t& blockSize,
                     uint8_t address = ESP_USB_HOST_ANY_ADDRESS,
                     uint32_t timeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS);
  bool mscCapacity(uint32_t& blockCount,
                   uint32_t& blockSize,
                   uint8_t address = ESP_USB_HOST_ANY_ADDRESS,
                   uint32_t timeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS);
  bool mscReadBlocks(uint32_t lba,
                     uint8_t* data,
                     uint32_t blockCount,
                     uint8_t address = ESP_USB_HOST_ANY_ADDRESS,
                     uint32_t timeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS);
  bool mscWriteBlocks(uint32_t lba,
                      const uint8_t* data,
                      uint32_t blockCount,
                      uint8_t address = ESP_USB_HOST_ANY_ADDRESS,
                      uint32_t timeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS);
  bool mscReadBlocks64(uint64_t lba,
                       uint8_t* data,
                       uint32_t blockCount,
                       uint8_t address = ESP_USB_HOST_ANY_ADDRESS,
                       uint32_t timeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS);
  bool mscWriteBlocks64(uint64_t lba,
                        const uint8_t* data,
                        uint32_t blockCount,
                        uint8_t address = ESP_USB_HOST_ANY_ADDRESS,
                        uint32_t timeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS);
  bool mscSynchronizeCache(uint8_t address = ESP_USB_HOST_ANY_ADDRESS,
                           uint32_t timeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS);
  bool mscMount(const char* basePath = "/usb",
                uint8_t address = ESP_USB_HOST_ANY_ADDRESS,
                uint8_t lun = 0,
                uint8_t maxFiles = 4,
                uint32_t timeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS,
                bool skipSyncCache = false);
  bool mscUnmount(const char* basePath = "/usb");
  bool mscMounted(const char* basePath = "/usb") const;
#endif

  size_t deviceCount() const;
  size_t getDevices(EspUsbHostDeviceInfo* devices, size_t maxDevices) const;
  bool getDevice(uint8_t address, EspUsbHostDeviceInfo& device) const;
  int lastError() const;
  const char* lastErrorName() const;

private:
  struct EndpointState
  {
    usb_device_handle_t deviceHandle = nullptr;
    usb_transfer_t* transfer = nullptr;
    uint8_t lastKeyboardReport[8] = {};
    uint8_t deviceIndex = 0xff;
    uint8_t deviceAddress = 0;
    uint8_t address = 0;
    uint8_t interfaceNumber = 0;
    uint8_t interfaceClass = 0;
    uint8_t interfaceSubClass = 0;
    uint8_t interfaceProtocol = 0;
    uint8_t lastMouseButtons = 0;
    bool inUse = false;
    bool transferSubmitted = false;
    bool resubmitAfterLed = false;
    bool keyboardReportReady = false;
  };

  struct DeviceState
  {
#if ESP_USB_HOST_ENABLE_MSC
    uint64_t mscBlockCount64 = 0;
#endif
    usb_device_handle_t handle = nullptr;
    EspUsbHostDeviceInfo info;
    String manufacturer;
    String product;
    String serial;
    uint32_t keyboardLedDirtyTimeMs = 0;
#if ESP_USB_HOST_ENABLE_MSC
    uint32_t mscTag = 1;
    uint32_t mscBlockCount = 0;
    uint32_t mscBlockSize = 0;
    uint16_t mscInPacketSize = 0;
    uint16_t mscOutPacketSize = 0;
    EspUsbHostMscSense mscLastSense = {};
#endif
    uint8_t interfaces[ESP_USB_HOST_MAX_INTERFACES] = {};
    uint8_t interfaceCount = 0;
    uint8_t keyboardInterfaceNumber = 0;
    uint8_t keyboardLedLastSent = 0;
    uint8_t hubIndex = 0;
#if ESP_USB_HOST_ENABLE_MSC
    uint8_t mscInterfaceNumber = 0;
    uint8_t mscInEndpointAddress = 0;
    uint8_t mscOutEndpointAddress = 0;
    uint8_t mscMaxLun = 0;
    uint8_t mscLun = 0;
    bool hasMscInterface = false;
    bool hasMscInEndpoint = false;
    bool hasMscOutEndpoint = false;
    bool hasMscLastSense = false;
    bool hasMscMaxLun = false;
#endif
    bool inUse = false;
    bool isHub = false;
    bool hasKeyboardInterface = false;
    bool keyboardNumLock = true;
    bool keyboardCapsLock = false;
    bool keyboardScrollLock = false;
    bool keyboardLedPending = false;
    bool keyboardLedDirty = false;
  };

  static void taskEntry(void* arg);
  static void clientTaskEntry(void* arg);
  static void clientEventCallback(const usb_host_client_event_msg_t* eventMsg, void* arg);
  static void transferCallback(usb_transfer_t* transfer);
  static void controlTransferCallback(usb_transfer_t* transfer);
  static String usbString(const usb_str_desc_t* strDesc);

  void taskLoop();
  void clientTaskLoop();
  void handleClientEvent(const usb_host_client_event_msg_t* eventMsg);
  void handleNewDevice(uint8_t address);
  void handleDeviceGone(usb_device_handle_t goneHandle);
  void scanHostDevices();
  void refreshDeviceTopology(DeviceState& device);
  void parseConfigDescriptor(DeviceState& device, const usb_config_desc_t* configDesc);
  void handleDescriptor(uint8_t descriptorType, const uint8_t* data);
  void handleTransfer(usb_transfer_t* transfer);
  void handleKeyboard(EndpointState& endpoint,
                      const uint8_t* data,
                      size_t length,
                      const uint8_t* rawData,
                      size_t rawLength);
  void handleMouse(EndpointState& endpoint, const uint8_t* data, size_t length);

  EndpointState* findEndpoint(usb_device_handle_t deviceHandle, uint8_t endpointAddress);
  EndpointState* allocateEndpoint(DeviceState& device);
  DeviceState* allocateDevice();
  DeviceState* findDevice(uint8_t address);
  const DeviceState* findDevice(uint8_t address) const;
  DeviceState* findDeviceByHandle(usb_device_handle_t handle);
  DeviceState* findKeyboardDevice(uint8_t address);
  const DeviceState* findKeyboardDevice(uint8_t address) const;
#if ESP_USB_HOST_ENABLE_MSC
  DeviceState* findMscDevice(uint8_t address);
  const DeviceState* findMscDevice(uint8_t address) const;
  bool mscCommand(DeviceState& device,
                  const uint8_t* command,
                  uint8_t commandLength,
                  uint8_t* data,
                  size_t dataLength,
                  bool dataIn,
                  uint32_t timeoutMs);
  bool mscResetRecovery(DeviceState& device, uint32_t timeoutMs);
  void mscUnmountAddress(uint8_t address);
#endif

  void releaseEndpoints(DeviceState& device, bool clearEndpoints);
  void releaseAllEndpoints(bool clearEndpoints);
  void releaseInterfaces(DeviceState& device);
  bool submitInputTransfer(EndpointState& endpoint);
  bool submitSetInterface(DeviceState& device, uint8_t interfaceNumber, uint8_t alternateSetting);
  void submitPendingTransfers(usb_device_handle_t deviceHandle, uint8_t interfaceNumber);
  bool sendKeyboardLedReport(DeviceState& device, uint8_t leds);
  void setLastError(esp_err_t err);

  DeviceCallback deviceConnectedCallback_;
  DeviceCallback deviceDisconnectedCallback_;
  KeyboardCallback keyboardCallback_{nullptr};
  MouseCallback mouseCallback_{nullptr};
  void* keyboardCallbackArg_{nullptr};
  void* mouseCallbackArg_{nullptr};

  DeviceState devices_[ESP_USB_HOST_MAX_DEVICES];
  EndpointState endpoints_[ESP_USB_HOST_MAX_ENDPOINTS];
  EspUsbHostConfig config_;
  TaskHandle_t taskHandle_ = nullptr;
  TaskHandle_t clientTaskHandle_ = nullptr;
  usb_host_client_handle_t clientHandle_ = nullptr;
  DeviceState* currentDevice_ = nullptr;
  esp_err_t lastError_ = ESP_OK;
  esp_err_t currentClaimResult_ = ESP_OK;
  uint32_t lastHostDeviceScanMs_ = 0;
  EspUsbHostKeyboardLayout keyboardLayout_ = ESP_USB_HOST_KEYBOARD_LAYOUT_EN_US;
  uint8_t nextHubIndex_ = 1;
  uint8_t currentInterfaceNumber_ = 0;
  uint8_t currentInterfaceAlternate_ = 0;
  uint8_t currentInterfaceClass_ = 0;
  uint8_t currentInterfaceSubClass_ = 0;
  uint8_t currentInterfaceProtocol_ = 0;
  volatile bool running_ = false;
  volatile bool ready_ = false;
};

#if ESP_USB_HOST_ENABLE_MSC
class EspUsbHostMscFS : public fs::FS
{
public:
  EspUsbHostMscFS();
  ~EspUsbHostMscFS();

  bool begin(EspUsbHost& host,
             const char* basePath = "/usb",
             uint8_t address = ESP_USB_HOST_ANY_ADDRESS,
             uint8_t lun = 0,
             uint8_t maxFiles = 4,
             uint32_t timeoutMs = ESP_USB_HOST_MSC_DEFAULT_TIMEOUT_MS,
             bool skipSyncCache = false);
  void end();
  bool mounted() const;
  const char* basePath() const;
  void setSkipSyncCache(bool skip);
  bool skipSyncCache() const;

private:
  EspUsbHost* host_ = nullptr;
  char basePath_[16] = {};
  bool skipSyncCache_ = false;
};
#endif

#endif  // #ifndef CONFIG_IDF_TARGET_ESP32
#endif  // ESP_USB_HOST_H
