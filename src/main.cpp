/* Ota command
   platformio run --target upload --upload-port 192.168.178.150
 */
const char* version = "v 0.2b";

//#include "image.c"



/******* ntp Fabrice *******/

#include <WiFiUdp.h>
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 3600000);
//NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200, 60000);

/******* autoPID *******/
#include "pidController.h"


/******* EEPROM *****/

#include <EEPROM.h>

/******* Ota *******/
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


const char* ssid = "FRITZ!Box 6340 Cable";
const char* password = "18921448231986014152";
const char* host = "sergio8266";

int led_pin = 16;



/*** Encoder ***/
#include <ClickEncoder.h>
#include <menuIO/clickEncoderIn.h>

#define encA    0
#define encB    2
#define encBtn  12

/******* Menu *******/

#include <menuIO/chainStream.h>

#include "barField.h"
bool useUpdateEvent=true;//if false, use enterEvent when field value is changed.




#include <SPI.h>
#include <Ucglib.h>
#include <menu.h>
#include <menuIO/serialOut.h>
#include <menuIO/serialIn.h>
#include <menuIO/UCGLibOut.h>

using namespace Menu;

#define UC_Width 160
#define UC_Height 128

// offset (display margin)
#define offsetX 5
#define offsetY 10

//font size plus margins
#define fontX 6
#define fontY 11

//define colors
#define BLACK {0,0,0}
#define BLUE {0,0,255}
#define GRAY {128,128,128}
#define WHITE {255,255,255}
#define YELLOW {255,255,0}
#define RED {255,0,0}

const colorDef<rgb> colors[] MEMMODE={
        {{BLACK,BLACK},{BLACK,BLUE,BLUE}},//bgColor
        {{GRAY,GRAY},{WHITE,WHITE,WHITE}},//fgColor
        {{WHITE,BLACK},{YELLOW,YELLOW,RED}},//valColor
        {{WHITE,BLACK},{WHITE,YELLOW,YELLOW}},//unitColor
        {{WHITE,GRAY},{BLACK,BLUE,WHITE}},//cursorColor
        {{WHITE,YELLOW},{BLUE,RED,RED}},//titleColor
};

//Ucglib_ST7735_18x128x160_HWSPI ucg(UC_DC , UC_CS, UC_RST);

Ucglib_ST7735_18x128x160_SWSPI ucg(/*sclk=*/ 14, /*data=*/ 13, /*cd=*/ 5, /*cs=*/ 15, /*reset=*/ 4);

//to test
void printCentered(int y, char* str) {
        int newX = ucg.getWidth()/2-ucg.getStrWidth(str)/2;
        ucg.drawString(newX, y, 0, str);
}


char* constMEM hexDigit MEMMODE="0123456789ABCDEF";
char* constMEM hexNr[] MEMMODE={"0","x",hexDigit,hexDigit};
char buf1[]="0x11";//<-- menu will edit this text

// mess
/*
   char* constMEM hours_1[] MEMMODE="12";
   char* constMEM hours_2[] MEMMODE="0123456789";

   char* constMEM minute_1[] MEMMODE={"0","x",hexDigit,hexDigit};
 */
char time[]= "00:00";

float temp = 21.35;

result tempGraph() {
        //  ucg.drawXBitmap(0, 0, VUMeter, 128, 64, WHITE);       // draws background
        //placeholder

}


// have fun
result fun() {
        ucg.setColor(0, 255, 0, 0);
        ucg.setColor(1, 0, 255, 0);
        ucg.setColor(2, 255, 0, 255);
        ucg.setColor(3, 0, 255, 255);
        ucg.drawGradientBox(50, 30, 100, 90);
        return proceed;
}


// convert time from timeClient in a integer
// ritorna un intero multiplo di 15m
int intTime = timeClient.getHours()*4+((timeClient.getMinutes()-timeClient.getMinutes()%15)/15);
//TODO
// la dashboard vine chiamata nel loop, la dashModifier(event e) setta un flag
//in base all evento e lampeggia e colorizza i cursori

enum cursorModifier { normal, focusIn, modified, exited, test};
cursorModifier m = normal;
void heating() {
  // heating animation

  int blink = millis()%200>0 ? false : true ;
  //debug


    for (int i=0; i<5; i++) {
            ucg.setColor(0,0,200);
            switch (i) {
            case 0: ucg.drawDisc(120, 100, 5, UCG_DRAW_LOWER_RIGHT); delay(200); break;
            case 1: ucg.drawDisc(120, 100, 5, UCG_DRAW_LOWER_LEFT); delay(200); break;
            case 2: ucg.drawDisc(120, 100, 5, UCG_DRAW_UPPER_LEFT); delay(200); break;
            case 3: ucg.drawDisc(120, 100, 5, UCG_DRAW_UPPER_RIGHT); delay(200); break;
            case 4: {
                    ucg.setColor(0,0,0);
                    ucg.drawDisc(120, 100, 5, UCG_DRAW_ALL);
                    break;
            }
            }
    }
}
int tempHistory(int samples) {
  return 0; //placeholder return average
}

void dash() {



        const int baselineHigh = 80;


        //24h in 15m intervals
        static int timeLine[96] ={0};

        //dummy
        for (int i = 34; i < 76; i++) {
                timeLine[i] = 1;
        }

        //draw timeline
        ucg.setColor(255,0,0);
        ucg.drawBox(0+offsetX, baselineHigh, ucg.getWidth()-offsetX, 3);

        //show set periods
        ucg.setColor(0,0,255);
        for (int i=0; i<96; i++) {
                if (timeLine[i] == 1) {
                        ucg.drawFrame(ucg.getWidth()/96*i, baselineHigh-10, ucg.getWidth()/96, 10);
                }
        }

        switch (m) {
        case normal: {
                //track actual time
                ucg.drawCircle(intTime*ucg.getWidth()/96, baselineHigh+3+2, 2, UCG_DRAW_ALL);
                heating();
                break;

        }
        case focusIn: {

                for (int i=0; i<5; i++) {
                        int last = 0;
                        //cancel the area for the next update
                        ucg.setClipRange(0, baselineHigh+3, ucg.getWidth(), 10);
                        ucg.setColor(0,0,0);
                        ucg.drawBox(0,0,1000,1000);
                        ucg.undoClipRange();
                        int blinkColor = (last == 0) ?  255 : 0;
                        ucg.setColor(0,blinkColor,0); //focu color
                        ucg.drawTriangle(intTime,baselineHigh+3+2, intTime+4, baselineHigh+3+7, intTime-4, baselineHigh+3+7);
                        last = !last;
                        delay(300);
                }
                ucg.setColor(255,255,255);



                break;

        }
        case modified: {

                //cancel the area for the next update
                ucg.setClipRange(0, baselineHigh+3, ucg.getWidth(), 10);
                ucg.setColor(0,0,0);
                ucg.drawBox(0,0,1000,1000);
                ucg.undoClipRange();

                ucg.setColor(0,255,0);

                ucg.drawTriangle(intTime,baselineHigh+3+2, intTime+4, baselineHigh+3+7, intTime-4, baselineHigh+3+7);

                ucg.setColor(255,255,255);

                break;
        }
        case exited: {
                for (int i=0; i<6; i++) {
                        int last = 0;
                        //cancel the area for the next update
                        ucg.setClipRange(0, baselineHigh+3, ucg.getWidth(), 10);
                        ucg.setColor(0,0,0);
                        ucg.drawBox(0,0,1000,1000);
                        ucg.undoClipRange();
                        int blinkColor = (last == 0) ?  255 : 0;
                        ucg.setColor(0,0,blinkColor); // exit color
                        ucg.drawTriangle(intTime,baselineHigh+3+2, intTime+4, baselineHigh+3+7, intTime-4, baselineHigh+3+7);
                        last = !last;
                        delay(300);
                }
                ucg.setColor(255,255,255);
                break;
        }


        default: {

                break;
        }
        } // closing switch
}

result dashModifier(eventMask e) {
        //debug
        Serial.println(" ");
        Serial.print("event throw is : ");
        Serial.println(e);

/*

   noEvent no event (use if you do not want an handler function)

   activateEvent the item is about to be active (system event) (do not use)

   enterEvent entering navigation level (this menuNode is now active)

   exitEvent leaving navigation level

   returnEvent entering previous level (return) (not implemented yet)

   focusEvent element just gained focus

   blurEvent element about to lose focus

   selFocusEvent child just gained focus

   selBlurEvent child about to lose focus

   updateEvent field values has been updated, this is only used if options->useUpdateEvent=true otherwise enterEvent will be used. (from 3.1.0)

   anyEvent all events



   events can be combined like:

   enterEvent | exitEvent | updateEvent

 */


        switch (e) {
        case enterEvent: {
                m = focusIn;
                break;
        }
        case updateEvent: {
                m = modified;
                break;
        }

        case exitEvent: {
                m = exited;
                dash();
                m = normal;
                return proceed;
                //break;
        }
        default: {};
        }
        dash(); // call outside idling

        return proceed;
}
//customizing a menu prompt look
class interactiveDash : public menu {
public:
interactiveDash(constMEM menuNodeShadow& shadow) : menu(shadow) {
}
Used printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len,idx_t) override {
        return out.printRaw("Set Time",len);
}
};
altMENU(interactiveDash,subMenu,"zzo ne so!",doNothing,noEvent,wrapStyle,(Menu::_menuData|Menu::_canNav)
        ,FIELD(intTime, "start time", "",0,96,1,1,dashModifier,anyEvent,noStyle)
        //,FIELD(intTime, "end time", "",0,96,1,1,dashModifier,updateEvent,noStyle)
        ,EXIT("Cancel")
        );



byte relay = 1; // HIGH ---> off
byte lastState = 1;
result store() {
        digitalWrite(led_pin, relay);

        if (lastState!= relay) {
                //debug states la conversione con + non funziona !!!
                Serial.print("Last state was: ");
                Serial.println(lastState);
                Serial.print("New state is: ");
                Serial.println(relay);
                EEPROM.write(0, relay);
                EEPROM.commit();
                delay(500);
                Serial.print(" in memory is: ");
                Serial.println(EEPROM.read(0));
                lastState= relay;
        }



        ucg.setColor(255,0,0);
        ucg.setClipRange(0, 120, 160, 8);
        ucg.drawString(0, 128, 0, "Saved");
        delay(500);
        ucg.setColor(0,0,0);
        ucg.drawBox(0, 120,160,8);
        //ucg.clearScreen();
        ucg.undoClipRange();
        ucg.setColor(255,255,255);

        return proceed;
}
TOGGLE(relay,startstop,"caldaia: ",doNothing,noEvent,noStyle//,doExit,enterEvent,noStyle
       ,VALUE("On",0,store,noEvent)
       ,VALUE("Off",1,store,noEvent)
       );

byte isOTA_ON = 0;
result setMode() {
        EEPROM.write (1, isOTA_ON);
        EEPROM.commit();
        static byte last = 0;
        if (isOTA_ON != last) {
                if (isOTA_ON == 1) {
                        //ucg.setClipRange(ucg_int_t x, ucg_int_t y, ucg_int_t w, ucg_int_t h)
                        ucg.setPrintPos(ucg.getWidth()-(ucg.getWidth()*.9), ucg.getHeight()-(ucg.getHeight()*.2));
                        ucg.println("OTA System will be online at next reboot");
                        // evita il reset del cagnaccio!!! da testare
                        yield();

                }
                else if (isOTA_ON ==2) {

                }
                else {
                        ucg.setPrintPos(ucg.getWidth()-(ucg.getWidth()*.9), ucg.getHeight()-(ucg.getHeight()*.2));
                        ucg.println("OTA System is offline");
                }
                last = isOTA_ON;
        }



        return proceed;
}
TOGGLE(isOTA_ON,OTA_status,"OTA Status: ",doNothing,noEvent,noStyle       //,doExit,enterEvent,noStyle
       ,VALUE("On",1,setMode,noEvent)
       ,VALUE("Off",0,setMode,noEvent)
       ,VALUE("always",2,setMode,noEvent)


       );
void restart() {
        delay(1000);
        ESP.restart();
}




//customizing a prompt look!
//by extending the prompt class
//this prompt will count seconds and update himself on the screen.
class altPrompt : public prompt {
public:
unsigned int t=0;
unsigned int last=0;
altPrompt(constMEM promptShadow& p) : prompt(p) {
}
Used printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len,idx_t) override {
        last=t;
        return out.printRaw(String(t).c_str(),len);
}
virtual bool changed(const navNode &nav,const menuOut& out,bool sub=true) {
        t=millis()/1000;
        return last!=t;
}
};

template<typename T>
class leadsField : public menuField<T> {
public:
// leadsField(const menuFieldShadow<T>& shadow):menuField<T>(shadow) {}
leadsField(constMEM menuFieldShadow<T>& shadow) : menuField<T>(shadow) {
}


Used printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len) {
        menuField<T>::reflex=menuField<T>::target();
        prompt::printTo(root,sel,out,idx,len);
        bool ed=this==root.navFocus;
        out.print((root.navFocus==this&&sel) ? (menuField<T>::tunning ? '>' : ':') : ' ');
        out.setColor(valColor,sel,menuField<T>::enabled,ed);
        char buffer[]="      ";
        sprintf(buffer, "%03d", menuField<T>::reflex);
        out.print(buffer);
        out.setColor(unitColor,sel,menuField<T>::enabled,ed);
        print_P(out,menuField<T>::units(),len);
        return len;
}
};

//list of allowed characters
char* const digit="0123456789";
char* const hexChars MEMMODE="0123456789ABCDEF";
char* const alphaNum[] MEMMODE = {" 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"};
//individual character validators
char* constMEM validData[] MEMMODE={hexChars,hexChars,hexChars,hexChars};

char* constMEM validIP[] MEMMODE = {"012",digit,digit,"."};
char buf0[]="000.000.000.000";
//define "Op 0" without macro
constMEM char op1Text[] MEMMODE="IP";//field name
constMEM textFieldShadowRaw op1InfoRaw MEMMODE={
        (callback)doNothing,
        (Menu::systemStyles)(_noStyle|_canNav|_parentDraw),
        op1Text,
        enterEvent,
        noStyle,
        buf0,//edit buffer
        validIP,
        4
};//MEMMODE static stuff
constMEM textFieldShadow& op1Info=*(textFieldShadow*)&op1InfoRaw;//hacking c++ to use progmem (hugly)
textField option0(op1Info);

//char buf2[]="0000";

#define MINVALUE 0
#define MAXVALUE 255


MENU(mainMenu,"Main menu",doNothing,noEvent,wrapStyle
     //,FIELD(temp, "Temperature", "C°",5,30,1,0.1,doNothing,noEvent,noStyle)
    //  ,BARFIELD(temp,"Temperature","C°",5,30,1,0.1,doNothing,noEvent,wrapStyle)//numeric field with a bar
    ,BARFIELD(setpoint,"Temperature","C°",MINVALUE,MAXVALUE,10,1,doNothing,noEvent,wrapStyle)//numeric field with a bar

     //,altFIELD(leadsField,temp,"Temperature","%",0,100,10,1,doNothing,enterEvent,wrapStyle)


     // TODO reuse example come in help, ex. end time

     //,OP("have fun",test,enterEvent)
     //,OBJ(option0) //field test ip formatted


     ,SUBMENU(subMenu)
     //,OP("restart", restart, enterEvent)
     //,altOP(altPrompt,"",doNothing,anyEvent)
     ,SUBMENU(startstop)
     ,SUBMENU(OTA_status)
     //,EDIT("Hex",buf1,hexNr,doNothing,noEvent,noStyle)
     ,EXIT("<Back")
     );

#define MAX_DEPTH 2

MENU_OUTPUTS(out,MAX_DEPTH
             ,UCG_OUT(ucg,colors,fontX,fontY,offsetX,offsetY,{0,0,UC_Width/fontX,UC_Height/fontY})
             ,SERIAL_OUT(Serial)
             );

/*** encoder ***/
ClickEncoder encoder = ClickEncoder(encA, encB, -1, 2);
//use encoderClickIn

ClickEncoderStream encStream (encoder, 1); // secondo argomento sensivity

serialIn serial(Serial);


MENU_INPUTS(in,&encStream,&serial);


//input from the encoder + encoder button + serial
//Stream* inputsList[]={&Serial};
//Stream* inputsList[]={&encStream,&Serial};


//chainStream<1> in(inputsList);//2 is the number of inputs

// fine encoder input

/* originale
   Stream* inputsList[]={&Serial};
   chainStream<1> in(inputsList);//1 is the number of inputs
 */



// NAVROOT(nav,mainMenu,MAX_DEPTH,serial,out);
NAVROOT(nav,mainMenu,MAX_DEPTH,in,out);



//when menu is suspended
result idle(menuOut& o,idleEvent e) {
        o.clear();
        // ucg.clearScreen();
        switch(e) {
        case idleStart: o.println("exit menu!");
                delay(500);
                o.clear();
                //ucg.clearScreen();
                break;

        case idling: {
                printCentered(60, "sleeping...");
                delay(500);
                ucg.clearScreen();

                //ucg.print("suspended...");
                break;
        }



        case idleEnd: o.println("resuming menu.");
                delay(500);
                //o.clear();
                ucg.clearScreen();
                //ucg.clearWriteError();

                break;


        }
        return proceed;
}


int mode = 0;

int readRunMode() {
        //debug
        Serial.print("Last state on EEPROM[O]was: ");
        Serial.println(EEPROM.read(0));
        Serial.print("Last state on EEPROM[1]was: ");
        Serial.println(EEPROM.read(1));

        mode = EEPROM.read(1);
        // TODO leggere anche il relay


        return mode;
}

void runMode_standard() {
        /********ntp *********/
        timeClient.begin();


        nav.idleTask=idle;//point a function to be used when menu is suspended
        nav.useUpdateEvent= true;
        nav.showTitle=true; // non so bene cosa faccia

        // TODO non parte con il menu in idle...risolvere!
        //nav.idleOn(dash(noEvent));//this menu will start on idle state, press select to enter menu



}

void runMode_OTA() {
        // take the relay down
        digitalWrite(led_pin, HIGH);
        //back to standardMode if crash or reboot
        EEPROM.write(1, 0);
        EEPROM.commit();



        /******* Ota handling *******/

        ArduinoOTA.setHostname(host);
        ArduinoOTA.onStart([]() {   // switch off all the PWMs during upgrade
                //  for(int i=0; i<N_DIMMERS;i++)
                //    analogWrite(dimmerPin[i], 0);
                //    analogWrite(led_pin,0);

                /*
                   String type;
                   if (ArduinoOTA.getCommand() == U_FLASH)
                   type = "sketch";
                   else // U_SPIFFS
                   type = "filesystem";

                   // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                 */
                ucg.clearScreen();

                //ucg.drawString(ucg.getWidth()/2, ucg.getHeight()/2 -ucg.getStrWidth("Start updating " + type), 0 , "Start updating " + type);
                //Serial.println("Start updating " + type);

                ucg.setFontPosBaseline();
                ucg.setColor(255,255,255);
                ucg.drawString(ucg.getWidth()/2 -ucg.getStrWidth("OTA starting")/2, ucg.getHeight()/2, 0, "OTA starting");
                delay(1000);
                ucg.clearScreen();
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
                //ucg.drawHLine(ucg.getWidth()/2, 0, (progress / (total / 100)));
                ucg.setColor(0,255,0);
                //ucg.drawFrame(0, ucg.getHeight()/2+5,  (progress / (total / 100)), 3);
                ucg.drawFrame(ucg.getWidth()/2-ucg.getStrWidth("OTA is on the way...")/2, ucg.getHeight()/2+5,  (progress / (total / 100)), 3);

                ucg.setColor(255,255,255);
                ucg.drawString(ucg.getWidth()/2-ucg.getStrWidth("OTA is on the way...")/2, 64, 0, "OTA is on the way...");


                //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onEnd([]() {   // do a fancy thing with our board led at end
                ucg.clearScreen();

                // for (int i=0; i<30; i++)
                // {
                //         analogWrite(led_pin,(i*100) % 1001);
                //         delay(50);
                // }
                //EEPROM.write(1,0);
                //EEPROM.commit();
                ucg.drawString( ucg.getWidth()/2 -ucg.getStrWidth("Done...restarting")/2, ucg.getHeight()/2, 0, "Done...restarting");
                delay(1000);
        });
        ArduinoOTA.onError([](ota_error_t error) {
                Serial.printf("Error[%u]: ", error);
                ucg.drawString(ucg.getWidth()/2-ucg.getStrWidth("Something went wrong :-(")/2, ucg.getHeight()/2, 0, "Something went wrong :-(");
                delay(1000);

                // tutte da correggere

                if (error == OTA_AUTH_ERROR) ucg.drawString(ucg.getWidth()/2, ucg.getHeight()/2 -ucg.getStrWidth("Auth failed"), 0, "Auth failed");

                //Serial.println("Auth Failed");
                else if (error == OTA_BEGIN_ERROR) ucg.drawString(ucg.getWidth()/2, ucg.getHeight()/2 -ucg.getStrWidth("Begin failed"), 0, "Begin failed");
                //Serial.println("Begin Failed");
                else if (error == OTA_CONNECT_ERROR) ucg.drawString(ucg.getWidth()/2, ucg.getHeight()/2 -ucg.getStrWidth("Connect Failed"), 0, "Connect Failed");

                //Serial.println("Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) ucg.drawString(ucg.getWidth()/2, ucg.getHeight()/2 -ucg.getStrWidth("Receive Failed"), 0, "Receive Failed");

                //Serial.println("Receive Failed");
                else if (error == OTA_END_ERROR) ucg.drawString(ucg.getWidth()/2, ucg.getHeight()/2 -ucg.getStrWidth("End Failed"), 0, "End Failed");


                //else Serial.println("sminchiato qualcosa..." );

                //Serial.println("End Failed");
                ESP.restart();
        });

        /* setup the OTA server */

        ArduinoOTA.begin();
        //visual placeholder
        ucg.setColor(0,255,0);
        ucg.drawDisc(0, 0, 25, UCG_DRAW_LOWER_RIGHT);
        delay(500);
}


void anytimeOta() {

  pidInit();
        /******* Ota handling *******/

        ArduinoOTA.setHostname(host);
        ArduinoOTA.onStart([]() { // switch off all the PWMs during upgrade
                //  for(int i=0; i<N_DIMMERS;i++)
                //    analogWrite(dimmerPin[i], 0);
                //    analogWrite(led_pin,0);

                /*
                   String type;
                   if (ArduinoOTA.getCommand() == U_FLASH)
                   type = "sketch";
                   else // U_SPIFFS
                   type = "filesystem";

                   // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                 */
                ucg.clearScreen();

                //ucg.drawString(ucg.getWidth()/2, ucg.getHeight()/2 -ucg.getStrWidth("Start updating " + type), 0 , "Start updating " + type);
                //Serial.println("Start updating " + type);

                ucg.setFontPosBaseline();
                ucg.setColor(255,255,255);
                ucg.drawString(ucg.getWidth()/2 -ucg.getStrWidth("OTA starting")/2, ucg.getHeight()/2, 0, "OTA starting");
                delay(1000);
                ucg.clearScreen();
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
                //ucg.drawHLine(ucg.getWidth()/2, 0, (progress / (total / 100)));
                ucg.setColor(0,255,0);
                //ucg.drawFrame(0, ucg.getHeight()/2+5,  (progress / (total / 100)), 3);
                ucg.drawFrame(ucg.getWidth()/2-ucg.getStrWidth("OTA is on the way...")/2, ucg.getHeight()/2+5,  (progress / (total / 100)), 3);

                ucg.setColor(255,255,255);
                ucg.drawString(ucg.getWidth()/2-ucg.getStrWidth("OTA is on the way...")/2, 64, 0, "OTA is on the way...");


                //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onEnd([]() { // do a fancy thing with our board led at end
                ucg.clearScreen();

                // for (int i=0; i<30; i++)
                // {
                //         analogWrite(led_pin,(i*100) % 1001);
                //         delay(50);
                // }
                //EEPROM.write(1,0);
                //EEPROM.commit();
                ucg.drawString( ucg.getWidth()/2 -ucg.getStrWidth("Done...restarting")/2, ucg.getHeight()/2, 0, "Done...restarting");
                delay(1000);
        });
        ArduinoOTA.onError([](ota_error_t error) {
                ucg.clearScreen();

                Serial.printf("Error[%u]: ", error);
                ucg.drawString(ucg.getWidth()/2-ucg.getStrWidth("Something went wrong :-(")/2, ucg.getHeight()/2, 0, "Something went wrong :-(");
                delay(1000);

                // tutte da correggere

                if (error == OTA_AUTH_ERROR) ucg.drawString(ucg.getWidth()/2, ucg.getHeight()/2 -ucg.getStrWidth("Auth failed"), 0, "Auth failed");

                //Serial.println("Auth Failed");
                else if (error == OTA_BEGIN_ERROR) ucg.drawString(ucg.getWidth()/2, ucg.getHeight()/2 -ucg.getStrWidth("Begin failed"), 0, "Begin failed");
                //Serial.println("Begin Failed");
                else if (error == OTA_CONNECT_ERROR) ucg.drawString(ucg.getWidth()/2, ucg.getHeight()/2 -ucg.getStrWidth("Connect Failed"), 0, "Connect Failed");

                //Serial.println("Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) ucg.drawString(ucg.getWidth()/2, ucg.getHeight()/2 -ucg.getStrWidth("Receive Failed"), 0, "Receive Failed");

                //Serial.println("Receive Failed");
                else if (error == OTA_END_ERROR) ucg.drawString(ucg.getWidth()/2, ucg.getHeight()/2 -ucg.getStrWidth("End Failed"), 0, "End Failed");


                //else Serial.println("sminchiato qualcosa..." );

                //Serial.println("End Failed");
                ESP.restart();
        });

        /* setup the OTA server */

        ArduinoOTA.begin();
        //visual placeholder
        // ucg.setColor(0,255,0);
        // ucg.drawDisc(0, 0, 25, UCG_DRAW_LOWER_RIGHT);
        // delay(500);
        /********ntp *********/
        timeClient.begin();


        nav.idleTask=idle;//point a function to be used when menu is suspended
        nav.useUpdateEvent= true;
        nav.showTitle=true; // non so bene cosa faccia

        // TODO non parte con il menu in idle...risolvere!
        //nav.idleOn(dash;//this menu will start on idle state, press select to enter menu

}
void setup() {

        EEPROM.begin(512);
        Serial.begin(115200);
        while(!Serial) ;
        Serial.flush();

        encoder.setButtonHeldEnabled(true);
        encoder.setDoubleClickEnabled(true);

        // Enable the button to be on pin 0.  Normally pin 0 is not recognized as a valid pin for a button,
        // this is to maintain backward compatibility with an old version of the library
        // This version can have the button on pin zero, and this call enables the feature.
        // in this version best to use pin -1 instead of 0 to disable button functions
        encoder.setButtonOnPinZeroEnabled(true);

        ucg.begin(UCG_FONT_MODE_TRANSPARENT);
        ucg.setFont(ucg_font_courB08_tr);//choose fized width font (monometric)
        ucg.setRotate270();
        ucg.setColor(255,255,255);
        ucg.setFontPosTop();
        //ucg.setRotate90();

        //Welcome Message
        ucg.clearScreen();
        printCentered(60, "the Winter is coming...");
        delay(500);
        ucg.clearScreen();
        printCentered(60, "Do you feel the winter?");
        delay(500);
        ucg.clearScreen();


        ucg.setFontPosBottom();




        /* set led_pin as output and default to off */
        pinMode(led_pin, OUTPUT);
        digitalWrite(led_pin, HIGH);


        Serial.println(F("Booting"));
        //ucg.println("Booting");


        //WiFi.persistent(false);

        WiFi.mode(WIFI_STA);

        WiFi.begin(ssid, password);

        while (WiFi.waitForConnectResult() != WL_CONNECTED) {

                Serial.println(F("Retrying connection..."));
        }
        Serial.println(F("Connected"));





        Serial.print(F("Ready"));
        Serial.print(F("IP address: "));
        Serial.println(WiFi.localIP());


        switch (readRunMode()) {

        case 0: runMode_standard(); break;
        case 1: runMode_OTA(); break;
        case 2: anytimeOta(); break;
        }


}


void encoderService() {
        static uint32_t lastService = 0;
        if (lastService + 1000 < micros()) {
                lastService = micros();
                encoder.service();
        }
}
void readEncoder () {
        static int lastCount, count; //int16_t
        count += encoder.getValue();

        //debug

        // if (count != lastCount) {
        //         Serial.print("Last Encoder Value was : ");
        //         Serial.println(lastCount);
        //         //lastCount = count;
        //         Serial.print("Encoder Value: ");
        //         Serial.println(count);
        //         //  insert nav hooks
        // }

        // if (count > lastCount) {
        //         lastCount = count;
        //         nav.doNav(navCmd(downCmd));
        // }
        // else if (count < lastCount) {
        //         lastCount = count;
        //         nav.doNav(navCmd(upCmd));
        // }
        // else nav.doNav(navCmd(noCmd));


        int d = count - lastCount;
        if (d <= -1) {
                lastCount = count;
                nav.doNav(navCmd(downCmd)); //menu::downCode;
        }
        if (d >= 1) {
                lastCount = count;
                nav.doNav(navCmd(upCmd)); //menu::upCode;
        }




}
void ShowTime() {
        static uint32_t lastService = 0;

        if (lastService + 1000 < millis()) {
                ucg.setClipRange(60, 0, 100, 10);
                ucg.setColor(0,0,0);
                ucg.drawBox(0,0,160,128);
                ucg.setColor(255,255,255);
                ucg.setPrintPos(96, 10);
                ucg.print(timeClient.getFormattedTime());
                lastService = millis();
                ucg.undoClipRange();
                //ucg.setPrintPos(ucg.getWidth()*.1, ucg.getHeight()*.9);
                // convert time from timeClient in a integer
                // ritorna un intero multiplo di 15m
                int intTime = timeClient.getHours()*4+((timeClient.getMinutes()-timeClient.getMinutes()%15)/15);

        }
}

void loop() {

        encoderService();
        //readEncoder();

        switch (mode) {
        case 0:  {
                nav.poll();
                if (nav.sleepTask) {
                        dash();
                }
                ShowTime();
                timeClient.update();
                break;
        }
        case 1: {
                ArduinoOTA.handle();
                printCentered(60, "OTA Mode");
                break;
        }
        case 2: {
                nav.poll();
                if (nav.sleepTask) {
                        dash();
                }
                ShowTime();
                timeClient.update();
                ArduinoOTA.handle();
                pidCompute();

                break;
        }
        default: break;
        }

}
