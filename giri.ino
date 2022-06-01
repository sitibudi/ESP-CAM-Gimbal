/*
  DIY Gimbal - MPU6050 Arduino Tutorial
  by Dejan, www.HowToMechatronics.com
  Code based on the MPU6050_DMP6 example from the i2cdevlib library by Jeff Rowberg:
  https://github.com/jrowberg/i2cdevlib
*/
//==================================LIBRARY==============================
// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"

#include "MPU6050_6Axis_MotionApps20.h"
//#include "MPU6050.h" // not necessary if using MotionApps include file

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif
#include <Servo.h>
String ON[1];
// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for SparkFun breakout and InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 mpu;
//MPU6050 mpu(0x69); // <-- use for AD0 high

//===================================DONE=================================
//==================================INITIALIZATION==============================
// Define the 3 servo motors
Servo servo0;
Servo servo1;
Servo servo2;
float correct;
int j = 0;
int dly = 5000; 
int jarak = 15; // variabel untuk menentukan nilai jarak yang akan mengaktifkan sistem security
//ultrasonic front 
#define echof 4
#define trigf 5
long durationf,distancef ;
//ultrasonic back
#define echob 6
#define trigb 7
long durationb,distanceb ;
// define TX RX variable
String security = "";
String front = "";
String back = "";
#define OUTPUT_READABLE_YAWPITCHROLL

#define INTERRUPT_PIN 2  // use pin 2 on Arduino Uno & most boards

bool blinkState = false;

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

// packet structure for InvenSense teapot demo
uint8_t teapotPacket[14] = { '$', 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0x00, '\r', '\n' };

//===================================DONE=================================

// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() {
  mpuInterrupt = true;
}


//==================================FUNCTION==============================
// Gimbal running
void gimbal(){
  
  // if programming failed, don't try to do anything
  if (!dmpReady) return;

  // wait for MPU interrupt or extra packet(s) available
  while (!mpuInterrupt && fifoCount < packetSize) {
    if (mpuInterrupt && fifoCount < packetSize) {
      // try to get out of the infinite loop
      fifoCount = mpu.getFIFOCount();
    }
  }

  // reset interrupt flag and get INT_STATUS byte
  mpuInterrupt = false;
  mpuIntStatus = mpu.getIntStatus();

  // get current FIFO count
  fifoCount = mpu.getFIFOCount();

  // check for overflow (this should never happen unless our code is too inefficient)
  if ((mpuIntStatus & _BV(MPU6050_INTERRUPT_FIFO_OFLOW_BIT)) || fifoCount >= 1024) {
    // reset so we can continue cleanly
    mpu.resetFIFO();
    fifoCount = mpu.getFIFOCount();
    Serial.println(F("FIFO overflow!"));

    // otherwise, check for DMP data ready interrupt (this should happen frequently)
  } else if (mpuIntStatus & _BV(MPU6050_INTERRUPT_DMP_INT_BIT)) {
    // wait for correct available data length, should be a VERY short wait
    while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

    // read a packet from FIFO
    mpu.getFIFOBytes(fifoBuffer, packetSize);

    // track FIFO count here in case there is > 1 packet available
    // (this lets us immediately read more without waiting for an interrupt)
    fifoCount -= packetSize;

    // Get Yaw, Pitch and Roll values
#ifdef OUTPUT_READABLE_YAWPITCHROLL
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

    //Pitch, Roll values - Radians to degrees
//    ypr[0] = ypr[0] * 180 / M_PI;
    ypr[1] = ypr[1] * 180 / M_PI;
    ypr[2] = ypr[2] * 180 / M_PI;
//    Serial.println(ypr[0]);
//    Serial.println(ypr[1]);
    // Skip 300 readings (self-calibration process)
    if (j <= 300) {
      correct = ypr[0]; // Yaw starts at random value, so we capture last value after 300 readings
      j++;
    }
    // After 300 readings
    else {
      ypr[0] = ypr[0] - correct; // Set the Yaw to 0 deg - subtract  the last random Yaw value from the currrent value to make the Yaw 0 degrees
      // Map the values of the MPU6050 sensor from -90 to 90 to values suatable for the servo control from 0 to 180
//      int servo0Value = map(ypr[0], -90, 90, 0, 180);
      int servo1Value = map(ypr[1], 90, -90, 0, 180); //ROLL
      int servo2Value = map(ypr[2], -90, 90, 180, 0); //PITCH
      
      // Control the servos according to the MPU6050 orientation
//      servo0.write(servo0Value);
      servo1.write(servo1Value-10);
      servo2.write(servo2Value-6);
      
    }
#endif
  }
}
// setup pin for both ultrasonic sensor
void ultrasonicsetup(){   
  pinMode(trigf,OUTPUT);
  pinMode(echof,INPUT);
  pinMode(trigb,OUTPUT);
  pinMode(echob,INPUT);
  
}

// function menjalankan ultrasonic bagian depan
void frontsystem(){
  digitalWrite(trigf,LOW);
  delayMicroseconds(2);
  digitalWrite(trigf,HIGH);
  delayMicroseconds(10);
  digitalWrite(trigf,LOW);
  durationf = pulseIn(echof,HIGH);
  
  distancef = durationf / 58.2;
  if(distancef <jarak){  //jika mendeteksi jarak yang ditentukan maka arduino mengirim 
    ON[1]='3';                 // angka 3 yang ke esp yang artinya ada benda/orang yang terdeteksi di mobil
//    Serial.println(ON[1]); 
Serial.println('3');  
  }
  else{
    ON[0]='2'; 
//    Serial.println(ON[0]);   //jika tidak maka arduino mengirim angka 2 yang artinya tidak ada benda yang terdeteksi
  Serial.println('2');
  }
  
  
}

//function menjalankan ultrasonic bagian belakang
void backsystem(){
   digitalWrite(trigb,LOW);
  delayMicroseconds(2);
  digitalWrite(trigb,HIGH);
  delayMicroseconds(10);
  digitalWrite(trigb,LOW);
  durationb = pulseIn(echob,HIGH);
  
  distanceb = durationb / 58.2;
  if(distanceb < jarak){
    ON[1]='3'; 
//    Serial.println(ON[1]);
    Serial.println('3');
    servo0.write(180);
    
    
  }
  else{
    ON[0] = '2';
//    Serial.println(ON[0]);
Serial.println('2');
    servo0.write(0);
  }

  
}


//===================================DONE=================================

// ================================================================
// ===                      INITIAL SETUP                       ===
// ================================================================

void setup() {
  // join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
  Wire.begin();
  Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
  Fastwire::setup(400, true);
#endif

  // initialize serial communication
  // (115200 chosen because it is required for Teapot Demo output, but it's
  // really up to you depending on your project)
//  Serial.begin(38400);
  Serial.begin(9600);
  while (!Serial); // wait for Leonardo enumeration, others continue immediately

  // initialize device
  //Serial.println(F("Initializing I2C devices..."));
  mpu.initialize();
  pinMode(INTERRUPT_PIN, INPUT);
  ultrasonicsetup();
  devStatus = mpu.dmpInitialize();
  
  mpu.setXGyroOffset(17); //17
  mpu.setYGyroOffset(-69);  //-69
  mpu.setZGyroOffset(27);
  mpu.setZAccelOffset(1688); 

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
    // turn on the DMP, now that it's ready
     Serial.println(F("Enabling DMP..."));
    mpu.setDMPEnabled(true);

    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
    mpuIntStatus = mpu.getIntStatus();

    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    Serial.println(F("DMP ready! Waiting for first interrupt..."));
    dmpReady = true;

    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
  } else {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    // Serial.print(F("DMP Initialization failed (code "));
    //Serial.print(devStatus);
    //Serial.println(F(")"));
  }

  // Define the pins to which the 3 servo motors are connected
  servo0.attach(10); //YAW
  servo1.attach(9); //ROLL
  servo2.attach(8); //PITCH

  servo0.write(0);
}
// ================================================================
// ===                    MAIN PROGRAM LOOP                     ===
// ================================================================

void loop() {
  
  gimbal(); //memanggil fungsi gimbal yang berisi perintah untuk melakukan stabilizer modul
  
  if(Serial.available()){
//  security += char(Serial.read());
 security =Serial.readStringUntil('\n');
  int security1 = security.toInt();
  Serial.println(security);
  
  if(security1 == 1){

  backsystem();
  frontsystem();
  }
  else{
    servo0.write(0);
    ON[0]='2';
//    Serial.println(ON[0]);
  Serial.println('2');
  }
}
}