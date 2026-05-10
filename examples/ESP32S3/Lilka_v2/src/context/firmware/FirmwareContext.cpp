#include "FirmwareContext.h"

#include "../WidgetCreator.h"
#include "Update.h"
#include "manager/FileManager.h"
#include "manager/file/FileStream.h"

const char STR_FIRMWARE_FN[] = "/firmware.bin";
const char STR_UPDATE_FIRMWARE[] = "Оновлення прошивки";

extern "C" bool verifyRollbackLater() {
    return true;
}

FirmwareContext::FirmwareContext()
{
  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  Label* title = new Label(ID_TITLE);
  layout->addWidget(title);
  title->setText(STR_UPDATE_FIRMWARE);
  title->setWidth(UI_WIDTH);
  title->setHeight(30);
  title->setGravity(IWidget::GRAVITY_CENTER);
  title->setAlign(IWidget::ALIGN_CENTER);
  title->setBackColor(COLOR_BLACK);

  Label* author = title->clone(ID_AUTHOR);
  layout->addWidget(author);
  author->setText(STR_AUTHOR);
  author->setPos(0, getCenterY(author));
  author->setBackColor(COLOR_MAIN_BACK);

  Label* version = author->clone(ID_VERSION);
  layout->addWidget(version);
  version->setText(STR_FIRM_VER);
  version->setPos(0, author->getYPos() + author->getHeight() + 10);
}

bool FirmwareContext::loop()
{
  return true;
}

void FirmwareContext::update()
{
  if (_input.isReleased(BtnID::BTN_OK))
  {
    _input.lock(BtnID::BTN_OK, 500);

    if (_fs.fileExist(STR_FIRMWARE_FN))
    {
      size_t firm_size = _fs.getFileSize(STR_FIRMWARE_FN);
      FILE* firm_ptr = _fs.openFile(STR_FIRMWARE_FN, "rb");

      FileStream f_stream(firm_ptr, STR_FIRMWARE_FN, firm_size);

      showUpdating();

      delay(200);

      Update.begin(firm_size, U_FLASH);
      Update.writeStream(f_stream);
      Update.end();

      _fs.closeFile(firm_ptr);
      ESP.restart();
    }
  }
  else if (_input.isReleased(BtnID::BTN_BACK))
  {
    _input.lock(BtnID::BTN_BACK, 500);
    openContextByID(ID_CONTEXT_MENU);
  }
}

void FirmwareContext::showUpdating()
{
  EmptyLayout* layout = WidgetCreator::getEmptyLayout();
  setLayout(layout);

  Label* updating_msg = new Label(1);
  layout->addWidget(updating_msg);
  updating_msg->setText(STR_WAIT);
  updating_msg->setWidth(UI_WIDTH);
  updating_msg->setHeight(UI_HEIGHT);
  updating_msg->setGravity(IWidget::GRAVITY_CENTER);
  updating_msg->setAlign(IWidget::ALIGN_CENTER);
  updating_msg->setBackColor(COLOR_MAIN_BACK);
  layout->onDraw();
}