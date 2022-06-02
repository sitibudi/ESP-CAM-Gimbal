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
// ** Espressif Internal Boards **
//#define CAMERA_MODEL_ESP32_CAM_BOARD
//#define CAMERA_MODEL_ESP32S2_CAM_BOARD
//#define CAMERA_MODEL_ESP32S3_CAM_LCD

#include "camera_pins.h"

//================================
const char* ssid = ""; // fill data according to the name of the ssid
const char* password = ""; // fill data according to the name of the password ssid
//================================



Servo servo0; 
int jarakb = 15; // atur jarak yang diinginkan untuk mengaktifkan sensor ultrasonic belakang
int jarakf = 15; // atur jarak yang diinginkan untuk mengaktifkan sensor ultrasonic depan
int dly = 5000; // nilai delay yang digunakan untuk mengatur jeda putaran servo ke bagian belakang
// define Front Ultrasnoic
#define echof 14
#define trigf 2
long durationf,distancef ;

// define Back Ultrasnoic
#define echob 15
#define trigb 13
long durationb,distanceb ;


// ===================
// Firebase setup
// ===================
#define DATABASE_URL " " //fill data according to the database URL from firebase
#define DATABASE_SECRET " " // fill data according to the database secret from firebase
/* Define the Firebase Data object */
FirebaseData fbdo;

/* Define the FirebaseAuth data for authentication data */
FirebaseAuth auth;

/* Define the FirebaseConfig data for config data */
FirebaseConfig config;

String security; // read firebase 

//============================================================DONE=====================================================


//===========================================================FUNCTION=====================================================
void startCameraServer();

// function for ultrasonic setup pin mode
void ultrasonicsetup(){   
  pinMode(trigf,OUTPUT);
  pinMode(echof,INPUT);
  pinMode(trigb,OUTPUT);
  pinMode(echob,INPUT);
  
}

// function to activate front ultrasonic 
void frontsystem(){
  digitalWrite(trigf,LOW);
  delayMicroseconds(2);
  digitalWrite(trigf,HIGH);
  delayMicroseconds(10);
  digitalWrite(trigf,LOW);
  durationf = pulseIn(echof,HIGH); // get the pulse of the ultrasonic trigger and echo
  
  distancef = durationf / 58.2; // formula to convert ulrasonic pulse to distance value
//  Serial.println(distancef); // to print distance value

  if(distancef <jarakf){  //jika mendeteksi jarak yang ditentukan maka arduino mengirim 
  int tiga = 3; // angka 3 yang ke esp yang artinya ada benda/orang yang terdeteksi di mobil
  //Firebase.setInt(dataObject,"path",target)
  Firebase.setInt(fbdo,"/ESP_Cam/alarm",tiga);  // send data to firebase 

  }
  
  else{ //jika tidak maka arduino mengirim angka 2 yang artinya tidak ada benda yang terdeteksi
  int dua = 2;
  Firebase.setInt(fbdo,"/ESP_Cam/alarm",dua);
  }
}

//function to activate back ultrasonic 
void backsystem(){
   digitalWrite(trigb,LOW);
  delayMicroseconds(2);
  digitalWrite(trigb,HIGH);
  delayMicroseconds(10);
  digitalWrite(trigb,LOW);
  durationb = pulseIn(echob,HIGH);
  
  distanceb = durationb / 58.2;
//  Serial.println(distanceb);
  if(distanceb < jarakb){

  int tiga = 3;
  Firebase.setInt(fbdo,"/ESP_Cam/alarm",tiga);
    servo0.write(180); // servo memutar 180 derajat 
    delay(dly); // delay digunakan untuk menahan servo disudut 180 derajat dengan jeda waktu yang ditentukan
  }
  else{
    int dua = 2;
    Firebase.setInt(fbdo,"/ESP_Cam/alarm",dua);
    servo0.write(0);  //servo kembali ke posisi awal (0) jika tidak sensor tidak mendeteksi apa-apa
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
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if(config.pixel_format == PIXFORMAT_JPEG){
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  s->set_vflip(s, 1);
#endif
//===========================WIFI ACCESS POINT============================================
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
//===========================ESP ACCESS POINT============================================
//  WiFi.softAP(ssid,password);
//  IPAddress IP = WiFi.softAPIP();
//  Serial.print("AP IP address:");
//  Serial.println(IP);
//==============================DONE======================================================
  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
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
//  =========================SECURITY LOGIC=====================================
  Firebase.getString(fbdo,"/ESP_Cam/security",security); // firebase get the string from firebase from a spesific path
 security = fbdo.stringData(); // move string data from firebase reading to the provided variable
// Serial.println(security); // print firebase data to serial monitor
if(security == "1"){ // jika security == 1 atau keamanan diaktifkan maka akan memanggil function untuk mengaktifkan sistem keamanan depan dan belakang
 backsystem(); 
 frontsystem();
}

}
