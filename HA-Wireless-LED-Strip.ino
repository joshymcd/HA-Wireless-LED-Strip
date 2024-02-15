#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"
#include "WIFIConfigs.h"

/************ WiFI ******************/
const char* WIFI_SSID = CONF_WIFI_SSID ;//"USE A Config file or replace ";
const char* WIFI_PASSWORD = CONF_WIFI_PASSWORD  ;//"5c7bd5e5fc"; 

/************ FastLED ******************/
#define DATA_PIN    D4 //on the NodeMCU 1.0, FastLED will default to the D5 pin after throwing an error during compiling. Leave as is. 
#define LED_TYPE    WS2812 //change to match your LED type
#define COLOR_ORDER GRB //change to match your LED configuration
#define NUM_LEDS    60 //change to match your setup


/************ MQTT Server ******************/
#define mqtt_server "192.168.1.1" 
#define mqtt_port 1883
#define mqtt_user "pi" 
#define mqtt_password "letmein" 


/************ MQTT Topics ******************/
#define setpowersub "ledstrip/setpower"
#define setpowerpub "ledstrip/setpowerpub"

#define setcolorsub "ledstrip/setcolour"
#define setcolorpub "ledstrip/setcolourpub"

#define setbrightness "ledstrip/setbrightness"
#define setbrightnesspub "ledstrip/setbrightnesspub"

//#define colorstatuspub "ledstrip/colourstatus"
//#define seteffectsub "ledstrip/seteffect"
//#define seteffectpub "ledstrip/seteffectpub"
//#define setanimationspeed "ledstrip/setanimationspeed"


/************ Vars ******************/
CRGB leds[NUM_LEDS];
WiFiClient espClient;
PubSubClient client(espClient);
String Message;
String Topic;
String Effect; 

//default state
int Rcolor = 255;
int Gcolor = 255;
int Bcolor = 255;
int Brightness = 255;
String Power = "ON";

//
bool ColourUpdateNeeded;
bool BrightnessUpdateNeeded;





void setup() {
  ColourUpdateNeeded = true; 
  Serial.begin(115200);
  //Setup LEDS
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS); 
  setColour();
  delay(3000);
  
  setup_scroll(255,0,0); 
  connectToWIFI();
  setup_scroll(0,0,255);
  connectToMQTT();
  setup_scroll(0,255,0);
  delay(3000);
  setup_scroll(255,255,255); 
}

void loop() {    
  //check wifi
  if (WiFi.status() != WL_CONNECTED) {
    connectToWIFI();
  }

  if (!client.connected()) {
    Serial.println("MQTT Disconnected ");
    connectToMQTT();
  }
  
  client.loop(); 
  updateStrip();   
}

void MQTTCallback(char* _topic, byte* _payload, unsigned int _length) {
  int i = 0;
  char message_buff[100];
  Serial.print("Topic: ");
  Serial.println(_topic);

  for (i = 0; i < _length; i++) {
    message_buff[i] = _payload[i];
  }
  message_buff[i] = '\0'; 

  Message = String(message_buff);
  Topic = String(_topic);
  
  Serial.print("Message: ");
  Serial.println(Message);
  
  if (Topic == setpowersub){
      Effect = "Solid";
      Power = Message;
      ColourUpdateNeeded = true;
  } 
  if (Topic == setcolorsub){
    Effect = "Solid";
    ColourUpdateNeeded = true;
    Rcolor = Message.substring(0, Message.indexOf(',')).toInt();
    Gcolor = Message.substring(Message.indexOf(',') + 1, Message.lastIndexOf(',')).toInt();
    Bcolor = Message.substring(Message.lastIndexOf(',') + 1).toInt();
    Serial.println("Setting colour to: ");
    Serial.print(Rcolor);
    Serial.print(" , ");
    Serial.print(Gcolor);
    Serial.print(" , ");
    Serial.println(Bcolor);
  }
  if(Topic == setbrightness){ 
    Brightness = Message.toInt(); 
  }
    //clearMQTTVars();
}


void updateStrip(){ 
  //Solid colour changes
  if(Effect == "Solid" && ColourUpdateNeeded){
    if (Power == "OFF"){
      client.publish(setpowerpub, "OFF");
      light_off();
      ColourUpdateNeeded = false;
      exit;
    } else {
      client.publish(setpowerpub, "ON");  
      //fill_solid(leds, NUM_LEDS, CRGB(Rcolor, Gcolor, Bcolor));
      Serial.println("qqqqqqqqqqqqqqqqqqqqqqqqqq");
      setColour();
      Serial.println("wwwwwwwwwwwwwwwwwwwwwwwwww");
      String currentColour = String(Rcolor) + "," + String(Gcolor) + "," + String(Bcolor);
      char tab2[1024];
      strcpy(tab2, currentColour.c_str()); 
      client.publish(setcolorpub,tab2 );
      FastLED.show();
      fadeToBrightness(Brightness);
      ColourUpdateNeeded = false; 
      exit;
    }     
  }

  
 
  
  //if(Brightness != FastLED.getBrightness()){
  //  Serial.println("Brightnesses not equal");
  //  fadeToBrightness(Brightness); 
  //}
   
}

 



void connectToWIFI(){ 
  delay(2000);
  Serial.println();
  WiFi.disconnect();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  Serial.println(WIFI_PASSWORD);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("."); 
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
}

void connectToMQTT(){ 
  delay(3000);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(MQTTCallback);  
  while (!client.connected()) {
    FastLED.show(); 
    Serial.print("Attempting MQTT connection... "); 
    if (client.connect("LED_Lights", mqtt_user, mqtt_password)) {
      break; 
    } else {
      Serial.println("failed...");
      delay(5000);
    }
  } 

  if (!client.connected()){ 
    connectToMQTT();
  }else{
    Serial.println("Connected");
    SubscribeToMQTT();
  }
}

void SubscribeToMQTT(){
  client.subscribe(setpowersub); 
  //client.subscribe(setpowerpub); 
  
  client.subscribe(setcolorsub); 
  //client.subscribe(setcolorpub); 
  
  client.subscribe(setbrightness); 
  //client.subscribe(setbrightnesspub); 
  publishCurrent();
}

void setup_scroll(int r, int g, int b){
  for(int dot = 0; dot < NUM_LEDS; dot++) { 
    leds[dot].red = r;
    leds[dot].green = g;
    leds[dot].blue = b;
    FastLED.show(); 
    delay(50);
  }
  
  Rcolor = r;
  Gcolor = g;
  Bcolor = b;
  
  String currentColour = String(r) + "," + String(g) + "," + String(b);
  char tab2[1024];
  strcpy(tab2, currentColour.c_str()); 
  client.publish(setcolorpub,tab2 );
}

void light_off(){
 int delay_amt;
 delay_amt = 15;

      
  int currentb;
  currentb = (int) FastLED.getBrightness(); 
  Serial.print("Fading from ");
  
  if (currentb > 0){ 
    while( currentb != 0 ) {
      currentb = currentb - 1; 
      FastLED.setBrightness(currentb);
      FastLED.show();
      delay(delay_amt); 
    }
  }
}

void fadeToBrightness(int brightness){
 int delay_amt;
 delay_amt = 15;

      
  int currentb;
  currentb = (int) FastLED.getBrightness(); 
  Serial.print("Fading from ");
  Serial.print(currentb);
  Serial.print(" to "); 
  Serial.println(brightness); 
  
  if (currentb > brightness){ 
    while( currentb != brightness ) {
      currentb = currentb - 1; 
      FastLED.setBrightness(currentb);
      FastLED.show();
      delay(delay_amt); 
    }
  }
  if (currentb < brightness){ 
    while( currentb != brightness ) {
      currentb = currentb + 1; 
      FastLED.setBrightness(currentb);
      FastLED.show();
      delay(delay_amt);
      
    }
  }
}

/*{
  int red_diff   = out.r() - in.r();
  int green_diff = out.g() - in.g();
  int blue_diff  = out.b() - in.b();
  for ( unsigned i = 0; i < n_steps; ++i){
    /* output is the color that is actually written to the pins
     * and output nicely fades from in to out.
     */
/*    rgb_color output ( in.r() + i * red_diff / n_steps,
                       in.g() + i * green_diff / n_steps,
                       in.b() + i * blue_diff/ n_steps);
    /*put the analog pins to the proper output.*/
 /*   analogWrite( r_pin, output.r() );
    analogWrite( g_pin, output.g() );
    analogWrite( b_pin, output.b() );
    delay(time);*/
//  }
//}

void setColour(){
  int numOfSteps = 100;
  int _delay = 40;
  double red_diff = 0;
  double green_diff = 0;
  double blue_diff = 0;
  double bright_diff = 0;
  int origBrightness  = FastLED.getBrightness();
  //Create Copy Of led Array Object
  CRGB CopyOfleds[NUM_LEDS];
  for(int dot = 0; dot < NUM_LEDS; dot++) { 
      CopyOfleds[dot] = leds[dot]; 
  }
 Serial.print("Solid colour fade start: ");

      Serial.print("Current [0]: ");
      Serial.print(leds[0].red);
      Serial.print(" , ");
      Serial.print(leds[0].green);
      Serial.print(" , ");
      Serial.println(leds[0].blue);

      Serial.print("Target: ");
      Serial.print(Rcolor);
      Serial.print(" , ");
      Serial.print(Gcolor);
      Serial.print(" , ");
      Serial.println(Bcolor); 

      Serial.print("DIFF: ");
      Serial.print(Rcolor - CopyOfleds[0].red);
      Serial.print(" , ");
      Serial.print(Gcolor - CopyOfleds[0].green);
      Serial.print(" , ");
      Serial.println(Bcolor - CopyOfleds[0].blue); 
 
      
  for ( int i = 1; i < numOfSteps+1; ++i){
    Serial.print("Starting Step ");
    Serial.print(i);
    Serial.print(" of ");
    Serial.println(numOfSteps); 
    
    for(int dot = 0; dot < NUM_LEDS; dot++) {  
      red_diff   = Rcolor - CopyOfleds[dot].red;
      green_diff   = Gcolor - CopyOfleds[dot].green;
      blue_diff   = Bcolor - CopyOfleds[dot].blue; 
      bright_diff = Brightness - origBrightness;
       
      leds[dot].red = CopyOfleds[dot].red +( i * (red_diff / numOfSteps));
      leds[dot].green = CopyOfleds[dot].green +( i * (green_diff / numOfSteps));
      leds[dot].blue = CopyOfleds[dot].blue +( i * (blue_diff / numOfSteps));
      FastLED.setBrightness(origBrightness +( i * (bright_diff / numOfSteps))); 
    }

    Serial.print("New [0]: ");
    Serial.print(leds[0].red);
    Serial.print(" , ");
    Serial.print(leds[0].green);
    Serial.print(" , ");
    Serial.print(leds[0].blue);
    Serial.print(" : ");
    Serial.println(FastLED.getBrightness());
    delay(_delay);
    FastLED.show(); 
  }
  Serial.print("Target was: ");
  Serial.print(Rcolor);
  Serial.print(" , ");
  Serial.print(Gcolor);
  Serial.print(" , ");
  Serial.println(Bcolor); 
  Serial.print("Colour set to: "); 
  Serial.print(leds[0].red);
  Serial.print(" , ");
  Serial.print(leds[0].green);
  Serial.print(" , ");
  Serial.print(leds[0].blue);
  Serial.print(" : ");
  Serial.println(FastLED.getBrightness());
  
  //String S_COL = String(Rcolor) + "," + String(Gcolor) + "," + String(Bcolor);
  //char C_COL[1024];
  //strcpy(C_COL, S_COL.c_str()); 
  //client.publish(setcolorpub,C_COL );
  publishCurrent();
}


void publishCurrent(){
  Serial.println("Publishing current state");
  
  String S_BRIGHT = String(FastLED.getBrightness());
  char C_BRIGHT[1024];
  strcpy(C_BRIGHT, S_BRIGHT.c_str());
  client.publish(setbrightnesspub,C_BRIGHT );


  String S_COL = String(Rcolor) + "," + String(Gcolor) + "," + String(Bcolor);
  char C_COL[1024];
  strcpy(C_COL, S_COL.c_str()); 
  client.publish(setcolorpub,C_COL );

  String S_POW = Power;
  char C_POW[1024];
  strcpy(C_POW, S_POW.c_str()); 
  client.publish(setcolorpub,C_POW );
}

void clearMQTTVars(){
  Message = "";
  Topic = "";
  Effect ="";
}
