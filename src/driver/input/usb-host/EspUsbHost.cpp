#include "EspUsbHost.h"

#ifndef CONFIG_IDF_TARGET_ESP32

#include <string.h>

#include "EspUsbHostHid.h"

#if ESP_USB_HOST_ENABLE_MSC
#include "diskio_impl.h"
#include "esp_vfs_fat.h"
#include "vfs_api.h"
#endif

#if CORE_DEBUG_LEVEL >= ARDUHAL_LOG_LEVEL_ERROR
static const char* TAG = "EspUsbHost";
#endif

static constexpr uint8_t USB_CLASS_HID_VALUE = 0x03;
static constexpr uint8_t USB_CLASS_HUB_VALUE = 0x09;
static constexpr uint8_t USB_CLASS_MASS_STORAGE_VALUE = 0x08;

static constexpr uint8_t HID_SUBCLASS_BOOT_VALUE = 0x01;
static constexpr uint8_t HID_PROTOCOL_KEYBOARD_VALUE = 0x01;
static constexpr uint8_t HID_PROTOCOL_MOUSE_VALUE = 0x02;
static constexpr uint8_t HID_CLASS_REQUEST_SET_REPORT = 0x09;
static constexpr uint8_t HID_CLASS_REQUEST_SET_PROTOCOL = 0x0B;
static constexpr uint8_t HID_PROTOCOL_REPORT_MODE = 0x01;
static constexpr uint8_t HID_SET_REPORT_REQUEST_TYPE = 0x21;

static constexpr uint8_t USB_REQUEST_SET_INTERFACE = 0x0B;
static constexpr uint8_t USB_STANDARD_INTERFACE_REQUEST_TYPE = 0x01;

#if ESP_USB_HOST_ENABLE_MSC
static constexpr uint8_t USB_MSC_SUBCLASS_SCSI = 0x06;
static constexpr uint8_t USB_MSC_PROTOCOL_BULK_ONLY = 0x50;
static constexpr uint32_t USB_MSC_CBW_SIGNATURE = 0x43425355;
static constexpr uint32_t USB_MSC_CSW_SIGNATURE = 0x53425355;
static constexpr uint8_t USB_MSC_CSW_STATUS_PASSED = 0x00;
static constexpr uint8_t USB_MSC_CSW_STATUS_PHASE_ERROR = 0x02;
static constexpr uint8_t USB_MSC_RESET_REQUEST = 0xff;
static constexpr uint8_t USB_MSC_RESET_REQUEST_TYPE = 0x21;
static constexpr uint8_t USB_MSC_GET_MAX_LUN_REQUEST = 0xfe;
static constexpr uint8_t USB_MSC_GET_MAX_LUN_REQUEST_TYPE = 0xa1;
static constexpr uint8_t SCSI_CMD_TEST_UNIT_READY = 0x00;
static constexpr uint8_t SCSI_CMD_REQUEST_SENSE = 0x03;
static constexpr uint8_t SCSI_CMD_INQUIRY = 0x12;
static constexpr uint8_t SCSI_CMD_READ_CAPACITY_10 = 0x25;
static constexpr uint8_t SCSI_CMD_READ_10 = 0x28;
static constexpr uint8_t SCSI_CMD_WRITE_10 = 0x2a;
static constexpr uint8_t SCSI_CMD_SYNCHRONIZE_CACHE_10 = 0x35;
static constexpr uint8_t SCSI_CMD_READ_16 = 0x88;
static constexpr uint8_t SCSI_CMD_WRITE_16 = 0x8a;
static constexpr uint8_t SCSI_CMD_SERVICE_ACTION_IN_16 = 0x9e;
static constexpr uint8_t SCSI_SERVICE_ACTION_READ_CAPACITY_16 = 0x10;
static constexpr size_t USB_MSC_MAX_TRANSFER_BYTES = 4096;
static constexpr size_t ESP_USB_HOST_MAX_MSC_MOUNTS = 2;

#pragma pack(push, 1)
struct EspUsbHostMscCbw
{
  uint32_t signature;
  uint32_t tag;
  uint32_t dataTransferLength;
  uint8_t flags;
  uint8_t lun;
  uint8_t commandBlockLength;
  uint8_t commandBlock[16];
};

struct EspUsbHostMscCsw
{
  uint32_t signature;
  uint32_t tag;
  uint32_t dataResidue;
  uint8_t status;
};
#pragma pack(pop)

struct EspUsbHostSyncTransferContext
{
  SemaphoreHandle_t done = nullptr;
  size_t actualLength = 0;
  usb_transfer_status_t status = USB_TRANSFER_STATUS_ERROR;
};

struct EspUsbHostMscFatMount
{
  uint64_t blockCount = 0;
  EspUsbHost* host = nullptr;
  FATFS* fs = nullptr;
  uint32_t blockSize = 0;
  char basePath[16] = {};
  char fatDrive[8] = {};
  uint8_t address = 0;
  uint8_t lun = 0;
  uint8_t pdrv = FF_DRV_NOT_USED;
  bool inUse = false;
  bool skipSyncCache = false;
};

static EspUsbHostMscFatMount mscFatMounts[ESP_USB_HOST_MAX_MSC_MOUNTS] = {};

static void syncTransferCallback(usb_transfer_t* transfer)
{
  EspUsbHostSyncTransferContext* context =
      static_cast<EspUsbHostSyncTransferContext*>(transfer->context);
  if (!context)
    return;
  context->status = transfer->status;
  context->actualLength = transfer->actual_num_bytes;
  xSemaphoreGive(context->done);
}

static uint32_t readBe32(const uint8_t* data)
{
  return (static_cast<uint32_t>(data[0]) << 24) |
      (static_cast<uint32_t>(data[1]) << 16) |
      (static_cast<uint32_t>(data[2]) << 8) |
      static_cast<uint32_t>(data[3]);
}

static uint64_t readBe64(const uint8_t* data)
{
  return (static_cast<uint64_t>(readBe32(data)) << 32) | readBe32(data + 4);
}

static void writeBe32(uint8_t* data, uint32_t value)
{
  data[0] = (value >> 24) & 0xff;
  data[1] = (value >> 16) & 0xff;
  data[2] = (value >> 8) & 0xff;
  data[3] = value & 0xff;
}

static void writeBe64(uint8_t* data, uint64_t value)
{
  writeBe32(data, static_cast<uint32_t>(value >> 32));
  writeBe32(data + 4, static_cast<uint32_t>(value & 0xffffffffULL));
}

static EspUsbHostMscFatMount* findMscFatMountByDrive(uint8_t pdrv)
{
  for (EspUsbHostMscFatMount& mount : mscFatMounts)
  {
    if (mount.inUse && mount.pdrv == pdrv)
      return &mount;
  }
  return nullptr;
}

static EspUsbHostMscFatMount* findMscFatMountByPath(const char* basePath)
{
  if (!basePath)
    return nullptr;
  for (EspUsbHostMscFatMount& mount : mscFatMounts)
  {
    if (mount.inUse && strcmp(mount.basePath, basePath) == 0)
      return &mount;
  }
  return nullptr;
}

static DSTATUS mscFatDiskInitialize(BYTE pdrv)
{
  return findMscFatMountByDrive(pdrv) ? 0 : STA_NOINIT;
}

static DSTATUS mscFatDiskStatus(BYTE pdrv)
{
  EspUsbHostMscFatMount* mount = findMscFatMountByDrive(pdrv);
  return mount && mount->host && mount->host->mscReady(mount->address) ? 0 : STA_NOINIT;
}

static DRESULT mscFatDiskRead(BYTE pdrv, BYTE* buff, uint32_t sector, unsigned count)
{
  EspUsbHostMscFatMount* mount = findMscFatMountByDrive(pdrv);
  if (!mount || !mount->host || !buff || count == 0)
    return RES_PARERR;
  mount->host->mscSelectLun(mount->lun, mount->address);
  return mount->host->mscReadBlocks64(sector, buff, count, mount->address) ? RES_OK : RES_ERROR;
}

static DRESULT mscFatDiskWrite(BYTE pdrv, const BYTE* buff, uint32_t sector, unsigned count)
{
  EspUsbHostMscFatMount* mount = findMscFatMountByDrive(pdrv);
  if (!mount || !mount->host || !buff || count == 0)
    return RES_PARERR;
  mount->host->mscSelectLun(mount->lun, mount->address);
  return mount->host->mscWriteBlocks64(sector, buff, count, mount->address) ? RES_OK : RES_ERROR;
}

static DRESULT mscFatDiskIoctl(BYTE pdrv, BYTE cmd, void* buff)
{
  EspUsbHostMscFatMount* mount = findMscFatMountByDrive(pdrv);
  if (!mount || !mount->host)
    return RES_NOTRDY;

  switch (cmd)
  {
    case CTRL_SYNC:
      if (!mount->host->mscReady(mount->address))
        return RES_NOTRDY;
      if (mount->skipSyncCache)
        return RES_OK;
      if (mount->host->mscSynchronizeCache(mount->address))
        return RES_OK;
      mount->skipSyncCache = true;
      return RES_OK;
    case GET_SECTOR_COUNT:
      if (!buff)
        return RES_PARERR;
      *static_cast<LBA_t*>(buff) = static_cast<LBA_t>(mount->blockCount);
      return RES_OK;
    case GET_SECTOR_SIZE:
      if (!buff)
        return RES_PARERR;
      *static_cast<WORD*>(buff) = static_cast<WORD>(mount->blockSize);
      return RES_OK;
    case GET_BLOCK_SIZE:
      if (!buff)
        return RES_PARERR;
      *static_cast<DWORD*>(buff) = 1;
      return RES_OK;
    default:
      return RES_PARERR;
  }
}

static const ff_diskio_impl_t MSC_FAT_DISKIO = {
    .init = &mscFatDiskInitialize,
    .status = &mscFatDiskStatus,
    .read = &mscFatDiskRead,
    .write = &mscFatDiskWrite,
    .ioctl = &mscFatDiskIoctl,
};
#endif  // ESP_USB_HOST_ENABLE_MSC

static unsigned hostPeripheralMap(EspUsbHostPort port)
{
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  switch (port)
  {
    case ESP_USB_HOST_PORT_HIGH_SPEED:
      return 1U << 0;
    case ESP_USB_HOST_PORT_FULL_SPEED:
      return 1U << 1;
    case ESP_USB_HOST_PORT_DEFAULT:
    default:
      return 0;
  }
#else
  (void)port;
  return 0;
#endif
}

static bool configHasInterfaceClass(const usb_config_desc_t* configDesc, uint8_t interfaceClass)
{
  const uint8_t* p = reinterpret_cast<const uint8_t*>(configDesc);
  for (int i = 0; i < configDesc->wTotalLength;)
  {
    const uint8_t length = p[i];
    if (length < 2 || i + length > configDesc->wTotalLength)
      return false;
    if (p[i + 1] == USB_INTERFACE_DESC)
    {
      const usb_intf_desc_t* intf = reinterpret_cast<const usb_intf_desc_t*>(&p[i]);
      if (intf->bInterfaceClass == interfaceClass)
        return true;
    }
    i += length;
  }
  return false;
}

static const char* speedName(usb_speed_t speed)
{
  switch (speed)
  {
    case USB_SPEED_LOW:
      return "low-speed";
    case USB_SPEED_FULL:
      return "full-speed";
    case USB_SPEED_HIGH:
      return "high-speed";
    default:
      return "unknown";
  }
}

static const char* className(uint8_t cls)
{
  switch (cls)
  {
    case 0x00:
      return "per-interface";
    case 0x01:
      return "Audio";
    case 0x02:
      return "CDC Control";
    case 0x03:
      return "HID";
    case 0x08:
      return "Mass Storage";
    case 0x09:
      return "Hub";
    case 0x0a:
      return "CDC Data";
    case 0x0e:
      return "Video";
    case 0xff:
      return "Vendor";
    default:
      return "Unknown";
  }
}

void espUsbHostPrint(const EspUsbHostDeviceInfo& device, Print& out)
{
  out.printf("device: address=%u portId=0x%02x vid=%04x pid=%04x class=0x%02x(%s) speed=%s product=\"%s\"\n",
             device.address, device.portId, device.vid, device.pid,
             device.deviceClass, className(device.deviceClass),
             speedName(device.speed), device.product);
}

EspUsbHost::EspUsbHost() = default;

EspUsbHost::~EspUsbHost()
{
  end();
}

bool EspUsbHost::begin()
{
  return begin(EspUsbHostConfig());
}

bool EspUsbHost::begin(const EspUsbHostConfig& config)
{
  if (running_)
    return true;

  config_ = config;
  running_ = true;
  ready_ = false;
  lastError_ = ESP_OK;
  nextHubIndex_ = 1;

  BaseType_t created;
  if (config_.taskCore == tskNO_AFFINITY)
  {
    created = xTaskCreate(taskEntry, "EspUsbHost", config_.taskStackSize, this,
                          config_.taskPriority, &taskHandle_);
  }
  else
  {
    created = xTaskCreatePinnedToCore(taskEntry, "EspUsbHost", config_.taskStackSize, this,
                                      config_.taskPriority, &taskHandle_, config_.taskCore);
  }

  if (created != pdPASS)
  {
    running_ = false;
    setLastError(ESP_ERR_NO_MEM);
    return false;
  }

  const uint32_t start = millis();
  while (running_ && !ready_ && millis() - start < 1000)
    delay(1);
  return ready_;
}

void EspUsbHost::end()
{
  if (!running_ && !taskHandle_ && !clientTaskHandle_)
    return;

  running_ = false;

  // Даємо taskLoop() час штатно зупинити clientTask і виконати usb_host_uninstall()
  const uint32_t deadline = millis() + 1000;
  while ((taskHandle_ || clientTaskHandle_) && millis() < deadline)
    delay(5);

  if (clientTaskHandle_)
  {
    vTaskDelete(clientTaskHandle_);
    clientTaskHandle_ = nullptr;
  }
  if (taskHandle_)
  {
    vTaskDelete(taskHandle_);
    taskHandle_ = nullptr;
  }
  ready_ = false;
}

bool EspUsbHost::ready() const
{
  return ready_;
}

void EspUsbHost::onDeviceConnected(DeviceCallback callback)
{
  deviceConnectedCallback_ = callback;
}

void EspUsbHost::onDeviceDisconnected(DeviceCallback callback)
{
  deviceDisconnectedCallback_ = callback;
}

void EspUsbHost::onKeyboard(KeyboardCallback callback, void* arg)
{
  keyboardCallback_ = callback;
  keyboardCallbackArg_ = arg;
}

void EspUsbHost::onMouse(MouseCallback callback, void* arg)
{
  mouseCallback_ = callback;
  mouseCallbackArg_ = arg;
}

void EspUsbHost::setKeyboardLayout(EspUsbHostKeyboardLayout layout)
{
  keyboardLayout_ = layout;
}

EspUsbHostKeyboardLayout EspUsbHost::getKeyboardLayout() const
{
  return keyboardLayout_;
}

bool EspUsbHost::sendHIDReport(uint8_t interfaceNumber,
                               uint8_t reportType,
                               uint8_t reportId,
                               const uint8_t* data,
                               size_t length,
                               uint8_t address)
{
  DeviceState* device = findDevice(address);
  if (!running_ || !device || !device->handle || !clientHandle_)
    return false;
  if (length > 0 && !data)
    return false;
  if (reportType != ESP_USB_HOST_HID_REPORT_TYPE_OUTPUT &&
      reportType != ESP_USB_HOST_HID_REPORT_TYPE_FEATURE)
    return false;

  usb_transfer_t* transfer = nullptr;
  esp_err_t err = usb_host_transfer_alloc(USB_SETUP_PACKET_SIZE + length, 0, &transfer);
  if (err != ESP_OK)
  {
    setLastError(err);
    return false;
  }

  usb_setup_packet_t* setup = reinterpret_cast<usb_setup_packet_t*>(transfer->data_buffer);
  setup->bmRequestType = HID_SET_REPORT_REQUEST_TYPE;
  setup->bRequest = HID_CLASS_REQUEST_SET_REPORT;
  setup->wValue = static_cast<uint16_t>((reportType << 8) | reportId);
  setup->wIndex = interfaceNumber;
  setup->wLength = static_cast<uint16_t>(length);
  if (length > 0)
    memcpy(transfer->data_buffer + USB_SETUP_PACKET_SIZE, data, length);

  transfer->device_handle = device->handle;
  transfer->bEndpointAddress = 0;
  transfer->callback = controlTransferCallback;
  transfer->context = this;
  transfer->num_bytes = USB_SETUP_PACKET_SIZE + length;

  err = usb_host_transfer_submit_control(clientHandle_, transfer);
  if (err != ESP_OK)
  {
    setLastError(err);
    usb_host_transfer_free(transfer);
    return false;
  }
  return true;
}

bool EspUsbHost::sendSetProtocol(uint8_t interfaceNumber, uint8_t address)
{
  DeviceState* device = findDevice(address);
  if (!running_ || !device || !device->handle || !clientHandle_)
    return false;

  usb_transfer_t* transfer = nullptr;
  esp_err_t err = usb_host_transfer_alloc(USB_SETUP_PACKET_SIZE, 0, &transfer);
  if (err != ESP_OK)
    return false;

  usb_setup_packet_t* setup = reinterpret_cast<usb_setup_packet_t*>(transfer->data_buffer);
  setup->bmRequestType = HID_SET_REPORT_REQUEST_TYPE;
  setup->bRequest = HID_CLASS_REQUEST_SET_PROTOCOL;
  setup->wValue = HID_PROTOCOL_REPORT_MODE;
  setup->wIndex = interfaceNumber;
  setup->wLength = 0;

  transfer->device_handle = device->handle;
  transfer->bEndpointAddress = 0;
  transfer->callback = controlTransferCallback;
  transfer->context = this;
  transfer->num_bytes = USB_SETUP_PACKET_SIZE;

  err = usb_host_transfer_submit_control(clientHandle_, transfer);
  if (err != ESP_OK)
  {
    setLastError(err);
    usb_host_transfer_free(transfer);
    return false;
  }
  return true;
}

bool EspUsbHost::setKeyboardLeds(bool numLock, bool capsLock, bool scrollLock, uint8_t address)
{
  DeviceState* device = findKeyboardDevice(address);
  if (!device)
    return false;

  device->keyboardNumLock = numLock;
  device->keyboardCapsLock = capsLock;
  device->keyboardScrollLock = scrollLock;
  const uint8_t leds = espUsbHostBuildKeyboardLedReport(numLock, capsLock, scrollLock);
  return sendKeyboardLedReport(*device, leds);
}

bool EspUsbHost::getKeyboardNumLock(uint8_t address) const
{
  const DeviceState* device = findKeyboardDevice(address);
  return device ? device->keyboardNumLock : false;
}

bool EspUsbHost::getKeyboardCapsLock(uint8_t address) const
{
  const DeviceState* device = findKeyboardDevice(address);
  return device ? device->keyboardCapsLock : false;
}

bool EspUsbHost::getKeyboardScrollLock(uint8_t address) const
{
  const DeviceState* device = findKeyboardDevice(address);
  return device ? device->keyboardScrollLock : false;
}

bool EspUsbHost::sendKeyboardLedReport(DeviceState& device, uint8_t leds)
{
  if (device.keyboardLedPending)
    return false;
  if (sendHIDReport(device.keyboardInterfaceNumber, ESP_USB_HOST_HID_REPORT_TYPE_OUTPUT, 0,
                    &leds, sizeof(leds), device.info.address))
  {
    device.keyboardLedPending = true;
    device.keyboardLedLastSent = leds;
    return true;
  }
  return false;
}

size_t EspUsbHost::deviceCount() const
{
  size_t count = 0;
  for (const DeviceState& device : devices_)
  {
    if (device.inUse)
      count++;
  }
  return count;
}

size_t EspUsbHost::getDevices(EspUsbHostDeviceInfo* devices, size_t maxDevices) const
{
  if (!devices || maxDevices == 0)
    return 0;
  size_t count = 0;
  for (const DeviceState& device : devices_)
  {
    if (!device.inUse)
      continue;
    devices[count++] = device.info;
    if (count >= maxDevices)
      break;
  }
  return count;
}

bool EspUsbHost::getDevice(uint8_t address, EspUsbHostDeviceInfo& deviceInfo) const
{
  const DeviceState* device = findDevice(address);
  if (!device)
    return false;
  deviceInfo = device->info;
  return true;
}

int EspUsbHost::lastError() const
{
  return lastError_;
}

const char* EspUsbHost::lastErrorName() const
{
  return esp_err_to_name(lastError_);
}

void EspUsbHost::setLastError(esp_err_t err)
{
  lastError_ = err;
}

String EspUsbHost::usbString(const usb_str_desc_t* strDesc)
{
  if (!strDesc || strDesc->bLength <= 2)
    return String();
  const size_t charCount = (strDesc->bLength - 2) / 2;
  String result;
  result.reserve(charCount);
  for (size_t i = 0; i < charCount; i++)
  {
    const uint16_t ch = strDesc->wData[i];
    result += (ch < 0x80) ? static_cast<char>(ch) : '?';
  }
  return result;
}

void EspUsbHost::taskEntry(void* arg)
{
  static_cast<EspUsbHost*>(arg)->taskLoop();
}

void EspUsbHost::clientTaskEntry(void* arg)
{
  static_cast<EspUsbHost*>(arg)->clientTaskLoop();
}

void EspUsbHost::taskLoop()
{
  usb_host_config_t hostConfig = {};
  hostConfig.skip_phy_setup = false;
  hostConfig.intr_flags = ESP_INTR_FLAG_LOWMED;
  hostConfig.peripheral_map = hostPeripheralMap(config_.port);

  esp_err_t err = usb_host_install(&hostConfig);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "usb_host_install() failed: %s", esp_err_to_name(err));
    setLastError(err);
    running_ = false;
    taskHandle_ = nullptr;
    vTaskDelete(nullptr);
    return;
  }

  usb_host_client_config_t clientConfig = {};
  clientConfig.is_synchronous = false;
  clientConfig.max_num_event_msg = 10;
  clientConfig.async.client_event_callback = clientEventCallback;
  clientConfig.async.callback_arg = this;

  err = usb_host_client_register(&clientConfig, &clientHandle_);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "usb_host_client_register() failed: %s", esp_err_to_name(err));
    setLastError(err);
    usb_host_uninstall();
    running_ = false;
    taskHandle_ = nullptr;
    vTaskDelete(nullptr);
    return;
  }

  BaseType_t created;
  if (config_.taskCore == tskNO_AFFINITY)
  {
    created = xTaskCreate(clientTaskEntry, "EspUsbHostClient", config_.taskStackSize, this,
                          config_.taskPriority, &clientTaskHandle_);
  }
  else
  {
    created = xTaskCreatePinnedToCore(clientTaskEntry, "EspUsbHostClient", config_.taskStackSize,
                                      this, config_.taskPriority, &clientTaskHandle_,
                                      config_.taskCore);
  }
  if (created != pdPASS)
  {
    setLastError(ESP_ERR_NO_MEM);
    running_ = false;
  }

  ready_ = running_;

  // 1. Таймаут 50 мс замість portMAX_DELAY, щоб цикл прокидався при running_ == false
  while (running_)
  {
    uint32_t eventFlags = 0;
    err = usb_host_lib_handle_events(pdMS_TO_TICKS(50), &eventFlags);
    if (err != ESP_OK && err != ESP_ERR_TIMEOUT)
      setLastError(err);
  }

  // 2. Чекаємо штатного завершення clientTaskLoop() (він виходить протягом <= 5 мс)
  for (int i = 0; i < 40 && clientTaskHandle_; i++)
    delay(5);
  if (clientTaskHandle_)
  {
    vTaskDelete(clientTaskHandle_);
    clientTaskHandle_ = nullptr;
  }

  // 3. Зупиняємо і скидаємо всі активні ендпоїнти, щоб відпустити in-flight трансфери
  for (EndpointState& ep : endpoints_)
  {
    if (ep.inUse && ep.deviceHandle)
    {
      usb_host_endpoint_halt(ep.deviceHandle, ep.address);
      usb_host_endpoint_flush(ep.deviceHandle, ep.address);
      usb_host_endpoint_clear(ep.deviceHandle, ep.address);
    }
  }

  // 4. Прокручуємо події клієнта, щоб спрацювали колбеки скасованих трансферів
  if (clientHandle_)
  {
    for (int i = 0; i < 10; i++)
    {
      if (usb_host_client_handle_events(clientHandle_, pdMS_TO_TICKS(10)) == ESP_ERR_TIMEOUT)
        break;
    }
  }

  // 5. Звільняємо трансфери, інтерфейси та закриваємо пристрої
  releaseAllEndpoints(false);
  for (DeviceState& device : devices_)
  {
    if (!device.inUse)
      continue;
#if ESP_USB_HOST_ENABLE_MSC
    if (device.hasMscInterface)
      mscUnmountAddress(device.info.address);
#endif
    releaseInterfaces(device);
    if (device.handle)
      usb_host_device_close(clientHandle_, device.handle);
    device = DeviceState();
  }

  // 6. Дереєструємо клієнта
  if (clientHandle_)
  {
    err = usb_host_client_deregister(clientHandle_);
    if (err != ESP_OK)
      ESP_LOGW(TAG, "usb_host_client_deregister() failed: %s", esp_err_to_name(err));
    clientHandle_ = nullptr;
  }

  // 7. Звільняємо пристрої в бібліотеці і чекаємо USB_HOST_LIB_EVENT_FLAGS_ALL_FREE
  err = usb_host_device_free_all();
  if (err == ESP_ERR_NOT_FINISHED)
  {
    const uint32_t freeDeadline = millis() + 500;
    while (millis() < freeDeadline)
    {
      uint32_t eventFlags = 0;
      usb_host_lib_handle_events(pdMS_TO_TICKS(20), &eventFlags);
      if (eventFlags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE)
        break;
    }
  }

  // 8. Деінсталюємо USB Host
  err = usb_host_uninstall();
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "usb_host_uninstall() failed: %s", esp_err_to_name(err));
    setLastError(err);
  }
  else
  {
    ESP_LOGI(TAG, "USB Host uninstalled");
  }

  ready_ = false;
  taskHandle_ = nullptr;
  vTaskDelete(nullptr);
}

void EspUsbHost::clientTaskLoop()
{
  while (running_)
  {
    esp_err_t err = usb_host_client_handle_events(clientHandle_, pdMS_TO_TICKS(5));
    if (err != ESP_OK && err != ESP_ERR_TIMEOUT)
      setLastError(err);

    for (DeviceState& device : devices_)
    {
      if (!device.inUse || !device.hasKeyboardInterface || !device.keyboardLedDirty ||
          device.keyboardLedPending)
        continue;
      if (millis() - device.keyboardLedDirtyTimeMs < 20)
        continue;
      const uint8_t leds = espUsbHostBuildKeyboardLedReport(
          device.keyboardNumLock, device.keyboardCapsLock, device.keyboardScrollLock);
      device.keyboardLedDirty = false;
      sendKeyboardLedReport(device, leds);
    }

    for (EndpointState& ep : endpoints_)
    {
      if (!ep.inUse || !ep.resubmitAfterLed)
        continue;
      DeviceState* dev = findDeviceByHandle(ep.deviceHandle);
      if (dev && (dev->keyboardLedDirty || dev->keyboardLedPending))
        continue;
      ep.resubmitAfterLed = false;
      if (ep.transfer && running_ && ep.deviceHandle)
        usb_host_transfer_submit(ep.transfer);
    }

    if (millis() - lastHostDeviceScanMs_ >= 500)
    {
      lastHostDeviceScanMs_ = millis();
      scanHostDevices();
    }
  }
  clientTaskHandle_ = nullptr;
  vTaskDelete(nullptr);
}

void EspUsbHost::clientEventCallback(const usb_host_client_event_msg_t* eventMsg, void* arg)
{
  static_cast<EspUsbHost*>(arg)->handleClientEvent(eventMsg);
}

void EspUsbHost::handleClientEvent(const usb_host_client_event_msg_t* eventMsg)
{
  switch (eventMsg->event)
  {
    case USB_HOST_CLIENT_EVENT_NEW_DEV:
      handleNewDevice(eventMsg->new_dev.address);
      break;
    case USB_HOST_CLIENT_EVENT_DEV_GONE:
      handleDeviceGone(eventMsg->dev_gone.dev_hdl);
      break;
    default:
      break;
  }
}

void EspUsbHost::handleNewDevice(uint8_t address)
{
  DeviceState* device = allocateDevice();
  if (!device)
  {
    setLastError(ESP_ERR_NO_MEM);
    return;
  }
  device->inUse = true;
  device->info.address = address;

  esp_err_t err = usb_host_device_open(clientHandle_, address, &device->handle);
  if (err != ESP_OK)
  {
    setLastError(err);
    *device = DeviceState();
    return;
  }

  const usb_device_desc_t* devDesc = nullptr;
  if (usb_host_get_device_descriptor(device->handle, &devDesc) == ESP_OK && devDesc)
  {
    device->info.vid = devDesc->idVendor;
    device->info.pid = devDesc->idProduct;
    device->info.usbVersion = devDesc->bcdUSB;
    device->info.deviceVersion = devDesc->bcdDevice;
    device->info.deviceClass = devDesc->bDeviceClass;
    device->info.deviceSubClass = devDesc->bDeviceSubClass;
    device->info.deviceProtocol = devDesc->bDeviceProtocol;
    device->info.maxPacketSize0 = devDesc->bMaxPacketSize0;
  }

  usb_device_info_t devInfo = {};
  if (usb_host_device_info(device->handle, &devInfo) == ESP_OK)
  {
    device->manufacturer = usbString(devInfo.str_desc_manufacturer);
    device->product = usbString(devInfo.str_desc_product);
    device->serial = usbString(devInfo.str_desc_serial_num);
    device->info.manufacturer = device->manufacturer.c_str();
    device->info.product = device->product.c_str();
    device->info.serial = device->serial.c_str();
    device->info.address = devInfo.dev_addr;
    device->info.speed = devInfo.speed;
    device->info.maxPacketSize0 = devInfo.bMaxPacketSize0;
    device->info.configurationValue = devInfo.bConfigurationValue;

    const uint8_t upstreamPort = devInfo.parent.port_num;
    if (devInfo.parent.dev_hdl)
    {
      DeviceState* parent = findDeviceByHandle(devInfo.parent.dev_hdl);
      device->info.parentAddress = parent ? parent->info.address : 0;
      device->info.portId = (parent && parent->hubIndex && upstreamPort > 0 && upstreamPort <= 0x0f) ? static_cast<uint8_t>((parent->hubIndex << 4) | upstreamPort) : upstreamPort;
    }
    else
    {
      device->info.portId = upstreamPort ? upstreamPort : 0x01;
    }
  }

  const usb_config_desc_t* configDesc = nullptr;
  err = usb_host_get_active_config_descriptor(device->handle, &configDesc);
  if (err != ESP_OK || !configDesc)
  {
    setLastError(err);
    usb_host_device_close(clientHandle_, device->handle);
    *device = DeviceState();
    return;
  }

  device->info.configurationValue = configDesc->bConfigurationValue;
  device->info.configurationAttributes = configDesc->bmAttributes;
  device->info.configurationMaxPower = configDesc->bMaxPower;
  device->info.configurationInterfaceCount = configDesc->bNumInterfaces;
  device->info.configurationTotalLength = configDesc->wTotalLength;

  const bool isHub = devDesc && (devDesc->bDeviceClass == USB_CLASS_HUB_VALUE || configHasInterfaceClass(configDesc, USB_CLASS_HUB_VALUE));
  device->isHub = isHub;
  if (isHub && nextHubIndex_ <= 0x0f)
    device->hubIndex = nextHubIndex_++;

  parseConfigDescriptor(*device, configDesc);

  const bool hasHid = configHasInterfaceClass(configDesc, USB_CLASS_HID_VALUE);
#if ESP_USB_HOST_ENABLE_MSC
  const bool hasMsc =
      device->hasMscInterface && device->hasMscInEndpoint && device->hasMscOutEndpoint;
#else
  const bool hasMsc = false;
#endif
  device->info.supported = isHub || hasHid || hasMsc;
  device->info.isHub = isHub;

  if (deviceConnectedCallback_)
    deviceConnectedCallback_(device->info);

  if (device->hasKeyboardInterface)
  {
    const uint8_t leds = espUsbHostBuildKeyboardLedReport(
        device->keyboardNumLock, device->keyboardCapsLock, device->keyboardScrollLock);
    sendKeyboardLedReport(*device, leds);
  }
}

void EspUsbHost::handleDeviceGone(usb_device_handle_t goneHandle)
{
  DeviceState* device = findDeviceByHandle(goneHandle);
  if (!device)
  {
    if (goneHandle)
      usb_host_device_close(clientHandle_, goneHandle);
    return;
  }

  EspUsbHostDeviceInfo info = device->info;
#if ESP_USB_HOST_ENABLE_MSC
  if (device->hasMscInterface)
    mscUnmountAddress(device->info.address);
#endif
  releaseEndpoints(*device, false);
  releaseInterfaces(*device);
  if (device->handle)
    usb_host_device_close(clientHandle_, device->handle);

  if (deviceDisconnectedCallback_)
    deviceDisconnectedCallback_(info);
  *device = DeviceState();
}

void EspUsbHost::refreshDeviceTopology(DeviceState& device)
{
  if (!device.inUse || !device.handle)
    return;

  usb_device_info_t devInfo = {};
  if (usb_host_device_info(device.handle, &devInfo) != ESP_OK)
    return;

  const uint8_t upstreamPort = devInfo.parent.port_num;
  device.info.address = devInfo.dev_addr;
  device.info.speed = devInfo.speed;
  device.info.maxPacketSize0 = devInfo.bMaxPacketSize0;
  device.info.configurationValue = devInfo.bConfigurationValue;

  if (devInfo.parent.dev_hdl)
  {
    DeviceState* parent = findDeviceByHandle(devInfo.parent.dev_hdl);
    device.info.parentAddress = parent ? parent->info.address : 0;
    device.info.portId = (parent && parent->hubIndex && upstreamPort > 0 && upstreamPort <= 0x0f) ? static_cast<uint8_t>((parent->hubIndex << 4) | upstreamPort) : upstreamPort;
  }
  else
  {
    device.info.parentAddress = 0;
    device.info.portId = upstreamPort ? upstreamPort : 0x01;
  }
}

void EspUsbHost::scanHostDevices()
{
  if (!running_ || !clientHandle_)
    return;

  uint8_t addresses[ESP_USB_HOST_MAX_DEVICES * 2] = {};
  int addressCount = 0;
  if (usb_host_device_addr_list_fill(static_cast<int>(sizeof(addresses)), addresses,
                                     &addressCount) != ESP_OK)
    return;

  for (int i = 0; i < addressCount; i++)
  {
    const uint8_t address = addresses[i];
    if (address == 0 || findDevice(address))
      continue;

    usb_device_handle_t handle = nullptr;
    if (usb_host_device_open(clientHandle_, address, &handle) != ESP_OK)
      continue;

    const usb_device_desc_t* devDesc = nullptr;
    const usb_config_desc_t* configDesc = nullptr;
    const bool hasDeviceDesc = usb_host_get_device_descriptor(handle, &devDesc) == ESP_OK && devDesc;
    const bool hasConfigDesc =
        usb_host_get_active_config_descriptor(handle, &configDesc) == ESP_OK && configDesc;
    const bool isHub = hasDeviceDesc && hasConfigDesc &&
        (devDesc->bDeviceClass == USB_CLASS_HUB_VALUE ||
         configHasInterfaceClass(configDesc, USB_CLASS_HUB_VALUE));
    if (!isHub)
    {
      usb_host_device_close(clientHandle_, handle);
      continue;
    }

    DeviceState* device = allocateDevice();
    if (!device)
    {
      usb_host_device_close(clientHandle_, handle);
      return;
    }

    device->inUse = true;
    device->handle = handle;
    device->info.address = address;

    usb_device_info_t devInfo = {};
    if (usb_host_device_info(device->handle, &devInfo) == ESP_OK)
    {
      device->manufacturer = usbString(devInfo.str_desc_manufacturer);
      device->product = usbString(devInfo.str_desc_product);
      device->serial = usbString(devInfo.str_desc_serial_num);
      device->info.manufacturer = device->manufacturer.c_str();
      device->info.product = device->product.c_str();
      device->info.serial = device->serial.c_str();
    }

    device->info.vid = devDesc->idVendor;
    device->info.pid = devDesc->idProduct;
    device->info.usbVersion = devDesc->bcdUSB;
    device->info.deviceVersion = devDesc->bcdDevice;
    device->info.deviceClass = devDesc->bDeviceClass;
    device->info.deviceSubClass = devDesc->bDeviceSubClass;
    device->info.deviceProtocol = devDesc->bDeviceProtocol;
    device->info.maxPacketSize0 = devDesc->bMaxPacketSize0;
    device->info.configurationValue = configDesc->bConfigurationValue;
    device->info.configurationAttributes = configDesc->bmAttributes;
    device->info.configurationMaxPower = configDesc->bMaxPower;
    device->info.configurationInterfaceCount = configDesc->bNumInterfaces;
    device->info.configurationTotalLength = configDesc->wTotalLength;
    device->isHub = true;
    device->info.isHub = true;
    device->info.supported = true;
    if (nextHubIndex_ <= 0x0f)
      device->hubIndex = nextHubIndex_++;

    parseConfigDescriptor(*device, configDesc);
    refreshDeviceTopology(*device);

    if (deviceConnectedCallback_)
      deviceConnectedCallback_(device->info);
  }

  for (DeviceState& device : devices_)
    refreshDeviceTopology(device);
}

void EspUsbHost::parseConfigDescriptor(DeviceState& device, const usb_config_desc_t* configDesc)
{
  const uint8_t* p = reinterpret_cast<const uint8_t*>(configDesc);
  for (int i = 0; i < configDesc->wTotalLength;)
  {
    const uint8_t length = p[i];
    if (length < 2 || i + length > configDesc->wTotalLength)
      return;
    currentDevice_ = &device;
    handleDescriptor(p[i + 1], &p[i]);
    i += length;
  }
  currentDevice_ = nullptr;
}

void EspUsbHost::handleDescriptor(uint8_t descriptorType, const uint8_t* data)
{
  DeviceState* device = currentDevice_;
  if (!device)
    return;

  switch (descriptorType)
  {
    case USB_INTERFACE_DESC:
    {
      const usb_intf_desc_t* intf = reinterpret_cast<const usb_intf_desc_t*>(data);
      currentInterfaceNumber_ = intf->bInterfaceNumber;
      currentInterfaceAlternate_ = intf->bAlternateSetting;
      currentInterfaceClass_ = intf->bInterfaceClass;
      currentInterfaceSubClass_ = intf->bInterfaceSubClass;
      currentInterfaceProtocol_ = intf->bInterfaceProtocol;
      currentClaimResult_ = ESP_FAIL;

      const bool isHid = currentInterfaceClass_ == USB_CLASS_HID_VALUE;
#if ESP_USB_HOST_ENABLE_MSC
      const bool isMsc = currentInterfaceClass_ == USB_CLASS_MASS_STORAGE_VALUE &&
          currentInterfaceSubClass_ == USB_MSC_SUBCLASS_SCSI &&
          currentInterfaceProtocol_ == USB_MSC_PROTOCOL_BULK_ONLY &&
          !device->hasMscInterface;
#else
      const bool isMsc = false;
#endif
      if (!isHid && !isMsc)
        break;

      currentClaimResult_ = usb_host_interface_claim(clientHandle_, device->handle,
                                                     currentInterfaceNumber_,
                                                     intf->bAlternateSetting);
      if (currentClaimResult_ == ESP_OK && device->interfaceCount < ESP_USB_HOST_MAX_INTERFACES)
      {
        device->interfaces[device->interfaceCount++] = currentInterfaceNumber_;
        if (currentInterfaceAlternate_ > 0)
          submitSetInterface(*device, currentInterfaceNumber_, currentInterfaceAlternate_);

        if (isHid && currentInterfaceSubClass_ == HID_SUBCLASS_BOOT_VALUE &&
            currentInterfaceProtocol_ == HID_PROTOCOL_KEYBOARD_VALUE &&
            !device->hasKeyboardInterface)
        {
          device->hasKeyboardInterface = true;
          device->keyboardInterfaceNumber = currentInterfaceNumber_;
        }
#if ESP_USB_HOST_ENABLE_MSC
        else if (isMsc)
        {
          device->hasMscInterface = true;
          device->mscInterfaceNumber = currentInterfaceNumber_;
        }
#endif
      }
      else
      {
        setLastError(currentClaimResult_);
      }
      break;
    }

    case USB_ENDPOINT_DESC:
    {
      if (currentClaimResult_ != ESP_OK)
        break;

      const usb_ep_desc_t* ep = reinterpret_cast<const usb_ep_desc_t*>(data);
      const bool isIn = (ep->bEndpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK) != 0;
      const uint8_t xferType = ep->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK;

#if ESP_USB_HOST_ENABLE_MSC
      if (currentInterfaceClass_ == USB_CLASS_MASS_STORAGE_VALUE &&
          currentInterfaceSubClass_ == USB_MSC_SUBCLASS_SCSI &&
          currentInterfaceProtocol_ == USB_MSC_PROTOCOL_BULK_ONLY &&
          xferType == USB_BM_ATTRIBUTES_XFER_BULK)
      {
        if (isIn)
        {
          device->hasMscInEndpoint = true;
          device->mscInEndpointAddress = ep->bEndpointAddress;
          device->mscInPacketSize = ep->wMaxPacketSize;
        }
        else
        {
          device->hasMscOutEndpoint = true;
          device->mscOutEndpointAddress = ep->bEndpointAddress;
          device->mscOutPacketSize = ep->wMaxPacketSize;
        }
        break;
      }
#endif

      if (currentInterfaceClass_ != USB_CLASS_HID_VALUE || !isIn ||
          xferType != USB_BM_ATTRIBUTES_XFER_INT)
        break;

      EndpointState* endpoint = allocateEndpoint(*device);
      if (!endpoint)
      {
        setLastError(ESP_ERR_NO_MEM);
        break;
      }

      esp_err_t err = usb_host_transfer_alloc(ep->wMaxPacketSize, 0, &endpoint->transfer);
      if (err != ESP_OK)
      {
        endpoint->inUse = false;
        setLastError(err);
        break;
      }

      endpoint->address = ep->bEndpointAddress;
      endpoint->interfaceNumber = currentInterfaceNumber_;
      endpoint->interfaceClass = currentInterfaceClass_;
      endpoint->interfaceSubClass = currentInterfaceSubClass_;
      endpoint->interfaceProtocol = currentInterfaceProtocol_;
      endpoint->transfer->device_handle = device->handle;
      endpoint->transfer->bEndpointAddress = ep->bEndpointAddress;
      endpoint->transfer->callback = transferCallback;
      endpoint->transfer->context = this;
      endpoint->transfer->num_bytes = ep->wMaxPacketSize;

      submitInputTransfer(*endpoint);
      break;
    }

    default:
      break;
  }
}

void EspUsbHost::transferCallback(usb_transfer_t* transfer)
{
  static_cast<EspUsbHost*>(transfer->context)->handleTransfer(transfer);
}

bool EspUsbHost::submitInputTransfer(EndpointState& endpoint)
{
  if (!endpoint.transfer || endpoint.transferSubmitted)
    return endpoint.transferSubmitted;

  esp_err_t err = usb_host_transfer_submit(endpoint.transfer);
  if (err != ESP_OK)
  {
    setLastError(err);
    return false;
  }
  endpoint.transferSubmitted = true;
  return true;
}

bool EspUsbHost::submitSetInterface(DeviceState& device,
                                    uint8_t interfaceNumber,
                                    uint8_t alternateSetting)
{
  if (!clientHandle_ || !device.handle)
    return false;

  usb_transfer_t* transfer = nullptr;
  esp_err_t err = usb_host_transfer_alloc(USB_SETUP_PACKET_SIZE, 0, &transfer);
  if (err != ESP_OK)
  {
    setLastError(err);
    return false;
  }

  usb_setup_packet_t* setup = reinterpret_cast<usb_setup_packet_t*>(transfer->data_buffer);
  USB_SETUP_PACKET_INIT_SET_INTERFACE(setup, interfaceNumber, alternateSetting);
  transfer->device_handle = device.handle;
  transfer->bEndpointAddress = 0;
  transfer->callback = controlTransferCallback;
  transfer->context = this;
  transfer->num_bytes = USB_SETUP_PACKET_SIZE;

  err = usb_host_transfer_submit_control(clientHandle_, transfer);
  if (err != ESP_OK)
  {
    setLastError(err);
    usb_host_transfer_free(transfer);
    return false;
  }
  return true;
}

void EspUsbHost::submitPendingTransfers(usb_device_handle_t deviceHandle,
                                        uint8_t interfaceNumber)
{
  for (EndpointState& endpoint : endpoints_)
  {
    if (!endpoint.inUse || endpoint.deviceHandle != deviceHandle ||
        endpoint.interfaceNumber != interfaceNumber || endpoint.transferSubmitted)
      continue;
    submitInputTransfer(endpoint);
  }
}

void EspUsbHost::controlTransferCallback(usb_transfer_t* transfer)
{
  EspUsbHost* host = static_cast<EspUsbHost*>(transfer->context);
  usb_device_handle_t deviceHandle = transfer->device_handle;
  uint8_t setInterfaceNumber = 0;
  bool isSetInterface = false;
  bool isLedSetReport = false;

  if (transfer->data_buffer && transfer->num_bytes >= USB_SETUP_PACKET_SIZE)
  {
    const usb_setup_packet_t* setup =
        reinterpret_cast<const usb_setup_packet_t*>(transfer->data_buffer);
    isSetInterface = (setup->bRequest == USB_REQUEST_SET_INTERFACE &&
                      setup->bmRequestType == USB_STANDARD_INTERFACE_REQUEST_TYPE);
    setInterfaceNumber = setup->wIndex & 0xff;
    isLedSetReport = (setup->bmRequestType == HID_SET_REPORT_REQUEST_TYPE &&
                      setup->bRequest == HID_CLASS_REQUEST_SET_REPORT &&
                      (setup->wValue >> 8) == ESP_USB_HOST_HID_REPORT_TYPE_OUTPUT);
  }

  if (transfer->status == USB_TRANSFER_STATUS_COMPLETED)
  {
    if (host && isSetInterface)
      host->submitPendingTransfers(deviceHandle, setInterfaceNumber);
  }
  else if (host)
  {
    host->setLastError(ESP_FAIL);
  }

  if (host && isLedSetReport)
  {
    DeviceState* device = host->findDeviceByHandle(deviceHandle);
    if (device && device->hasKeyboardInterface)
    {
      device->keyboardLedPending = false;
      if (transfer->status == USB_TRANSFER_STATUS_COMPLETED)
      {
        const uint8_t currentLeds = espUsbHostBuildKeyboardLedReport(
            device->keyboardNumLock, device->keyboardCapsLock, device->keyboardScrollLock);
        if (currentLeds != device->keyboardLedLastSent)
          device->keyboardLedDirty = true;
      }
    }
  }

  usb_host_transfer_free(transfer);
}

void EspUsbHost::handleTransfer(usb_transfer_t* transfer)
{
  EndpointState* endpoint = findEndpoint(transfer->device_handle, transfer->bEndpointAddress);
  if (!endpoint)
    return;

  DeviceState* device = findDeviceByHandle(endpoint->deviceHandle);

  if (transfer->status == USB_TRANSFER_STATUS_COMPLETED && transfer->actual_num_bytes > 0)
  {
    if (endpoint->interfaceClass == USB_CLASS_HID_VALUE)
    {
      if (endpoint->interfaceSubClass == HID_SUBCLASS_BOOT_VALUE &&
          endpoint->interfaceProtocol == HID_PROTOCOL_KEYBOARD_VALUE)
      {
        handleKeyboard(*endpoint, transfer->data_buffer, transfer->actual_num_bytes,
                       transfer->data_buffer, transfer->actual_num_bytes);
      }
      else if (endpoint->interfaceSubClass == HID_SUBCLASS_BOOT_VALUE &&
               endpoint->interfaceProtocol == HID_PROTOCOL_MOUSE_VALUE)
      {
        handleMouse(*endpoint, transfer->data_buffer, transfer->actual_num_bytes);
      }
      else if (transfer->actual_num_bytes >= 5 &&
               transfer->data_buffer[0] == ESP_USB_HOST_HID_REPORT_ID_MOUSE)
      {
        handleMouse(*endpoint, transfer->data_buffer, transfer->actual_num_bytes);
      }
      else if (transfer->actual_num_bytes >= 9 &&
               transfer->data_buffer[0] == ESP_USB_HOST_HID_REPORT_ID_KEYBOARD)
      {
        handleKeyboard(*endpoint, transfer->data_buffer + 1, transfer->actual_num_bytes - 1,
                       transfer->data_buffer, transfer->actual_num_bytes);
      }
    }
  }

  if (transfer->status == USB_TRANSFER_STATUS_NO_DEVICE ||
      transfer->status == USB_TRANSFER_STATUS_CANCELED)
    return;

  if (running_ && endpoint->deviceHandle)
  {
    const bool isKeyboardEndpoint = device && device->hasKeyboardInterface &&
        endpoint->interfaceNumber == device->keyboardInterfaceNumber;
    if (isKeyboardEndpoint && device->keyboardLedDirty)
    {
      endpoint->resubmitAfterLed = true;
    }
    else
    {
      esp_err_t err = usb_host_transfer_submit(transfer);
      if (err != ESP_OK && err != ESP_ERR_INVALID_STATE && err != ESP_ERR_NOT_FINISHED)
        setLastError(err);
    }
  }
}

void EspUsbHost::handleKeyboard(EndpointState& endpoint,
                                const uint8_t* data,
                                size_t length,
                                const uint8_t* rawData,
                                size_t rawLength)
{
  if (length < ESP_USB_HOST_BOOT_KEYBOARD_REPORT_SIZE)
    return;
  if (!espUsbHostIsBootKeyboardReportValid(data, length))
    return;

  DeviceState* device = findDeviceByHandle(endpoint.deviceHandle);

  EspUsbHostKeyboardReport previousReport;
  EspUsbHostKeyboardReport currentReport;
  memcpy(previousReport.data, endpoint.lastKeyboardReport, ESP_USB_HOST_BOOT_KEYBOARD_REPORT_SIZE);
  memcpy(currentReport.data, data, ESP_USB_HOST_BOOT_KEYBOARD_REPORT_SIZE);

  if (device)
  {
    const uint8_t* keys = currentReport.data + 2;
    const uint8_t* lastKeys = previousReport.data + 2;
    bool ledChanged = false;
    for (int i = 0; i < 6; i++)
    {
      const uint8_t key = keys[i];
      if (key == 0)
        continue;
      bool existed = false;
      for (int j = 0; j < 6; j++)
      {
        if (lastKeys[j] == key)
        {
          existed = true;
          break;
        }
      }
      if (!existed)
      {
        if (key == HID_KEY_NUM_LOCK)
        {
          device->keyboardNumLock = !device->keyboardNumLock;
          ledChanged = true;
        }
        else if (key == HID_KEY_CAPS_LOCK)
        {
          device->keyboardCapsLock = !device->keyboardCapsLock;
          ledChanged = true;
        }
        else if (key == HID_KEY_SCROLL_LOCK)
        {
          device->keyboardScrollLock = !device->keyboardScrollLock;
          ledChanged = true;
        }
      }
    }
    if (ledChanged)
    {
      device->keyboardLedDirty = true;
      device->keyboardLedDirtyTimeMs = millis();
    }
  }

  const bool capsLock = device ? device->keyboardCapsLock : false;
  const bool numLock = device ? device->keyboardNumLock : false;
  const bool scrollLock = device ? device->keyboardScrollLock : false;

  EspUsbHostKeyboardEvent events[ESP_USB_HOST_BOOT_KEYBOARD_MAX_EVENTS];
  const size_t eventCount = espUsbHostBuildKeyboardEvents(
      endpoint.interfaceNumber, previousReport, currentReport, keyboardLayout_, capsLock, numLock,
      events, ESP_USB_HOST_BOOT_KEYBOARD_MAX_EVENTS);

  endpoint.keyboardReportReady = true;

  for (size_t i = 0; i < eventCount; i++)
  {
    events[i].address = endpoint.deviceAddress;
    if (device)
    {
      events[i].vid = device->info.vid;
      events[i].pid = device->info.pid;
      events[i].manufacturer = device->info.manufacturer;
      events[i].product = device->info.product;
      events[i].serial = device->info.serial;
    }
    events[i].numLock = numLock;
    events[i].capsLock = capsLock;
    events[i].scrollLock = scrollLock;
    events[i].rawData = rawData;
    events[i].rawLength = rawLength;
    events[i].reportData = data;
    events[i].reportLength = length;
    if (keyboardCallback_)
      keyboardCallback_(events[i], keyboardCallbackArg_);
  }

  memcpy(endpoint.lastKeyboardReport, data, ESP_USB_HOST_BOOT_KEYBOARD_REPORT_SIZE);
}

void EspUsbHost::handleMouse(EndpointState& endpoint, const uint8_t* data, size_t length)
{
  if (!mouseCallback_)
    return;

  const uint8_t* rawData = data;
  const size_t rawLength = length;

  if (endpoint.interfaceProtocol != HID_PROTOCOL_MOUSE_VALUE && length >= 5 &&
      data[0] == ESP_USB_HOST_HID_REPORT_ID_MOUSE)
  {
    data += 1;
    length -= 1;
  }

  EspUsbHostMouseEvent event;
  if (!espUsbHostParseBootMouseReport(endpoint.interfaceNumber, data, length,
                                      endpoint.lastMouseButtons, event))
    return;

  event.address = endpoint.deviceAddress;
  DeviceState* device = findDeviceByHandle(endpoint.deviceHandle);
  if (device)
  {
    event.vid = device->info.vid;
    event.pid = device->info.pid;
    event.manufacturer = device->info.manufacturer;
    event.product = device->info.product;
    event.serial = device->info.serial;
  }
  event.rawData = rawData;
  event.rawLength = rawLength;
  event.reportData = data;
  event.reportLength = length;

  mouseCallback_(event, mouseCallbackArg_);
  endpoint.lastMouseButtons = event.buttons;
}

EspUsbHost::EndpointState* EspUsbHost::findEndpoint(usb_device_handle_t deviceHandle,
                                                    uint8_t endpointAddress)
{
  for (EndpointState& endpoint : endpoints_)
  {
    if (endpoint.inUse && endpoint.deviceHandle == deviceHandle &&
        endpoint.address == endpointAddress)
      return &endpoint;
  }
  return nullptr;
}

EspUsbHost::EndpointState* EspUsbHost::allocateEndpoint(DeviceState& device)
{
  for (EndpointState& endpoint : endpoints_)
  {
    if (!endpoint.inUse)
    {
      endpoint = EndpointState();
      endpoint.inUse = true;
      endpoint.deviceIndex = static_cast<uint8_t>(&device - devices_);
      endpoint.deviceAddress = device.info.address;
      endpoint.deviceHandle = device.handle;
      return &endpoint;
    }
  }
  return nullptr;
}

EspUsbHost::DeviceState* EspUsbHost::allocateDevice()
{
  for (DeviceState& device : devices_)
  {
    if (!device.inUse)
    {
      device = DeviceState();
      return &device;
    }
  }
  return nullptr;
}

EspUsbHost::DeviceState* EspUsbHost::findDevice(uint8_t address)
{
  return const_cast<DeviceState*>(static_cast<const EspUsbHost*>(this)->findDevice(address));
}

const EspUsbHost::DeviceState* EspUsbHost::findDevice(uint8_t address) const
{
  for (const DeviceState& device : devices_)
  {
    if (!device.inUse)
      continue;
    if (address == ESP_USB_HOST_ANY_ADDRESS || device.info.address == address)
      return &device;
  }
  return nullptr;
}

EspUsbHost::DeviceState* EspUsbHost::findDeviceByHandle(usb_device_handle_t handle)
{
  for (DeviceState& device : devices_)
  {
    if (device.inUse && device.handle == handle)
      return &device;
  }
  return nullptr;
}

EspUsbHost::DeviceState* EspUsbHost::findKeyboardDevice(uint8_t address)
{
  return const_cast<DeviceState*>(
      static_cast<const EspUsbHost*>(this)->findKeyboardDevice(address));
}

const EspUsbHost::DeviceState* EspUsbHost::findKeyboardDevice(uint8_t address) const
{
  for (const DeviceState& device : devices_)
  {
    if (!device.inUse || !device.handle || !device.hasKeyboardInterface)
      continue;
    if (address == ESP_USB_HOST_ANY_ADDRESS || device.info.address == address)
      return &device;
  }
  return nullptr;
}

void EspUsbHost::releaseEndpoints(DeviceState& device, bool clearEndpoints)
{
  for (EndpointState& endpoint : endpoints_)
  {
    if (!endpoint.inUse || endpoint.deviceHandle != device.handle)
      continue;
    if (endpoint.transfer)
    {
      if (clearEndpoints && device.handle)
        usb_host_endpoint_clear(device.handle, endpoint.address);
      usb_host_transfer_free(endpoint.transfer);
    }
    endpoint = EndpointState();
  }
}

void EspUsbHost::releaseAllEndpoints(bool clearEndpoints)
{
  for (DeviceState& device : devices_)
  {
    if (device.inUse)
      releaseEndpoints(device, clearEndpoints);
  }
}

void EspUsbHost::releaseInterfaces(DeviceState& device)
{
  for (uint8_t i = 0; i < device.interfaceCount; i++)
  {
    usb_host_interface_release(clientHandle_, device.handle, device.interfaces[i]);
    device.interfaces[i] = 0;
  }
  device.interfaceCount = 0;
}

#if ESP_USB_HOST_ENABLE_MSC

EspUsbHost::DeviceState* EspUsbHost::findMscDevice(uint8_t address)
{
  return const_cast<DeviceState*>(static_cast<const EspUsbHost*>(this)->findMscDevice(address));
}

const EspUsbHost::DeviceState* EspUsbHost::findMscDevice(uint8_t address) const
{
  for (const DeviceState& device : devices_)
  {
    if (!device.inUse || !device.handle || !device.hasMscInterface || !device.hasMscInEndpoint ||
        !device.hasMscOutEndpoint)
      continue;
    if (address == ESP_USB_HOST_ANY_ADDRESS || device.info.address == address)
      return &device;
  }
  return nullptr;
}

bool EspUsbHost::mscReady(uint8_t address) const
{
  return findMscDevice(address) != nullptr;
}

bool EspUsbHost::mscLastSense(EspUsbHostMscSense& sense, uint8_t address) const
{
  const DeviceState* device = findMscDevice(address);
  if (!device || !device->hasMscLastSense)
    return false;
  sense = device->mscLastSense;
  return true;
}

bool EspUsbHost::mscResetRecovery(DeviceState& device, uint32_t timeoutMs)
{
  if (!device.handle || xTaskGetCurrentTaskHandle() == clientTaskHandle_)
    return false;

  EspUsbHostSyncTransferContext context;
  context.done = xSemaphoreCreateBinary();
  if (!context.done)
  {
    setLastError(ESP_ERR_NO_MEM);
    return false;
  }

  usb_transfer_t* transfer = nullptr;
  esp_err_t err = usb_host_transfer_alloc(USB_SETUP_PACKET_SIZE, 0, &transfer);
  if (err != ESP_OK)
  {
    setLastError(err);
    vSemaphoreDelete(context.done);
    return false;
  }

  usb_setup_packet_t* setup = reinterpret_cast<usb_setup_packet_t*>(transfer->data_buffer);
  setup->bmRequestType = USB_MSC_RESET_REQUEST_TYPE;
  setup->bRequest = USB_MSC_RESET_REQUEST;
  setup->wValue = 0;
  setup->wIndex = device.mscInterfaceNumber;
  setup->wLength = 0;

  transfer->device_handle = device.handle;
  transfer->bEndpointAddress = 0;
  transfer->callback = syncTransferCallback;
  transfer->context = &context;
  transfer->num_bytes = USB_SETUP_PACKET_SIZE;

  err = usb_host_transfer_submit_control(clientHandle_, transfer);
  bool ok = false;
  if (err == ESP_OK && xSemaphoreTake(context.done, pdMS_TO_TICKS(timeoutMs)) == pdTRUE)
    ok = (context.status == USB_TRANSFER_STATUS_COMPLETED);

  usb_host_transfer_free(transfer);
  vSemaphoreDelete(context.done);
  usb_host_endpoint_clear(device.handle, device.mscInEndpointAddress);
  usb_host_endpoint_clear(device.handle, device.mscOutEndpointAddress);
  return ok;
}

bool EspUsbHost::mscCommand(DeviceState& device,
                            const uint8_t* command,
                            uint8_t commandLength,
                            uint8_t* data,
                            size_t dataLength,
                            bool dataIn,
                            uint32_t timeoutMs)
{
  if (!device.handle || !command || commandLength == 0 || commandLength > 16)
    return false;
  if (xTaskGetCurrentTaskHandle() == clientTaskHandle_)
    return false;

  EspUsbHostSyncTransferContext context;
  context.done = xSemaphoreCreateBinary();
  if (!context.done)
  {
    setLastError(ESP_ERR_NO_MEM);
    return false;
  }

  auto submitAndWait = [&](uint8_t endpointAddress, const uint8_t* out, uint8_t* in,
                           size_t length, size_t& actual) -> bool
  {
    size_t transferLength = length;
    if ((endpointAddress & USB_B_ENDPOINT_ADDRESS_EP_DIR_MASK) != 0 && device.mscInPacketSize > 0)
    {
      transferLength =
          ((length + device.mscInPacketSize - 1) / device.mscInPacketSize) * device.mscInPacketSize;
    }
    const size_t allocLength = transferLength > 0 ? transferLength : 1;
    usb_transfer_t* transfer = nullptr;
    esp_err_t err = usb_host_transfer_alloc(allocLength, 0, &transfer);
    if (err != ESP_OK)
    {
      setLastError(err);
      return false;
    }
    if (out && length > 0)
      memcpy(transfer->data_buffer, out, length);
    context.status = USB_TRANSFER_STATUS_ERROR;
    context.actualLength = 0;
    transfer->device_handle = device.handle;
    transfer->bEndpointAddress = endpointAddress;
    transfer->callback = syncTransferCallback;
    transfer->context = &context;
    transfer->num_bytes = transferLength;

    err = usb_host_transfer_submit(transfer);
    if (err != ESP_OK)
    {
      setLastError(err);
      usb_host_transfer_free(transfer);
      return false;
    }

    if (xSemaphoreTake(context.done, pdMS_TO_TICKS(timeoutMs)) != pdTRUE)
    {
      usb_host_endpoint_clear(device.handle, endpointAddress);
      usb_host_transfer_free(transfer);
      setLastError(ESP_ERR_TIMEOUT);
      return false;
    }

    actual = context.actualLength;
    const bool ok = (context.status == USB_TRANSFER_STATUS_COMPLETED);
    if (ok && in && length > 0)
      memcpy(in, transfer->data_buffer, actual < length ? actual : length);
    usb_host_transfer_free(transfer);
    if (!ok)
      setLastError(ESP_FAIL);
    return ok;
  };

  const uint8_t commandOpcode = command[0];
  const bool allowResetRecovery = commandOpcode != SCSI_CMD_SYNCHRONIZE_CACHE_10;
  bool requestSenseAfterCommand = false;
  const uint32_t tag = device.mscTag++;

  EspUsbHostMscCbw cbw = {};
  cbw.signature = USB_MSC_CBW_SIGNATURE;
  cbw.tag = tag;
  cbw.dataTransferLength = static_cast<uint32_t>(dataLength);
  cbw.flags = dataIn ? 0x80 : 0x00;
  cbw.lun = device.mscLun;
  cbw.commandBlockLength = commandLength;
  memcpy(cbw.commandBlock, command, commandLength);

  size_t actual = 0;
  bool ok = submitAndWait(device.mscOutEndpointAddress, reinterpret_cast<const uint8_t*>(&cbw),
                          nullptr, sizeof(cbw), actual);
  if (ok && dataLength > 0)
  {
    if (dataIn)
      ok = submitAndWait(device.mscInEndpointAddress, nullptr, data, dataLength, actual);
    else
      ok = submitAndWait(device.mscOutEndpointAddress, data, nullptr, dataLength, actual);

    if (!ok)
    {
      usb_host_endpoint_clear(device.handle,
                              dataIn ? device.mscInEndpointAddress : device.mscOutEndpointAddress);
    }
  }

  EspUsbHostMscCsw csw = {};
  bool cswOk = submitAndWait(device.mscInEndpointAddress, nullptr,
                             reinterpret_cast<uint8_t*>(&csw), sizeof(csw), actual);
  if (!cswOk)
  {
    usb_host_endpoint_clear(device.handle, device.mscInEndpointAddress);
    cswOk = submitAndWait(device.mscInEndpointAddress, nullptr, reinterpret_cast<uint8_t*>(&csw),
                          sizeof(csw), actual);
  }

  if (!cswOk || actual < sizeof(csw) || csw.signature != USB_MSC_CSW_SIGNATURE || csw.tag != tag ||
      csw.status == USB_MSC_CSW_STATUS_PHASE_ERROR)
  {
    ok = false;
    if (allowResetRecovery)
      mscResetRecovery(device, timeoutMs);
  }
  else if (csw.status != USB_MSC_CSW_STATUS_PASSED)
  {
    ok = false;
    requestSenseAfterCommand = (commandOpcode != SCSI_CMD_REQUEST_SENSE);
  }

  vSemaphoreDelete(context.done);
  if (requestSenseAfterCommand)
  {
    EspUsbHostMscSense sense;
    mscRequestSense(sense, device.info.address, timeoutMs);
  }
  return ok;
}

bool EspUsbHost::mscMaxLun(uint8_t& maxLun, uint8_t address, uint32_t timeoutMs)
{
  DeviceState* device = findMscDevice(address);
  if (!device)
    return false;
  if (device->hasMscMaxLun)
  {
    maxLun = device->mscMaxLun;
    return true;
  }
  if (xTaskGetCurrentTaskHandle() == clientTaskHandle_)
    return false;

  EspUsbHostSyncTransferContext context;
  context.done = xSemaphoreCreateBinary();
  if (!context.done)
  {
    setLastError(ESP_ERR_NO_MEM);
    return false;
  }

  usb_transfer_t* transfer = nullptr;
  esp_err_t err = usb_host_transfer_alloc(USB_SETUP_PACKET_SIZE + 1, 0, &transfer);
  if (err != ESP_OK)
  {
    setLastError(err);
    vSemaphoreDelete(context.done);
    return false;
  }

  usb_setup_packet_t* setup = reinterpret_cast<usb_setup_packet_t*>(transfer->data_buffer);
  setup->bmRequestType = USB_MSC_GET_MAX_LUN_REQUEST_TYPE;
  setup->bRequest = USB_MSC_GET_MAX_LUN_REQUEST;
  setup->wValue = 0;
  setup->wIndex = device->mscInterfaceNumber;
  setup->wLength = 1;

  transfer->device_handle = device->handle;
  transfer->bEndpointAddress = 0;
  transfer->callback = syncTransferCallback;
  transfer->context = &context;
  transfer->num_bytes = USB_SETUP_PACKET_SIZE + 1;

  err = usb_host_transfer_submit_control(clientHandle_, transfer);
  if (err == ESP_OK && xSemaphoreTake(context.done, pdMS_TO_TICKS(timeoutMs)) == pdTRUE &&
      context.status == USB_TRANSFER_STATUS_COMPLETED &&
      context.actualLength >= USB_SETUP_PACKET_SIZE + 1)
  {
    device->mscMaxLun = transfer->data_buffer[USB_SETUP_PACKET_SIZE];
  }
  else
  {
    device->mscMaxLun = 0;
  }

  device->hasMscMaxLun = true;
  maxLun = device->mscMaxLun;
  usb_host_transfer_free(transfer);
  vSemaphoreDelete(context.done);
  return true;
}

bool EspUsbHost::mscSelectLun(uint8_t lun, uint8_t address, uint32_t timeoutMs)
{
  DeviceState* device = findMscDevice(address);
  if (!device)
    return false;
  uint8_t maxLun = 0;
  if (!mscMaxLun(maxLun, device->info.address, timeoutMs))
    return false;
  if (lun > maxLun)
    return false;
  if (device->mscLun == lun)
    return true;

  device->mscLun = lun;
  device->mscBlockCount = 0;
  device->mscBlockCount64 = 0;
  device->mscBlockSize = 0;
  device->hasMscLastSense = false;
  return true;
}

bool EspUsbHost::mscGetBlockDeviceInfo(EspUsbHostMscBlockDeviceInfo& info,
                                       uint8_t address,
                                       uint32_t timeoutMs)
{
  DeviceState* device = findMscDevice(address);
  if (!device)
    return false;
  uint8_t maxLun = 0;
  if (!mscMaxLun(maxLun, device->info.address, timeoutMs))
    return false;
  uint64_t blockCount = 0;
  uint32_t blockSize = 0;
  if (!mscCapacity64(blockCount, blockSize, device->info.address, timeoutMs))
    return false;

  info = EspUsbHostMscBlockDeviceInfo();
  info.address = device->info.address;
  info.interfaceNumber = device->mscInterfaceNumber;
  info.lun = device->mscLun;
  info.maxLun = maxLun;
  info.blockCount = blockCount;
  info.blockSize = blockSize;
  info.capacityBytes = blockCount * blockSize;
  return true;
}

bool EspUsbHost::mscInquiry(EspUsbHostMscInquiry& inquiry, uint8_t address, uint32_t timeoutMs)
{
  DeviceState* device = findMscDevice(address);
  if (!device)
    return false;

  uint8_t command[6] = {};
  uint8_t data[36] = {};
  command[0] = SCSI_CMD_INQUIRY;
  command[4] = sizeof(data);
  if (!mscCommand(*device, command, sizeof(command), data, sizeof(data), true, timeoutMs))
    return false;

  inquiry = EspUsbHostMscInquiry();
  inquiry.peripheralDeviceType = data[0] & 0x1f;
  inquiry.removable = (data[1] & 0x80) != 0;
  memcpy(inquiry.vendor, data + 8, 8);
  memcpy(inquiry.product, data + 16, 16);
  memcpy(inquiry.revision, data + 32, 4);
  inquiry.vendor[8] = '\0';
  inquiry.product[16] = '\0';
  inquiry.revision[4] = '\0';
  return true;
}

bool EspUsbHost::mscRequestSense(EspUsbHostMscSense& sense, uint8_t address, uint32_t timeoutMs)
{
  DeviceState* device = findMscDevice(address);
  if (!device)
    return false;

  uint8_t command[6] = {};
  uint8_t data[18] = {};
  command[0] = SCSI_CMD_REQUEST_SENSE;
  command[4] = sizeof(data);
  if (!mscCommand(*device, command, sizeof(command), data, sizeof(data), true, timeoutMs))
    return false;

  sense = EspUsbHostMscSense();
  sense.responseCode = data[0] & 0x7f;
  sense.senseKey = data[2] & 0x0f;
  sense.additionalSenseCode = data[12];
  sense.additionalSenseQualifier = data[13];
  device->mscLastSense = sense;
  device->hasMscLastSense = true;
  return true;
}

bool EspUsbHost::mscTestUnitReady(uint8_t address, uint32_t timeoutMs)
{
  DeviceState* device = findMscDevice(address);
  if (!device)
    return false;
  uint8_t command[6] = {};
  command[0] = SCSI_CMD_TEST_UNIT_READY;
  return mscCommand(*device, command, sizeof(command), nullptr, 0, true, timeoutMs);
}

bool EspUsbHost::mscWaitReady(uint8_t address, uint32_t readyTimeoutMs, uint32_t commandTimeoutMs)
{
  if (xTaskGetCurrentTaskHandle() == clientTaskHandle_)
    return false;
  const uint32_t started = millis();
  while (true)
  {
    DeviceState* device = findMscDevice(address);
    if (device && mscTestUnitReady(device->info.address, commandTimeoutMs))
      return true;
    if (device)
    {
      EspUsbHostMscSense sense;
      mscRequestSense(sense, device->info.address, commandTimeoutMs);
    }
    if (millis() - started >= readyTimeoutMs)
      return false;
    delay(50);
  }
}

bool EspUsbHost::mscCapacity64(uint64_t& blockCount,
                               uint32_t& blockSize,
                               uint8_t address,
                               uint32_t timeoutMs)
{
  DeviceState* device = findMscDevice(address);
  if (!device)
    return false;

  uint8_t cmd10[10] = {};
  uint8_t data10[8] = {};
  cmd10[0] = SCSI_CMD_READ_CAPACITY_10;
  if (!mscCommand(*device, cmd10, sizeof(cmd10), data10, sizeof(data10), true, timeoutMs))
    return false;

  const uint32_t lastLba32 = readBe32(data10);
  blockSize = readBe32(data10 + 4);
  if (lastLba32 == 0xffffffffU)
  {
    uint8_t cmd16[16] = {};
    uint8_t data16[32] = {};
    cmd16[0] = SCSI_CMD_SERVICE_ACTION_IN_16;
    cmd16[1] = SCSI_SERVICE_ACTION_READ_CAPACITY_16;
    writeBe32(cmd16 + 10, sizeof(data16));
    if (!mscCommand(*device, cmd16, sizeof(cmd16), data16, sizeof(data16), true, timeoutMs))
      return false;
    blockCount = readBe64(data16) + 1ULL;
    blockSize = readBe32(data16 + 8);
  }
  else
  {
    blockCount = static_cast<uint64_t>(lastLba32) + 1ULL;
  }

  device->mscBlockCount64 = blockCount;
  device->mscBlockCount = blockCount <= 0xffffffffULL ? static_cast<uint32_t>(blockCount) : 0;
  device->mscBlockSize = blockSize;
  return blockSize > 0;
}

bool EspUsbHost::mscCapacity(uint32_t& blockCount,
                             uint32_t& blockSize,
                             uint8_t address,
                             uint32_t timeoutMs)
{
  uint64_t blockCount64 = 0;
  if (!mscCapacity64(blockCount64, blockSize, address, timeoutMs))
    return false;
  if (blockCount64 > 0xffffffffULL)
    return false;
  blockCount = static_cast<uint32_t>(blockCount64);
  return true;
}

bool EspUsbHost::mscReadBlocks(uint32_t lba,
                               uint8_t* data,
                               uint32_t blockCount,
                               uint8_t address,
                               uint32_t timeoutMs)
{
  return mscReadBlocks64(lba, data, blockCount, address, timeoutMs);
}

bool EspUsbHost::mscWriteBlocks(uint32_t lba,
                                const uint8_t* data,
                                uint32_t blockCount,
                                uint8_t address,
                                uint32_t timeoutMs)
{
  return mscWriteBlocks64(lba, data, blockCount, address, timeoutMs);
}

bool EspUsbHost::mscReadBlocks64(uint64_t lba,
                                 uint8_t* data,
                                 uint32_t blockCount,
                                 uint8_t address,
                                 uint32_t timeoutMs)
{
  DeviceState* device = findMscDevice(address);
  if (!device || !data || blockCount == 0)
    return false;
  if (device->mscBlockSize == 0)
  {
    uint64_t capBlocks = 0;
    uint32_t blkSize = 0;
    if (!mscCapacity64(capBlocks, blkSize, device->info.address, timeoutMs))
      return false;
  }
  if (device->mscBlockCount64 > 0 &&
      (lba >= device->mscBlockCount64 || blockCount > device->mscBlockCount64 - lba))
    return false;

  const uint32_t maxBlocksPerChunk =
      device->mscBlockSize >= USB_MSC_MAX_TRANSFER_BYTES ? 1 : static_cast<uint32_t>(USB_MSC_MAX_TRANSFER_BYTES / device->mscBlockSize);

  uint32_t remaining = blockCount;
  uint64_t currentLba = lba;
  uint8_t* ptr = data;

  while (remaining > 0)
  {
    const uint32_t chunk = remaining < maxBlocksPerChunk ? remaining : maxBlocksPerChunk;
    const size_t bytes = static_cast<size_t>(chunk) * device->mscBlockSize;
    bool ok = false;
    if (currentLba + chunk - 1 <= 0xffffffffULL && chunk <= 0xffffU)
    {
      uint8_t cmd[10] = {};
      cmd[0] = SCSI_CMD_READ_10;
      writeBe32(cmd + 2, static_cast<uint32_t>(currentLba));
      cmd[7] = static_cast<uint8_t>((chunk >> 8) & 0xff);
      cmd[8] = static_cast<uint8_t>(chunk & 0xff);
      ok = mscCommand(*device, cmd, sizeof(cmd), ptr, bytes, true, timeoutMs);
    }
    else
    {
      uint8_t cmd[16] = {};
      cmd[0] = SCSI_CMD_READ_16;
      writeBe64(cmd + 2, currentLba);
      writeBe32(cmd + 10, chunk);
      ok = mscCommand(*device, cmd, sizeof(cmd), ptr, bytes, true, timeoutMs);
    }
    if (!ok)
      return false;
    remaining -= chunk;
    currentLba += chunk;
    ptr += bytes;
  }
  return true;
}

bool EspUsbHost::mscWriteBlocks64(uint64_t lba,
                                  const uint8_t* data,
                                  uint32_t blockCount,
                                  uint8_t address,
                                  uint32_t timeoutMs)
{
  DeviceState* device = findMscDevice(address);
  if (!device || !data || blockCount == 0)
    return false;
  if (device->mscBlockSize == 0)
  {
    uint64_t capBlocks = 0;
    uint32_t blkSize = 0;
    if (!mscCapacity64(capBlocks, blkSize, device->info.address, timeoutMs))
      return false;
  }
  if (device->mscBlockCount64 > 0 &&
      (lba >= device->mscBlockCount64 || blockCount > device->mscBlockCount64 - lba))
    return false;

  const uint32_t maxBlocksPerChunk =
      device->mscBlockSize >= USB_MSC_MAX_TRANSFER_BYTES ? 1 : static_cast<uint32_t>(USB_MSC_MAX_TRANSFER_BYTES / device->mscBlockSize);

  uint32_t remaining = blockCount;
  uint64_t currentLba = lba;
  const uint8_t* ptr = data;

  while (remaining > 0)
  {
    const uint32_t chunk = remaining < maxBlocksPerChunk ? remaining : maxBlocksPerChunk;
    const size_t bytes = static_cast<size_t>(chunk) * device->mscBlockSize;
    bool ok = false;
    if (currentLba + chunk - 1 <= 0xffffffffULL && chunk <= 0xffffU)
    {
      uint8_t cmd[10] = {};
      cmd[0] = SCSI_CMD_WRITE_10;
      writeBe32(cmd + 2, static_cast<uint32_t>(currentLba));
      cmd[7] = static_cast<uint8_t>((chunk >> 8) & 0xff);
      cmd[8] = static_cast<uint8_t>(chunk & 0xff);
      ok = mscCommand(*device, cmd, sizeof(cmd), const_cast<uint8_t*>(ptr), bytes, false,
                      timeoutMs);
    }
    else
    {
      uint8_t cmd[16] = {};
      cmd[0] = SCSI_CMD_WRITE_16;
      writeBe64(cmd + 2, currentLba);
      writeBe32(cmd + 10, chunk);
      ok = mscCommand(*device, cmd, sizeof(cmd), const_cast<uint8_t*>(ptr), bytes, false,
                      timeoutMs);
    }
    if (!ok)
      return false;
    remaining -= chunk;
    currentLba += chunk;
    ptr += bytes;
  }
  return true;
}

bool EspUsbHost::mscSynchronizeCache(uint8_t address, uint32_t timeoutMs)
{
  DeviceState* device = findMscDevice(address);
  if (!device)
    return false;
  uint8_t command[10] = {};
  command[0] = SCSI_CMD_SYNCHRONIZE_CACHE_10;
  return mscCommand(*device, command, sizeof(command), nullptr, 0, false, timeoutMs);
}

bool EspUsbHost::mscMount(const char* basePath,
                          uint8_t address,
                          uint8_t lun,
                          uint8_t maxFiles,
                          uint32_t timeoutMs,
                          bool skipSyncCache)
{
  if (!basePath || basePath[0] != '/' || strlen(basePath) >= sizeof(mscFatMounts[0].basePath))
    return false;
  if (findMscFatMountByPath(basePath))
    return false;
  if (!mscWaitReady(address, timeoutMs, timeoutMs))
    return false;

  DeviceState* device = findMscDevice(address);
  if (!device || !mscSelectLun(lun, device->info.address, timeoutMs))
    return false;

  EspUsbHostMscBlockDeviceInfo blockInfo;
  if (!mscGetBlockDeviceInfo(blockInfo, device->info.address, timeoutMs))
    return false;
  if (blockInfo.blockCount == 0 || blockInfo.blockSize == 0 || blockInfo.blockCount > 0xffffffffULL)
    return false;

  EspUsbHostMscFatMount* mount = nullptr;
  for (EspUsbHostMscFatMount& candidate : mscFatMounts)
  {
    if (!candidate.inUse)
    {
      mount = &candidate;
      break;
    }
  }
  if (!mount)
    return false;

  BYTE pdrv = FF_DRV_NOT_USED;
  esp_err_t err = ff_diskio_get_drive(&pdrv);
  if (err != ESP_OK)
  {
    setLastError(err);
    return false;
  }

  *mount = EspUsbHostMscFatMount();
  mount->inUse = true;
  mount->host = this;
  mount->address = device->info.address;
  mount->lun = lun;
  mount->pdrv = pdrv;
  mount->blockCount = blockInfo.blockCount;
  mount->blockSize = blockInfo.blockSize;
  mount->skipSyncCache = skipSyncCache;
  strncpy(mount->basePath, basePath, sizeof(mount->basePath) - 1);
  snprintf(mount->fatDrive, sizeof(mount->fatDrive), "%u:", static_cast<unsigned>(pdrv));

  ff_diskio_register(pdrv, &MSC_FAT_DISKIO);

  const esp_vfs_fat_conf_t conf = {
      .base_path = mount->basePath,
      .fat_drive = mount->fatDrive,
      .max_files = maxFiles,
  };
  err = esp_vfs_fat_register_cfg(&conf, &mount->fs);
  if (err != ESP_OK)
  {
    ff_diskio_unregister(pdrv);
    *mount = EspUsbHostMscFatMount();
    setLastError(err);
    return false;
  }

  const FRESULT mountResult = f_mount(mount->fs, mount->fatDrive, 1);
  if (mountResult != FR_OK)
  {
    esp_vfs_fat_unregister_path(mount->basePath);
    ff_diskio_unregister(pdrv);
    *mount = EspUsbHostMscFatMount();
    setLastError(ESP_FAIL);
    return false;
  }
  return true;
}

bool EspUsbHost::mscUnmount(const char* basePath)
{
  EspUsbHostMscFatMount* mount = findMscFatMountByPath(basePath);
  if (!mount)
    return false;

  if (!mount->skipSyncCache)
    mscSynchronizeCache(mount->address);
  f_mount(nullptr, mount->fatDrive, 0);
  esp_err_t err = esp_vfs_fat_unregister_path(mount->basePath);
  ff_diskio_unregister(mount->pdrv);
  *mount = EspUsbHostMscFatMount();
  if (err != ESP_OK)
  {
    setLastError(err);
    return false;
  }
  return true;
}

bool EspUsbHost::mscMounted(const char* basePath) const
{
  EspUsbHostMscFatMount* mount = findMscFatMountByPath(basePath);
  return mount && mount->host == this;
}

void EspUsbHost::mscUnmountAddress(uint8_t address)
{
  for (EspUsbHostMscFatMount& mount : mscFatMounts)
  {
    if (!mount.inUse || mount.host != this || mount.address != address)
      continue;
    f_mount(nullptr, mount.fatDrive, 0);
    esp_err_t err = esp_vfs_fat_unregister_path(mount.basePath);
    ff_diskio_unregister(mount.pdrv);
    if (err != ESP_OK)
      setLastError(err);
    mount = EspUsbHostMscFatMount();
  }
}

EspUsbHostMscFS::EspUsbHostMscFS() : fs::FS(fs::FSImplPtr(new VFSImpl())) {}

EspUsbHostMscFS::~EspUsbHostMscFS()
{
  end();
}

bool EspUsbHostMscFS::begin(EspUsbHost& host,
                            const char* basePath,
                            uint8_t address,
                            uint8_t lun,
                            uint8_t maxFiles,
                            uint32_t timeoutMs,
                            bool skipSyncCache)
{
  if (!basePath || basePath[0] != '/' || strlen(basePath) >= sizeof(basePath_))
    return false;
  if (host_)
  {
    if (strcmp(basePath_, basePath) == 0 && host_->mscMounted(basePath_))
      return true;
    _impl->mountpoint(nullptr);
    host_ = nullptr;
    basePath_[0] = '\0';
  }
  if (!host.mscMount(basePath, address, lun, maxFiles, timeoutMs, skipSyncCache || skipSyncCache_))
    return false;
  strncpy(basePath_, basePath, sizeof(basePath_) - 1);
  host_ = &host;
  _impl->mountpoint(basePath_);
  return true;
}

void EspUsbHostMscFS::end()
{
  if (!host_)
    return;
  _impl->mountpoint(nullptr);
  host_->mscUnmount(basePath_);
  host_ = nullptr;
  basePath_[0] = '\0';
}

bool EspUsbHostMscFS::mounted() const
{
  return host_ && host_->mscMounted(basePath_);
}

const char* EspUsbHostMscFS::basePath() const
{
  return basePath_;
}

void EspUsbHostMscFS::setSkipSyncCache(bool skip)
{
  skipSyncCache_ = skip;
}

bool EspUsbHostMscFS::skipSyncCache() const
{
  return skipSyncCache_;
}

#endif  // ESP_USB_HOST_ENABLE_MSC

#endif  // #ifndef CONFIG_IDF_TARGET_ESP32
