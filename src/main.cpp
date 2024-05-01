#include <Arduino.h>
#include <GEM_u8g2.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <WebSerialLite.h>
#include <led.h>
#if (USE_BUTTONS == 0)
  #include <KeyDetector.h>
  const byte channelA = 2;
  const byte channelB = 3;
  const byte buttonPin = 4;
  // Array of Key objects that will link GEM key identifiers with dedicated pins
  // (it is only necessary to detect signal change on a single channel of the encoder, either A or B;
  // order of the channel and push-button Key objects in an array is not important)
  Key keys[] = {{KEY_A, channelA}, {KEY_C, buttonPin}};
  //Key keys[] = {{KEY_C, buttonPin}, {KEY_A, channelA}};

  // Create KeyDetector object
  // KeyDetector myKeyDetector(keys, sizeof(keys)/sizeof(Key));
  // To account for switch bounce effect of the buttons (if occur) you may want to specify debounceDelay
  // as the third argument to KeyDetector constructor.
  // Make sure to adjust debounce delay to better fit your rotary encoder.
  // Also it is possible to enable pull-up mode when buttons wired with pull-up resistors (as in this case).
  // Analog threshold is not necessary for this example and is set to default value 16.
  KeyDetector myKeyDetector(keys, sizeof(keys)/sizeof(Key), /* debounceDelay= */ 5, /* analogThreshold= */ 16, /* pullup= */ true);

  bool secondaryPressed = false;  // If encoder rotated while key was being pressed; used to prevent unwanted triggers
  bool cancelPressed = false;  // Flag indicating that Cancel action was triggered, used to prevent it from triggering multiple times
  const int keyPressDelay = 1000; // How long to hold key in pressed state to trigger Cancel action, ms
  long keyPressTime = 0; // Variable to hold time of the key press event
  long now; // Variable to hold current time taken with millis() function at the beginning of loop()
#endif




//init display
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, SCL_PIN, SDA_PIN, U8X8_PIN_NONE);

//Items MainMenu
void displayAboutPage();
GEMItem menuItemButtonDisplayAbout("Infos", displayAboutPage);

//Stats page
int sketch_size = ESP.getSketchSize();
GEMItem menuItemSketchSize("Sketch Size:", sketch_size, true);

int led_brightness = 100;
//LED Control
void validateLEDBrightness();
GEMItem menuItemLEDBrightness("Brightness:", led_brightness, validateLEDBrightness);
bool enable_led = false;
GEMItem menuItemEnableLED("Enable LED:", enable_led);

//Pages
GEMPage menuPageMain("Main Menu");
GEMPage menuPageStatistics("Statistics");
GEMPage menuPageLEDControl("LED Control");

//Page link Items
GEMItem menuItemButtonMainToStatistics("Statistics", menuPageStatistics);
GEMItem menuItemButtonMainToLEDControl("LED Control", menuPageLEDControl);

//forward declaration
void setupMenu();

GEM_u8g2 menu(u8g2, /* menuPointerType= */ GEM_POINTER_ROW, /* menuItemsPerScreen= */ GEM_ITEMS_COUNT_AUTO, /* menuItemHeight= */ 10, /* menuPageScreenTopOffset= */ 17, /* menuValuesLeftOffset= */ 86);

//OTA
const char* ssid = "KroneNet";
const char* password = "test123test456test789";

//Webserver
AsyncWebServer server(80);

unsigned long ota_progress_millis = 0;

//Restart invoke
unsigned long restart_start_millis = 0;
unsigned long restart_timer_millis = 0;
unsigned restart_delay = 5000;
bool restart_device = false;

int percentToInt(int percent)
{
  return percent*255/100;
}
void recvMsg(uint8_t *data, size_t len){
  WebSerial.print("Received Data...\n");
  String d = "";
  for(int i=0; i < (int)len; i++){
    d += char(data[i]);
  }
  WebSerial.println(d);
}

void onOTAStart() {
  WebSerial.print("OTA update started!\n");
  u8g2.clear();
}

void onOTAProgress(size_t current, size_t final) {
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();

    char buffer[21];
    char percent_buffer[5];
    float percent = (((float)current)/(float)final);
    snprintf(buffer, 21, "%u/%u bytes", current, final);
    snprintf(percent_buffer, 5, "%u%%",(int)(percent*100));

    u8g2.firstPage();
    do {
      u8g2.drawFrame(2,17,(int)(u8g2.getDisplayWidth()-4),19);
      u8g2.drawBox(4,19,(int)((u8g2.getDisplayWidth()-8)*percent),15);
      u8g2.setFontMode(1);
      u8g2.setFont(u8g2_font_6x12_tr);
      u8g2.drawStr(8,41, buffer);
      u8g2.setDrawColor(2);
      u8g2.drawStr((int)(u8g2.getDisplayWidth()/2)-4, 22, percent_buffer);
      //u8g2.setCursor(8,5);
      //u8g2.print();
    } while (u8g2.nextPage());
  }
}

void onOTAEnd(bool success) {
  if (success) {
    WebSerial.println("OTA update finished successfully!");
  } else {
    WebSerial.println("There was an error during OTA update!");
  }
  restart_start_millis = millis();
  restart_timer_millis = millis();
  restart_device = true;
}

static const char _ELEMENT_HTML[] PROGMEM = {
  "<html><head><style type=\"text/css\">body { font-size: lager; }</style>"
  "<body>Hello to this website<br>Go to <a href=\"/update\">Webupdate</a><br>Go to <a href=\"/webserial\">Webserial</a>"
  "</body></html>"
};
void setup() {
  // Serial communication setup
  //Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_PIN_1, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = FPSTR(_ELEMENT_HTML);
    //if (request->hasArg("foo"))
    //html += "test";
    request->send(200, "text/html", html);
  });

  ElegantOTA.begin(&server);
  ElegantOTA.setAutoReboot(false);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  WebSerial.begin(&server);
  WebSerial.onMessage(recvMsg);
  
  server.begin();
  WebSerial.print("HTTP server started...\n");

  WebSerial.print("Init GUI...\n");
  #if USE_BUTTONS
    u8g2.begin(OK_PIN, RIGHT_PIN, LEFT_PIN, UP_PIN, DOWN_PIN, CANCEL_PIN);
  #else
    u8g2.begin();
    menu.invertKeysDuringEdit(1);
  #endif
  WebSerial.print("Draw Menu...\n");
  menu.setSplash(splash_width, splash_height, splash_bits);
  menu.hideVersion();
  menu.setSplashDelay(7000);
  menu.init();
  setupMenu();
  menu.drawMenu();
}

void setupMenu() {
  WebSerial.print("Called setupMenu()");
  
  menuPageMain.addMenuItem(menuItemButtonMainToLEDControl);
  menuPageMain.addMenuItem(menuItemButtonDisplayAbout);
  menuPageMain.addMenuItem(menuItemButtonMainToStatistics);
  

  menuPageStatistics.addMenuItem(menuItemSketchSize);
  menuPageStatistics.setParentMenuPage(menuPageMain);

  menuPageLEDControl.addMenuItem(menuItemLEDBrightness);
  menuPageLEDControl.addMenuItem(menuItemEnableLED);
  menuPageLEDControl.setParentMenuPage(menuPageMain);

  menu.setMenuPageCurrent(menuPageMain);
}

char restart_timer_label[30];
void loop() {
  #if USE_BUTTONS
    if (menu.readyForKey()) {
      menu.registerKeyPress(u8g2.getMenuEvent());
    }
  #else
      now = millis();
      // If menu is ready to accept button press...
      if (menu.readyForKey()) {
        // ...detect key press using KeyDetector library
        // and pass pressed button to menu
        myKeyDetector.detect();
      
        switch (myKeyDetector.trigger) {
          case KEY_C:
            // Button was pressed
            WebSerial.println("Button pressed");
            // Save current time as a time of the key press event
            keyPressTime = now;
            break;
        }
        /* Detecting rotation of the encoder on release rather than push
        (i.e. myKeyDetector.triggerRelease rather myKeyDetector.trigger)
        may lead to more stable readings (without excessive signal ripple) */
        switch (myKeyDetector.triggerRelease) {
          case KEY_A:
            // Signal from Channel A of encoder was detected
            if (digitalRead(channelB) == LOW) {
              // If channel B is low then the knob was rotated CCW
              if (myKeyDetector.current == KEY_C) {
                // If push-button was pressed at that time, then treat this action as GEM_KEY_LEFT,...
                WebSerial.println("Rotation CCW with button pressed (release)");
                menu.registerKeyPress(GEM_KEY_LEFT);
                // Button was in a pressed state during rotation of the knob, acting as a modifier to rotation action
                secondaryPressed = true;
              } else {
                // ...or GEM_KEY_UP otherwise
                WebSerial.println("Rotation CCW (release)");
                menu.registerKeyPress(GEM_KEY_UP);
              }
            } else {
              // If channel B is high then the knob was rotated CW
              if (myKeyDetector.current == KEY_C) {
                // If push-button was pressed at that time, then treat this action as GEM_KEY_RIGHT,...
                WebSerial.println("Rotation CW with button pressed (release)");
                menu.registerKeyPress(GEM_KEY_RIGHT);
                // Button was in a pressed state during rotation of the knob, acting as a modifier to rotation action
                secondaryPressed = true;
              } else {
                // ...or GEM_KEY_DOWN otherwise
                WebSerial.println("Rotation CW (release)");
                menu.registerKeyPress(GEM_KEY_DOWN);
              }
            }
            break;
          case KEY_C:
            // Button was released
            WebSerial.println("Button released");
            if (!secondaryPressed) {
              // If button was not used as a modifier to rotation action...
              if (now <= keyPressTime + keyPressDelay) {
                // ...and if not enough time passed since keyPressTime,
                // treat key that was pressed as Ok button
                menu.registerKeyPress(GEM_KEY_OK);
              }
            }
            secondaryPressed = false;
            cancelPressed = false;
            break;
        }
        // After keyPressDelay passed since keyPressTime
        if (now > keyPressTime + keyPressDelay) {
          switch (myKeyDetector.current) {
            case KEY_C:
              if (!secondaryPressed && !cancelPressed) {
                // If button was not used as a modifier to rotation action, and Cancel action was not triggered yet
                WebSerial.println("Button remained pressed");
                // Treat key that was pressed as Cancel button
                menu.registerKeyPress(GEM_KEY_CANCEL);
                cancelPressed = true;
              }
              break;
          }
        }
      }
  #endif
  ElegantOTA.loop();
  if (enable_led)
  {
    analogWrite(LED_PIN_1, percentToInt(led_brightness));
  }
  else
  {
    analogWrite(LED_PIN_1, 0);
  }
  //restart invoked
  if (restart_device)
  {
    if (millis() - restart_start_millis > restart_delay)
    {
      ESP.restart();
      restart_device = false;
    }
    else if (millis() - restart_timer_millis > 1000)
    {
      int restart_diff_millis = (restart_delay-(restart_timer_millis - restart_start_millis));
      snprintf(restart_timer_label, 29, "Restart in %u sek...", (restart_diff_millis/1000));
      u8g2.firstPage();
      do {
        u8g2.setFont(u8g2_font_6x12_tr);
        u8g2.drawStr(u8g2.getDisplayWidth()/2 - strlen(restart_timer_label)*3, u8g2.getDisplayHeight()/2 - 4, restart_timer_label);
      } while (u8g2.nextPage());
      restart_timer_millis = millis();
    }
  }
}
unsigned long previousMillis = 0;
int current_about_page = 0;
char str_buffer[2][22];

void displayAboutPageEnter() {
  WebSerial.print("Entering About Page\n");
  WebSerial.printf("This is an int %u", ESP.getBootMode());
  current_about_page = 0;
  previousMillis = 0;
  u8g2.clear();
}

void displayAboutPageLoop() {
  byte key = u8g2.getMenuEvent();
  if (key == GEM_KEY_CANCEL) {
    menu.context.exit();
    return;
  } 
  else if (key == GEM_KEY_OK)
  {
    if (current_about_page == 0) {
      current_about_page = 1;
    } else {
      current_about_page = 0;
    }
  }
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= LOOP_UPDATE_DELAY) {
    previousMillis = currentMillis;
    switch (current_about_page)
    {
    case 0:
      snprintf(str_buffer[0], 22, "Static things....");
      snprintf(str_buffer[1], 22, "BM: %u, BV: %u, CID: %u", ESP.getBootMode(), ESP.getBootVersion(), ESP.getChipId());
      snprintf(str_buffer[2], 22, "CV: 3.1.2, MHZ: 80, C", ESP.getCoreVersion(), ESP.getCpuFreqMHz(), ESP.getCycleCount());
      u8g2.firstPage();
      do {
        u8g2.setFont(u8g2_font_6x12_tr);
        u8g2.drawStr(2, 2, str_buffer[0]);
        u8g2.drawStr(2, 17+2, str_buffer[1]);
        u8g2.drawStr(2, 17+u8g2.getMaxCharHeight()+2, str_buffer[2]);
      } while (u8g2.nextPage());
      break;
    case 1:
      snprintf(str_buffer[0], 22, "Static things2....");
      snprintf(str_buffer[1], 22, "BM: %u, BV: %u, CID: %u", ESP.getBootMode(), ESP.getBootVersion(), ESP.getChipId());
      snprintf(str_buffer[2], 22, "CV: 3.1.2, MHZ: 80, C", ESP.getCoreVersion(), ESP.getCpuFreqMHz(), ESP.getCycleCount());
      u8g2.firstPage();
      do {
        u8g2.setFont(u8g2_font_6x12_tr);
        u8g2.drawStr(2, 2, str_buffer[0]);
        u8g2.drawStr(2, 17+2, str_buffer[1]);
        u8g2.drawStr(2, 17+u8g2.getMaxCharHeight()+2, str_buffer[2]);
      } while (u8g2.nextPage());
    default:
      break;
    }
  }
}
void displayAboutPageExit() {
  menu.reInit();
  menu.drawMenu();
  menu.clearContext();

  WebSerial.println("Exit About Page");
}

void displayAboutPage() {
  menu.context.loop = displayAboutPageLoop;
  menu.context.enter = displayAboutPageEnter;
  menu.context.exit = displayAboutPageExit;
  menu.context.allowExit = false;
  menu.context.enter();
}


void validateLEDBrightness() {
  if (led_brightness > 100)
  {
    led_brightness = 100;
  }
  else if (led_brightness < 0)
  {
    led_brightness = 0;
  }
}

