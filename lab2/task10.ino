#include <PDM.h>
#include <Arduino_APDS9960.h>
#include <Arduino_BMI270_BMM150.h>

const int SOUND_THRESHOLD = 50;   
const int DARK_THRESHOLD  = 100;  
const float MOTION_THRESHOLD = 0.15;
const int PROX_THRESHOLD  = 60;   


int lastClearVal = 0;
int lastProxVal = 0;

short sampleBuffer[256];
volatile int samplesRead = 0;

void onPDMdata() {
  int bytesAvailable = PDM.available();
  PDM.read(sampleBuffer, bytesAvailable);
  samplesRead = bytesAvailable / 2;
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(2000); 

  if (!APDS.begin()) Serial.println("APDS Failed");
  if (!IMU.begin())  Serial.println("IMU Failed");

  if (!PDM.begin(1, 16000)) {
    Serial.println("PDM Failed");
    while (1);
  }
  
  PDM.onReceive(onPDMdata);
  Serial.println("All sensors initialized.");
}

void loop() {

  int micLevel = 0;
  if (samplesRead) {
    long sum = 0;
    for (int i = 0; i < samplesRead; i++) {
      sum += abs(sampleBuffer[i]);
    }
    micLevel = sum / samplesRead;
    samplesRead = 0;
  }

  if (APDS.colorAvailable()) {
    int r, g, b, tempClear;
    APDS.readColor(r, g, b, tempClear);
    lastClearVal = tempClear;
  }
  if (APDS.proximityAvailable()) {
    lastProxVal = APDS.readProximity();
  }

  float ax, ay, az, motionMag = 1.0;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);
    motionMag = sqrt(ax*ax + ay*ay + az*az);
  }

  bool isSound  = micLevel > SOUND_THRESHOLD;
  bool isDark   = lastClearVal < DARK_THRESHOLD;
  bool isMoving = abs(motionMag - 1.0) > MOTION_THRESHOLD;
  bool isNear   = lastProxVal < PROX_THRESHOLD;

  String label = "UNKNOWN";
  if (!isSound && !isDark && !isMoving && !isNear)      label = "QUIET_BRIGHT_STEADY_FAR";
  else if (isSound && !isDark && !isMoving && !isNear)  label = "NOISY_BRIGHT_STEADY_FAR";
  else if (!isSound && isDark && !isMoving && isNear)   label = "QUIET_DARK_STEADY_NEAR";
  else if (isSound && !isDark && isMoving && isNear)    label = "NOISY_BRIGHT_MOVING_NEAR";

  Serial.print("raw,mic="); Serial.print(micLevel);
  Serial.print(",clear="); Serial.print(lastClearVal);
  Serial.print(",motion="); Serial.print(motionMag, 3);
  Serial.print(",prox="); Serial.println(lastProxVal);

  Serial.print("flags,sound="); Serial.print(isSound);
  Serial.print(",dark="); Serial.print(isDark);
  Serial.print(",moving="); Serial.print(isMoving);
  Serial.print(",near="); Serial.println(isNear);

  Serial.print("state,"); Serial.println(label);

  delay(1000); 
}
