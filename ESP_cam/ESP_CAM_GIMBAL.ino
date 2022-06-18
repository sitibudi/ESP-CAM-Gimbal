//============================================================LIBRARY=====================================================
#include <WiFi.h>
#include <ESP32Servo.h> 
#include <FirebaseESP32.h>
#include "esp_camera.h"

//============================================================DONE=====================================================

//==========================================================INITIALIZATION=====================================================
// ===================
// Select camera model
// ===================
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM

#include "camera_pins.h"

//================================
const char* ssid = "Bro-Bor";
const char* password = "9434276267";
//================================

WiFiServer server(80);
bool connected = false;
WiFiClient live_client;

// setup html page 
String index_html = "<meta charset=\"utf-8\"/>\n" \
                    "<style>\n" \
                    "#content {\n" \
                    "display: flex;\n" \
                    "flex-direction: column;\n" \
                    "justify-content: center;\n" \
                    "align-items: center;\n" \
                    "text-align: center;\n" \
                    "min-height: 100vh;}\n" \
                    "</style>\n" \
                    "<body bgcolor=\"#000000\"><div id=\"content\"><h2 style=\"color:#ffffff\"></h2><img src=\"video\"></div></body>";




Servo servo0; 
int jarakb = 15; // atur jarak yang diinginkan untuk mengaktifkan sensor ultrasonic belakang
int jarakf = 15; // atur jarak yang diinginkan untuk mengaktifkan sensor ultrasonic depan
int dly = 5000; // nilai delay yang digunakan untuk mengatur jeda putaran servo ke bagian belakang
// define Front Ultrasnoic
#define echof 14
#define trigf 2
long durationf,distancef ;
bool depan = false;

// define Back Ultrasnoic
#define echob 15
#define trigb 13
long durationb,distanceb ;
bool belakang = false;

//rotate
String rot;
int rot1;
String security; // read firebase 

// ===================
// Firebase setup
// ===================
#define DATABASE_URL "my-app-30024-default-rtdb.firebaseio.com/" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app
#define DATABASE_SECRET "ELzrulkWhlXHGoULahavaYHIPATNHqp955X5fhgM"
/* Define the Firebase Data object */
FirebaseData fbdo;

/* Define the FirebaseAuth data for authentication data */
FirebaseAuth auth;

/* Define the FirebaseConfig data for config data */
FirebaseConfig config;



//============================================================DONE=====================================================


//===========================================================FUNCTION=====================================================

//continue sending camera frame
void liveCam(WiFiClient &client){
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
      Serial.println("Frame buffer could not be acquired");
      return;
  }
  client.print("--frame\n");
  client.print("Content-Type: image/jpeg\n\n");
  client.flush();
  client.write(fb->buf, fb->len);
  client.flush();
  client.print("\n");
  esp_camera_fb_return(fb);
}

// setup http for camera web page
void http_resp(){
  WiFiClient client = server.available();                          
  if (client.connected()) {    
      String req = "";
      while(client.available()){
        req += (char)client.read();
      }
      Serial.println("request " + req);
      int addr_start = req.indexOf("GET") + strlen("GET");
      int addr_end = req.indexOf("HTTP", addr_start);
      if (addr_start == -1 || addr_end == -1) {
          Serial.println("Invalid request " + req);
          return;
      }
      req = req.substring(addr_start, addr_end);
      req.trim();
      Serial.println("Request: " + req);
      client.flush();
 
      String s;
      if (req == "/")
      {
          s = "HTTP/1.1 200 OK\n";
          s += "Content-Type: text/html\n\n";
          s += index_html;
          s += "\n";
          client.print(s);
          client.stop();
      }
      else if (req == "/video")
      {
          live_client = client;
          live_client.print("HTTP/1.1 200 OK\n");
          live_client.print("Content-Type: multipart/x-mixed-replace; boundary=frame\n\n");
          live_client.flush();
          connected = true;
      }
      else
      {
          s = "HTTP/1.1 404 Not Found\n\n";
          client.print(s);
          client.stop();
      }
    }      
}

// function for ultrasonic setup pin mode
void ultrasonicsetup(){   
  pinMode(trigf,OUTPUT);
  pinMode(echof,INPUT);
  pinMode(trigb,OUTPUT);
  pinMode(echob,INPUT);
  
}
// function to activate controlling rotation servo from firebase value
void rotate(){
  
  Firebase.getString(fbdo,"/ESP_Cam/rotate",rot);
  rot = fbdo.stringData();
  rot1 = rot.toInt();
  servo0.write(rot1);
  
}

// function to activate front ultrasonic 
void frontsystem(){
  depan = false;
  digitalWrite(trigf,LOW);
  delayMicroseconds(2);
  digitalWrite(trigf,HIGH);
  delayMicroseconds(10);
  digitalWrite(trigf,LOW);
  durationf = pulseIn(echof,HIGH); // get the pulse of the ultrasonic trigger and echo
  
  distancef = durationf / 58.2; // formula to convert ulrasonic pulse to distance value
//  Serial.println(distancef); // to print distance value

  if(distancef <jarakf){  //jika mendeteksi jarak yang ditentukan maka arduino mengirim 
  depan = false;
  int nilai = 1; // angka 3 yang ke esp yang artinya ada benda/orang yang terdeteksi di mobil
  //Firebase.setInt(dataObject,"path",target)
  Firebase.setInt(fbdo,"/ESP_Cam/alarm",nilai);  // send data to firebase 

  }
  
  else{ //jika tidak maka arduino mengirim angka 0 yang artinya tidak ada benda yang terdeteksi
  int nilai = 0;
  Firebase.setInt(fbdo,"/ESP_Cam/alarm",nilai);
  depan = true;
  }
}

//function to activate back ultrasonic 
void backsystem(){
  belakang = false;
   digitalWrite(trigb,LOW);
  delayMicroseconds(2);
  digitalWrite(trigb,HIGH);
  delayMicroseconds(10);
  digitalWrite(trigb,LOW);
  durationb = pulseIn(echob,HIGH);
  
  distanceb = durationb / 58.2;
  Serial.println(distanceb);
  if(distanceb < jarakb){
    belakang = false;
  int nilai = 1;
  Firebase.setInt(fbdo,"/ESP_Cam/alarm",nilai);
  }
  else{
    belakang = true;
    int nilai = 0;
    Firebase.setInt(fbdo,"/ESP_Cam/alarm",nilai);

  }

  
}

// function for setup esp32 cam
void espsetup(){
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_VGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // settings camera
  sensor_t * s = esp_camera_sensor_get();
  s->set_vflip(s, 1);          // 0 = disable , 1 = enable vertical flip
  
//===========================WIFI ACCESS POINT============================================
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Camera Ready! Use 'http://");
  String IP = WiFi.localIP().toString();
  Serial.print(IP);
  Serial.println("' to connect"+IP);
  index_html.replace("server_ip",IP );
  server.begin();
 
 //==============================DONE======================================================
}

// function for setup and connect firebase 
void firebaseSetup(){
  //========================Setup FIREBASE realtime database======================
    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    /* Assign the certificate file (optional) */
    //config.cert.file = "/cert.cer";
    //config.cert.file_storage = StorageType::FLASH;

    /* Assign the database URL and database secret(required) */
    config.database_url = DATABASE_URL;
    config.signer.tokens.legacy_token = DATABASE_SECRET;

    Firebase.reconnectWiFi(true);

    /* Initialize the library with the Firebase authen and config */
    Firebase.begin(&config, &auth);
//     Firebase.begin(DATABASE_URL,DATABASE_SECRET);
    
  Serial.setDebugOutput(true);
  Serial.println();
  
}
//============================================================DONE=====================================================

//==========================================================MAIN PROGRAM=====================================================
void setup() {
  Serial.begin(115200);
  ///call all the function setup
  ultrasonicsetup(); 
  espsetup();
  firebaseSetup();
  servo0.attach(12); // servo attach to pin GPIO 12 in ESP32 CAM
  servo0.write(0); // first thing to do is set the servo to 0 degree position
}

void loop() {
  // running live camera 
    http_resp();
  if(connected == true){
    liveCam(live_client);
  }
  
//  =========================SECURITY LOGIC=====================================
  Firebase.getString(fbdo,"/ESP_Cam/security",security); // firebase get the string from firebase from a spesific path
 security = fbdo.stringData(); // move string data from firebase reading to the provided variable
// Serial.println(security); // print firebase data to serial monitor
  if(security == "1"){ // jika security == 1 atau keamanan diaktifkan maka akan memanggil function untuk mengaktifkan sistem keamanan depan dan belakang
  backsystem(); 
  frontsystem();
  rotate();
  
  }
  else{
  int nilai = 0;
  Firebase.setInt(fbdo,"/ESP_Cam/alarm",nilai);

  rot = "0";
  Firebase.setString(fbdo,"/ESP_Cam/rotate",rot);
  

  
}
 
}
