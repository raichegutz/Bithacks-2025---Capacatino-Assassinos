#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

int motor1Pin1 = 4; 
int motor1Pin2 = 5; 
int enable1Pin = 6; 

int motor2Pin1 = 16; 
int motor2Pin2 = 15; 
int enable2Pin = 7; 

int servoPin = 18;
unsigned long lastServoUpdate = 0;
unsigned long servoInterval   = 50; // Move servo every 250 ms
int currentAngle = 0;
int angleStep    = 50;
Servo myServo;

const int trigPin = 12;
const int echoPin = 13;
//define sound speed in cm/uS
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

unsigned long lastUltrasonicUpdate = 0;
const unsigned long ultrasonicInterval = 100; // measure every 100 milliseconds

int distance;

// TwoWire I2C1 = TwoWire(0);  // The default "Wire" is usually TwoWire(1) or simply Wire
TwoWire I2C2 = TwoWire(1);  // Create another TwoWire instance for a second bus

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

int phase2 = 0;

#define SCREEN_WIDTH 128 // OLED display width,  in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define TRIG_PIN 36 // ESP32 pin GPIO14 connected to Ultrasonic Sensor's TRIG pin
#define ECHO_PIN 37 // ESP32 pin GPIO27 connected to Ultrasonic Sensor's ECHO pin

#define MAX_TRASH_LEVEL 2 // maximum trash level allowed before dumping
//#define CUP_HEIGHT 12 // cup height 
#define ENTIRE_HEIGHT 10 // distance from sensor to the bottom of cup

#define RESET_SERVO 0 //resets cup servo to original position

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C2, -1); // create SSD1306 display object connected to I2C
Servo CupServo;

String tempString;

float incr_distance = 0.0;

void setup() {
  phase2 = 0;
  Wire.begin(41, 42);
  I2C2.begin(9,8);
  
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);

  myServo.attach(servoPin);

  // sets the pins as outputs:
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);

  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
  pinMode(enable2Pin, OUTPUT);
  
  digitalWrite(enable1Pin, HIGH);
  digitalWrite(enable2Pin, HIGH);

  Serial.begin(115200);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

  delay(2000);         // wait for initializing
  oled.clearDisplay(); // clear display

  oled.setTextSize(2);      // text size
  oled.setTextColor(WHITE); // text color
  oled.setCursor(0, 10);    // position to display

  tempString.reserve(10);   // to avoid fragmenting memory when using String

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  CupServo.setPeriodHertz(50);
  CupServo.attach(21);
  CupServo.write(RESET_SERVO);

}

void tipCuptoDump(){
  for(int posDegrees = RESET_SERVO; posDegrees <= 120; posDegrees++) {
    CupServo.write(posDegrees);
    Serial.println(posDegrees);   
  }
  delay(2500);
  for(int posDegrees = 120; posDegrees >= RESET_SERVO; posDegrees--) {
    CupServo.write(posDegrees);
    Serial.println(posDegrees);  
  } 
}

void oledDisplayCenter(String text) {
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;

  oled.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);

  // display on horizontal and vertical center
  oled.clearDisplay(); // clear display
  oled.setCursor((SCREEN_WIDTH - width) / 2, (SCREEN_HEIGHT - height) / 2);
  oled.setRotation(2);
  oled.println(text); // text to display
  oled.display();
}

void forwardMotor(){
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH); 
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH); 
}

void backwardMotor(){
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW); 
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW); 
}

void stopMotor(){
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW); 
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, LOW); 
}

void turnLeft(){
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW); 
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
}

void turnRight(){
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW); 
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, LOW);
}

int distanceF(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  long duration = pulseIn(echoPin, HIGH);
  // Calculate the distance
  int distanceCm = duration * SOUND_SPEED/2;
  return distanceCm;
}

void loop() {
  if(phase2 == 0){
    Serial.println("0");
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // read the echo
    long duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms (max ~5 meters)
    float distance = 0.0;

    if (duration == 0) {
      Serial.println("Out of range");
    } else {
      distance = (duration * 0.0343) / 2;
    }



    // calculating percentage of trash levels
    float trash_percent = 0.0;
    if((distance > MAX_TRASH_LEVEL) && (distance < ENTIRE_HEIGHT)){
      CupServo.write(RESET_SERVO);
      trash_percent = (((ENTIRE_HEIGHT - distance)/ENTIRE_HEIGHT)*100);

      if (trash_percent >= incr_distance){
        incr_distance = trash_percent;
      }

      tempString = String(incr_distance, 0);
      tempString += " %";
      oledDisplayCenter(tempString); // display temperature on OLED
    } 
    else if (distance >= ENTIRE_HEIGHT){
      CupServo.write(RESET_SERVO);
      tempString = String(incr_distance, 0);
      tempString += " %";
      oledDisplayCenter(tempString);
    }
    else {
      int16_t x1;
      int16_t y1;
      uint16_t width;
      uint16_t height;

      oled.clearDisplay();
      oled.getTextBounds("Trash Full", 0, 0, &x1, &y1, &width, &height);
      oled.setCursor((SCREEN_WIDTH - width) / 2, 16);
      oled.println("Trash Full");
      oled.getTextBounds("Emptying!", 0, 0, &x1, &y1, &width, &height);
      oled.setCursor((SCREEN_WIDTH - width) / 2, 35);
      oled.println("Emptying!");
      oled.setRotation(2);
      oled.display();
      incr_distance = 0.0;
      phase2 = 1;
    }
    delay(800);
  }
  

  // Move the DC motor forward at maximum speed
  if (phase2 == 1){
    Serial.println("1");
    distance = distanceF();

    if(distance >= 10){
      forwardMotor();
      myServo.write(85);
    }
    if(distance < 10){
      // backwardMotor();
      // delay(1000);
      stopMotor();

      myServo.write(65);
      delay(250);
      int leftCm = distanceF();
      Serial.print("11111Distance (cm): ");
      Serial.println(leftCm);

      myServo.write(115);
      delay(250);
      int rightCm = distanceF();
      Serial.print("2Distance (cm): ");
      Serial.println(rightCm);

      backwardMotor();
      delay(750);
      stopMotor();

      if(leftCm > rightCm){
        turnRight();
        delay(1000);
      }
      if(rightCm > leftCm){
        turnLeft();
        delay(1000);
      }
  }

  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);
  // Calculate average light level
  float sum = c;
  float r_scaled = (r / sum) * 255.0;
  Serial.print("R: "); Serial.println((int)r_scaled);

  if(r_scaled > 130){
    stopMotor();
    tipCuptoDump();
    phase2 = 0;
  }

  }
  
}
