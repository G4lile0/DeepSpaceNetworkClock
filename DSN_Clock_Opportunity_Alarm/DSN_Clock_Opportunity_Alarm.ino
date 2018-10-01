//  	Arduino NASA / JPL (DSN) Deep Space Clock, currently focus on the fate of the Opportunity rover 
//	
//  	by G4lile0 
//   
//  	2-10-2018  first public release
//	two weekend project // need a big clean up ...  

#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
WiFiClient client; // wifi client object
#define USE_SERIAL Serial
#include "naif_id.h"


#include <FS.h>
#include <SD.h>



#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"



#include <TaskScheduler.h>




AudioGeneratorMP3 *mp3;
AudioFileSourceSD *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;



// Callback methods prototypes
void t1Callback();    //  timer clock 
void t2Callback();    //  timer pictures carrousel 
void t3Callback();    //  news Ticker
void t4Callback();    //  Animation active receiving opportunity




//Tasks
Task t5();
Task t1(1000, TASK_FOREVER , &t1Callback);   // timer clock
Task t2(5000, TASK_FOREVER , &t2Callback);  // photos
Task t3(150, TASK_FOREVER, &t3Callback);    // scroll news 
Task t4(300, TASK_FOREVER, &t4Callback);    //Animation active receiving opportunity



Scheduler runner;








// Use hardware SPI
TFT_eSPI tft = TFT_eSPI();

// JPEG decoder library
#include <JPEGDecoder.h>




//WiFi Credentials and server host.

const char* host         = "eyes.nasa.gov";
const char* ssid         = "";
const char* password     = "";



const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;


byte opportunity_status=0;  // 0 no info  // 1 active listening   // 2 receiving data!!!
byte opportunity_status_previo=50;  // 0 no info  // 1 active listening   // 2 receiving data!!!



// Variables and funtion for Intro Screen

// With 1024 stars the update rate is ~65 frames per second
#define NSTARS 1024
uint8_t sx[NSTARS] = {};
uint8_t sy[NSTARS] = {};
uint8_t sz[NSTARS] = {};

uint8_t za, zb, zc, zx;






// Fast 0-255 random number generator from http://eternityforest.com/Projects/rng.php:
uint8_t __attribute__((always_inline)) rng()
{
  zx++;
  za = (za^zc^zx);
  zb = (zb+za);
  zc = (zc+(zb>>1)^za);
  return zc;
}

// end of Variables and funtion for Intro Screen

void setup() {

 
  za = random(256);
  zb = random(256);
  zc = random(256);
  zx = random(256);

  Serial.begin(115200);


  WiFi.begin(ssid,password);

  
  // Turning ON TFT Backlight 
  pinMode(32,OUTPUT);
  digitalWrite(32, HIGH);

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  // fastSetup() must be used immediately before fastPixel() to prepare screen
  // It must be called after any other graphics drawing function call if fastPixel()
  // is to be called again
  //tft.fastSetup(); // Prepare plot window range for fast pixel plotting

  //
  // Intro
  //

 unsigned int animation=0;
 unsigned int animation_delay=0;
 unsigned int show_text=1;

 
 // Not PROGMEM because I will play with this variable.
 String texto_1 = "I'm worried about the fateof the Opportunity Rover. This clock analyzes  NASA Deep Space Network data inrealtime seeking for its  answer....                                                    by @G4lile0";

 unsigned int hide_text=0;


  // nomal 500 .. for debug 50
  while (animation< 530) {

    
  //unsigned long t0 = micros();
  uint8_t spawnDepthVariation = 255;

  for(int i = 0; i < NSTARS; ++i)
  {
    if (sz[i] <= 1)
    {
      sx[i] = 160 - 120 + rng();
      sy[i] = rng();
      sz[i] = spawnDepthVariation--;
    }
    else
    {
      int old_screen_x = ((int)sx[i] - 160) * 256 / sz[i] + 160;
      int old_screen_y = ((int)sy[i] - 120) * 256 / sz[i] + 120;

      // This is a faster pixel drawing function for occassions where many single pixels must be drawn
      tft.drawPixel(old_screen_x, old_screen_y,TFT_BLACK);

      sz[i] -= 2;
      if (sz[i] > 1)
      {
        int screen_x = ((int)sx[i] - 160) * 256 / sz[i] + 160;
        int screen_y = ((int)sy[i] - 120) * 256 / sz[i] + 120;
  
        if (screen_x >= 0 && screen_y >= 0 && screen_x < 320 && screen_y < 240)
        {
          uint8_t r, g, b;
          r = g = b = 255 - sz[i];
          tft.drawPixel(screen_x, screen_y, tft.color565(r,g,b));
        }
        else
          sz[i] = 0; // Out of screen, die.
      }
    }
  }
  //unsigned long t1 = micros();
  //static char timeMicros[8] = {};

 // Calcualte frames per second
  // Serial.println(1.0/((t1 - t0)/1000000.0));
  animation=animation+1;
  //Serial.println(animation);
  

  if ((animation > 50) && (animation < 350))  {
  // Set "cursor" at top left corner of display (0,0) and select font 2
  // (cursor will move to next line automatically during printing with 'tft.println'
  //  or stay on the line is there is room for the text with tft.print)
  tft.setCursor(5, 0, 2);
  // Set the font colour to be white with a black background, set text size multiplier to 1
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  tft.setTextSize(2);
  // We can now plot text on screen using the "print" class
  tft.println("Opportunity Rover Alarm");}

  if ((animation > 100)&& (animation < 370))   {
    
  // Set "cursor" at top left corner of display (0,0) and select font 2
  // (cursor will move to next line automatically during printing with 'tft.println'
  //  or stay on the line is there is room for the text with tft.print)
  tft.setCursor(0, 50, 1);
  // Set the font colour to be white with a black background, set text size multiplier to 1
  tft.setTextColor(TFT_WHITE);  tft.setTextSize(2);

  if (animation_delay-animation>10) {
    animation_delay=animation;
    show_text = show_text+1;
    
        if (animation > 150)   {
            texto_1.setCharAt(hide_text, ' ');
            hide_text++;

            }
    }
    
  String texto = texto_1.substring(0 , show_text);     
  tft.println(texto);
    
  }



  // We can now plot text on screen using the "print" class
    
  }


  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0, 2);
  // Set the font colour to be white with a black background, set text size multiplier to 1
  tft.setTextColor(TFT_GREEN,TFT_BLACK);  tft.setTextSize(1);
  // We can now plot text on screen using the "print" class

  tft.println("Pictures Credit: NASA/JPL and Sean Doran");

  tft.println("Initializating WiFi");
  tft.println("Connecting NTP SERVER");

  tft.println("Connecting NASA /JPL servers");

 //init and get the time
 configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
 printLocalTime();
 printLocalTime();

//  tft.println("");
//  tft.fillScreen(TFT_BLACK);


  if (!SD.begin(21)) {
    tft.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    tft.println("No SD card attached");
    return;
  }

  tft.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    tft.println("MMC");
  } else if (cardType == CARD_SD) {
    tft.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    tft.println("SDHC");
  } else {
    tft.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  tft.printf("SD Card Size: %lluMB\n", cardSize);
  tft.println("initialisation done.");

  
  delay(500);

  drawSdJpeg("/op/opprtunity.jpg", 0, 0);     // This draws a jpeg pulled off the SD Card

  delay(3000);
  tft.fillScreen(TFT_BLACK);
  

  // drawSdJpeg("/opportunity3.jpg", 40, 60);     // This draws a jpeg pulled off the SD Card
//  delay(2000);



  
  runner.init();
  Serial.println("Initialized scheduler");
  runner.addTask(t1);
  Serial.println("added t1");
  t1.enable();

  runner.addTask(t2);
  Serial.println("added t2");
  t2.enable();

 runner.addTask(t3);
  Serial.println("added t3");
  t3.enable();

 runner.addTask(t4);
  Serial.println("added t4");

  ParseData(); 


}


byte daysleft = 0;
byte hoursleft = 0;
byte minleft =0;
byte secondsleft = 0;

// fist time we use an imposible number to forze updat
byte daysleft_previo = 240;
byte hoursleft_previo = 240;
byte minleft_previo = 240;
//byte secondsleft = 0;





signed long end_of_mission = 1539561600;  // 15-Oct-2018??

int foto = 0;

int delay_foto = 10;


// to check how many times to scroll the ticker bar 
byte ticker_iteration=0;

void loop()
{
  runner.execute();




    if (opportunity_status_previo != opportunity_status) {
        opportunity_status_previo= opportunity_status;
        

      
  if (opportunity_status==0) {
            t2.enable();
            t4.disable();
    }


  if (opportunity_status==1) {
            t2.disable();
            t4.enable();

            tft.fillRect(0, 66, 320, 174, TFT_BLACK);
   //            tft.fillScreen(TFT_BLACK);

            drawSdJpeg("/op/a1/ant.jpg", 0, 66);     // This draws a jpeg pulled off the SD Card
            drawSdJpeg("/op/a1/oppy.jpg", 200, 66);     // This draws a jpeg pulled off the SD Card
    }

 
  if (opportunity_status==2) {
     t2.disable();
     t1.disable();
     drawSdJpeg("/op/opprtunity.jpg", 0, 0);     // This draws a jpeg pulled off the SD Card


      Serial.printf("Sample MP3 playback begins...\n");

  // pno_cs from https://ccrma.stanford.edu/~jos/pasp/Sound_Examples.html
  file = new AudioFileSourceSD("/op/alarm.mp3");
  id3 = new AudioFileSourceID3(file);
  out = new AudioOutputI2S(0, 1); // Output to builtInDAC
  out->SetOutputModeMono(true);
  mp3 = new AudioGeneratorMP3();
  mp3->begin(id3, out);

  while (mp3->isRunning()) {
    if (!mp3->loop()) mp3->stop();
  }
  



     

    }

      
    }

 

 
 
  }
  




void t1Callback() {
//    Serial.print("t2: ");
//    Serial.println(millis());


  struct tm timeinfo;
   time_t now;
   time(&now);

   tft.setCursor(40, 0, 2);
  // Set the font colour to be white with a black background, set text size multiplier to 1
  tft.setTextColor(TFT_GREEN,TFT_BLACK);  tft.setTextSize(1);
   if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
   // return;
  }
  tft.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  
  
    
  tft.setTextColor(TFT_RED,TFT_BLACK);
  tft.setCursor(0, 18, 7);
  daysleft = (end_of_mission - now )/60/60/24; 

    // print days only if there is changes to reduce flicker
  if (daysleft!=daysleft_previo) {
        if (daysleft<10)  tft.print ("0");
        tft.print (daysleft);
        tft.print ("  "); 
  }
  daysleft_previo=daysleft;



   // hoursleft = (  end_of_mission - now)/60/60 - daysleft*24 ;
  hoursleft = 23-timeinfo.tm_hour;

   if (hoursleft!=hoursleft_previo) {    
    tft.setCursor(88, 18, 7);
    if (hoursleft<10)  tft.print ("0");
    tft.print (hoursleft);
    tft.print (":"); 
        
   }
  
  hoursleft_previo=hoursleft;
  
  
     
  //minleft =  ( end_of_mission - now)/60 - hoursleft*60 - daysleft*24*60 ;
  minleft=59-timeinfo.tm_min; 

  if (minleft!=minleft_previo) {
    tft.setCursor(240-(3*25), 18, 7);
    if (minleft<10)  tft.print ("0");
    tft.print (minleft);
    tft.print (":");
  }
  minleft_previo=minleft;

  

  tft.setCursor(240, 18, 7);
  if ((59-timeinfo.tm_sec)<10)  tft.print ("0");
  
  
  tft.println (59-timeinfo.tm_sec);
  

   tft.setTextColor(TFT_WHITE);
   tft.setCursor(61, 50, 2);
   tft.print ("days");
   tft.setCursor(97+55, 50, 2);
   tft.print ("h.");
   tft.setCursor(182+47, 50, 2);
   tft.print ("m.");
   tft.setCursor(258+46, 50, 2);
   tft.print ("s.");



  tft.setCursor(30, 223, 2);
  // Set the font colour to be white with a black background, set text size multiplier to 1
  tft.setTextColor(TFT_GREEN,TFT_BLACK);  tft.setTextSize(1);
 // tft.print ("En un lugar de la mancha de cuyo.");


 //delay(4500);
// printLocalTime();
// delay(1000);
// ParseData();


    
  
}





void t2Callback() {



//  delay_foto++;

//  if (delay_foto>10) {

//    delay_foto=0;
//    String random_string = "/op/"+ (String) (random(1000,1149))+".jpg";
//    drawSdJpeg("/op/"+ (String) (random(1000,1149))+".jpg", 40, 59);     // This draws a jpeg pulled off the SD Card
      char cadena[16];
      sprintf(cadena, "/op/%d.jpg", random(1000,1149));
      drawSdJpeg(cadena, 52, 64);     // This draws a jpeg pulled off the SD Card
//    drawSdJpeg(sprintf( "%d", delay_foto),40,60);

//}

    if (ticker_iteration == 2)   {
        ticker_iteration=0;
         opportunity_status=0;
         ParseData(); 
            }
    
}







int offset_sin = 0;
//String text0_sin = " "; // sample text


String   text_sin = " Oppy!!  Wake up!! ";

int textWidth;
float count = 1.0;



void t4Callback() {


// tft.setCursor(130, 70, 2);
// tft.print ("   Wake up!!!    Wake up Oppy!!!  ");
 
// routine to draw the actual wobbly text character by character, each with a sine wave. Based on Code by Joseph Rautenbach


  
/*
 * 

  textWidth = texto.length()*6+85; // get the width of the text

  for(int i=0; i<textWidth; i+=1) {
  count += 0.2;
    for(int j=0; j<texto.length(); j++) {
      tft.setCursor(130+(j*8), map(sin(count+(float)(j/1.5))*100, -100, 100, 80, 90),2);
      tft.print(texto[j]);
    }


  }

*/


const int width = 9; // width of the marquee display (in characters)


//  if (ticker_iteration==0)   {
//         text = text + "                                                 ";
//        ticker_iteration++;  
//         
//    }


if (offset_sin==0 ) {
  offset_sin=text_sin.length();
//  ticker_iteration_sin++;
  
}
//  Serial.println(ticker_iteration);
offset_sin--;




// Loop once through the string

//for (int offset = 0; offset < text.length(); offset++)

//{

// Construct the string to display for this iteration

String t = "";


//text_iteration++;
//if (text_iteration> width)  text_iteration=0;


for (int i =0; i < width ; i++) {
  
t += text_sin.charAt((offset_sin + i) % text_sin.length());

}
// Print the string for this iteration

//tft.setCursor(0, tft.height()/2-10); // display will be halfway down screen
// tft.setCursor(120, 110, 2);

 
//tft.print(t);


//  textWidth = t.length()*6+85; // get the width of the text

  tft.fillRect(120, 99, 75, 40, TFT_BLACK);
  tft.setTextColor(TFT_YELLOW,TFT_BLACK);
//  TFT_BLACK
  //   for(int i=0; i<textWidth; i+=1) {
    count += 0.2;
    for(int j=0; j<width  ; j++) {
      tft.setCursor(120+(j*8), map(sin(count+(float)(j/1.5))*100, -100, 100, 100, 120),2);  //this funtion is based in code by Joseph Rautenbach
 
      tft.print(t[j]);
    }

//     }





/*
 * 

  textWidth = texto.length()*6+85; // get the width of the text

  for(int i=0; i<textWidth; i+=1) {
  count += 0.2;
    for(int j=0; j<texto.length(); j++) {
      tft.setCursor(130+(j*8), map(sin(count+(float)(j/1.5))*100, -100, 100, 80, 90),2);
      tft.print(texto[j]);
    }


  }

*/









 // Short delay so the text doesn't move too fast

//delay(200);

//}









  

  
}



















int offset = 0;
String text = " "; // sample text


void t3Callback() {






const int width = 45; // width of the marquee display (in characters)


  if (ticker_iteration==0)   {
         text = text + "                                                 ";
        ticker_iteration++;  
         
    }


offset++;
if (offset> text.length())  {
  offset=0;
  ticker_iteration++;
  
}
//  Serial.println(ticker_iteration);




// Loop once through the string

//for (int offset = 0; offset < text.length(); offset++)

//{

// Construct the string to display for this iteration

String t = "";


//text_iteration++;
//if (text_iteration> width)  text_iteration=0;


for (int i = 0; i < width; i++) {
  
t += text.charAt((offset + i) % text.length());

}
// Print the string for this iteration

//tft.setCursor(0, tft.height()/2-10); // display will be halfway down screen
 tft.setCursor(5, 223, 2);
 tft.setTextColor(TFT_GREEN,TFT_BLACK);
//
tft.print(t);


 // Short delay so the text doesn't move too fast

//delay(200);

//}



    if (t3.isLastIteration()) {
//      t3.disable();
//      runner.deleteTask(t3);
//      t2.setInterval(500);
//      Serial.println("t1: disable t3 and delete it from the chain. t2 interval set to 500");
//      ParseData();     

    }


}








  


void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
  //  Serial.println("Failed to obtain time");
    return;
  }
  tft.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
   time_t now;
   time(&now);
  tft.println(now);
  tft.println(now/5);
  
}





void ParseData(void) {
  


if((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
//    USE_SERIAL.print("[HTTP] begin...\n");
        time_t now;
    time(&now);
    http.begin("https://"+ String(host) +"/dsn/data/dsn.xml?r=" + String(now/5)); //HTTP
    int httpCode = http.GET();
    USE_SERIAL.print("Connecting NASA...\n");
 
    if(httpCode > 0) {
//      USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
        if(httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
    //        USE_SERIAL.println(payload);

        text = " ";
        int inicio=0;
        
        while (payload.indexOf("<downSignal",inicio)>0) {
        int pos_ini = payload.indexOf("<downSignal",inicio);
        inicio = pos_ini +10 ;
        int pos_fin = payload.indexOf("/>",inicio);
    //    Serial.println(String("Inicio ") + inicio + " fin " + pos_fin);
    //    Serial.println("");
    //    Serial.println(payload.substring(inicio , pos_fin));
        xmlDataParser(payload.substring(inicio , pos_fin));
         
        }
                 http.end();
            }
        } else {
            USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
       }
    }
}
        


















void xmlDataParser(String payload) {

//    payload = payload.substring(pos_ini,pos_fin);

      int pos_ini = 0;
      int pos_fin = 0;
      float dataRate =0;
      String spacecraft ="";
      int spacecraftId=0;
      float power=0;
      float frecuency=0;
      
    
        if (payload.indexOf("spacecraft=")>0) {
                  int pos_ini= payload.indexOf("spacecraft=")+12;
                  int pos_fin= payload.indexOf("\"",pos_ini);
                  USE_SERIAL.print("Data from:");
           //       USE_SERIAL.print(String(payload.substring(pos_ini,pos_fin)));
                  spacecraft= String(payload.substring(pos_ini,pos_fin));
                  USE_SERIAL.print(" ");
                  USE_SERIAL.print(spacecraft);
            }
    
          
        if (payload.indexOf("spacecraftId=")>0) {
                  int pos_ini= payload.indexOf("spacecraftId=")+14;
                  int pos_fin= payload.indexOf("\"",pos_ini);
//                  USE_SERIAL.print(" id ");
//                  USE_SERIAL.print(String(payload.substring(pos_ini,pos_fin)));
                    spacecraftId= String(payload.substring(pos_ini,pos_fin)).toInt();

         //         if ((pos_fin-pos_ini)>1) then toInt()
                  
                  USE_SERIAL.print(" ");
                  USE_SERIAL.print(spacecraftId);
//                  naif_to_name(spacecraftId);
            }

            USE_SERIAL.println(" ");
            
            USE_SERIAL.print("naif:  ");
                        
            USE_SERIAL.println(naif_to_name(spacecraftId));

            USE_SERIAL.println(" ");
 
        if ((naif_to_name(spacecraftId)>0) & (naif_to_name(spacecraftId)<sizeof(naif))) {
          USE_SERIAL.println(naif_to_name(spacecraftId));
          spacecraft = naif_name[naif_to_name(spacecraftId)];
        }



        if (payload.indexOf("power=")>0) {
                  int pos_ini= payload.indexOf("power=")+7;
                  int pos_fin= payload.indexOf("\"",pos_ini);
                  USE_SERIAL.print(" with Power of ");
//                  USE_SERIAL.print(String(payload.substring(pos_ini,pos_fin)));
                  power = String(payload.substring(pos_ini,pos_fin)).toFloat();
                  USE_SERIAL.print(power);
                  USE_SERIAL.print(" dBm ");
                  
            }


        if (payload.indexOf("frequency=")>0) {
                  int pos_ini= payload.indexOf("frequency=")+11;
                  int pos_fin= payload.indexOf("\"",pos_ini);
                  USE_SERIAL.print(" on Frequency ");
      //            USE_SERIAL.print(String(payload.substring(pos_ini,pos_fin)));
                  frecuency = (String(payload.substring(pos_ini,pos_fin)).toFloat())/1000000000;
                   USE_SERIAL.print(frecuency);
                  USE_SERIAL.print(" GHz ");
        
            }


        if (payload.indexOf("dataRate=")>0) {
                  int pos_ini= payload.indexOf("dataRate=")+10;
                  int pos_fin= payload.indexOf("\"",pos_ini);
                  USE_SERIAL.print(" dataRate: ");
                  dataRate = String(payload.substring(pos_ini,pos_fin)).toFloat();
                  
                  if (((dataRate)>1000)&((dataRate)<1000000))   { 
                      USE_SERIAL.print(dataRate/1000);
                      USE_SERIAL.print(" Kbps ");
                    }

                  if ((dataRate)>1000000) { 
                      USE_SERIAL.print(dataRate/1000000);
                      USE_SERIAL.print(" Mbps ");
                    }

                      if ((dataRate)<1000){ 
                      USE_SERIAL.print(dataRate);
                      USE_SERIAL.print(" bps ");
                    }

                 // USE_SERIAL.print(String(payload.substring(pos_ini,pos_fin)));
     //             USE_SERIAL.print(" bps ");

            }

       if (payload.indexOf("signalType=")>0) {
                  int pos_ini= payload.indexOf("signalType=")+12;
                  int pos_fin= payload.indexOf("\"",pos_ini);
                  USE_SERIAL.print(" SignalType  ");
                  USE_SERIAL.print(String(payload.substring(pos_ini,pos_fin)));
                  USE_SERIAL.print(" ");
            }

       if (payload.indexOf("signalTypeDebug=")>0) {
                  int pos_ini= payload.indexOf("signalTypeDebug=")+17;
                  int pos_fin= payload.indexOf("\"",pos_ini);
//                  USE_SERIAL.print("signalTypedebug ");
                  USE_SERIAL.print(String(payload.substring(pos_ini,pos_fin)));
            }


    if (dataRate>0 ) {
      

          text = text + " -- Receiving " + spacecraft + " Power: " + String(power) +" dBm Freq: "+ String(frecuency) + " GHz Datarate: " ;

            if (((dataRate)>1000)&((dataRate)<1000000))   { 
                      text= text + String((dataRate/1000)) +" Kbps " ;
                     
                    }

                  if ((dataRate)>1000000) { 
                      text= text + String((dataRate/1000000)) +" Mbps " ;
//                      USE_SERIAL.print(dataRate/1000000);
//                      USE_SERIAL.print(" Mbps ");
                    }

                      if ((dataRate)<1000){ 
                        text= text + String((dataRate)) +" bps " ;
//                      USE_SERIAL.print(dataRate);
//                      USE_SERIAL.print(" bps ");
                    }


             // We are recieving data from Opportunity!!! It's alive!!!!
            if (spacecraftId==253 )     {
              opportunity_status=2; 
         }
                 
             
      } else {
        
         text = text + " -- Listening: " + spacecraft ;

             // if any DSN is wating info from the opportunity id253 chage the animation.
          if (spacecraftId==253 )     {
            opportunity_status=1;
              
         } 


         if (frecuency>0) {
            text = text +" on Freq: "+ String(frecuency) + "GHz";
         
         }
          
        
        
        } 


 USE_SERIAL.println(" ");
         USE_SERIAL.print(text);



            

USE_SERIAL.println("");

}




String xmlTakeParam(String inStr,String needParam)
{
  if(inStr.indexOf("<"+needParam+">")>0){
     int CountChar=needParam.length();
     int indexStart=inStr.indexOf("<"+needParam+">");
     int indexStop= inStr.indexOf("</"+needParam+">");  
     return inStr.substring(indexStart+CountChar+2, indexStop);
  }
  return "not found";
}
















/////////// Algoritmos JPG


//####################################################################################################
// Draw a JPEG on the TFT pulled from SD Card
//####################################################################################################
// xpos, ypos is top left corner of plotted image
//void drawSdJpeg(const char *filename, int xpos, int ypos) {

void drawSdJpeg(char *filename, int xpos, int ypos) {

  // Open the named file (the Jpeg decoder library will close it)
  File jpegFile = SD.open( filename, FILE_READ);  // or, file handle reference for SD library
 
  if ( !jpegFile ) {
    Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
    return;
  }

  Serial.println("===========================");
  Serial.print("Drawing file: "); Serial.println(filename);
  Serial.println("===========================");

  // Use one of the following methods to initialise the decoder:
  boolean decoded = JpegDec.decodeSdFile(jpegFile);  // Pass the SD file handle to the decoder,
  //boolean decoded = JpegDec.decodeSdFile(filename);  // or pass the filename (String or character array)

  if (decoded) {
    // print information about the image to the serial port
    jpegInfo();
    // render the image onto the screen at given coordinates
    jpegRender(xpos, ypos);
  }
  else {
    Serial.println("Jpeg file format not supported!");
  }
}

//####################################################################################################
// Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
//####################################################################################################
// This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
// fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void jpegRender(int xpos, int ypos) {

  //jpegInfo(); // Print information from the JPEG file (could comment this line out)

  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  bool swapBytes = tft.getSwapBytes();
  tft.setSwapBytes(true);
  
  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = min(mcu_w, max_x % mcu_w);
  uint32_t min_h = min(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // Fetch data from the file, decode and display
  while (JpegDec.read()) {    // While there is more data in the file
    pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)

    // Calculate coordinates of top left corner of current MCU
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // calculate how many pixels must be drawn
    uint32_t mcu_pixels = win_w * win_h;

    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
      tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    else if ( (mcu_y + win_h) >= tft.height())
      JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }

  tft.setSwapBytes(swapBytes);

  showTime(millis() - drawTime); // These lines are for sketch testing only
}

//####################################################################################################
// Print image information to the serial port (optional)
//####################################################################################################
// JpegDec.decodeFile(...) or JpegDec.decodeArray(...) must be called before this info is available!
void jpegInfo() {

  // Print information extracted from the JPEG file
  Serial.println("JPEG image info");
  Serial.println("===============");
  Serial.print("Width      :");
  Serial.println(JpegDec.width);
  Serial.print("Height     :");
  Serial.println(JpegDec.height);
  Serial.print("Components :");
  Serial.println(JpegDec.comps);
  Serial.print("MCU / row  :");
  Serial.println(JpegDec.MCUSPerRow);
  Serial.print("MCU / col  :");
  Serial.println(JpegDec.MCUSPerCol);
  Serial.print("Scan type  :");
  Serial.println(JpegDec.scanType);
  Serial.print("MCU width  :");
  Serial.println(JpegDec.MCUWidth);
  Serial.print("MCU height :");
  Serial.println(JpegDec.MCUHeight);
  Serial.println("===============");
  Serial.println("");
}

//####################################################################################################
// Show the execution time (optional)
//####################################################################################################
// WARNING: for UNO/AVR legacy reasons printing text to the screen with the Mega might not work for
// sketch sizes greater than ~70KBytes because 16 bit address pointers are used in some libraries.

// The Due will work fine with the HX8357_Due library.

void showTime(uint32_t msTime) {
  //tft.setCursor(0, 0);
  //tft.setTextFont(1);
  //tft.setTextSize(2);
  //tft.setTextColor(TFT_WHITE, TFT_BLACK);
  //tft.print(F(" JPEG drawn in "));
  //tft.print(msTime);
  //tft.println(F(" ms "));
  Serial.print(F(" JPEG drawn in "));
  Serial.print(msTime);
  Serial.println(F(" ms "));
}


/// Algoritmos JPG












