#include <Arduino_HS300x.h>
#include <Arduino_BMI270_BMM150.h>
#include <Arduino_APDS9960.h>

const int TEMP_THRESHOLD = 26;   
const int HUMIDITY_THRESHOLD  = 70;  
const float MAGNETIC_THRESHOLD = 100;
const int COLOR_THRESHOLD  = 80; 

enum {BASELINE_NORMAL, EVENT_TRIGGERED};
unsigned char state;

void setup() {
  Serial.begin(115200);
  delay(500);

  if (!HS300x.begin()) {
    Serial.println("Failed to initialize humidity/temperature sensor.");
    while (1);
  }

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  if (!APDS.begin()) {
    Serial.println("Failed to initialize APDS9960 sensor.");
    while (1);
  }

  state = BASELINE_NORMAL;

}

void loop() {

  // read values
  float temperature = HS300x.readTemperature();
  float humidity = HS300x.readHumidity();
  // Serial.print("t:");
  // Serial.println(temperature);
  // Serial.print("h:");
  // Serial.println(humidity);

  static float x, y, z, mag;
  if (IMU.magneticFieldAvailable()) {
    IMU.readMagneticField(x, y, z);
    mag = sqrt(x*x + y*y + z*z);
    // Serial.print("mag:");
    // Serial.println(mag);
  }


  static int r, g, b, c;
  if (APDS.colorAvailable()) {
    APDS.readColor(r, g, b, c);
    // Serial.println(r);
    // Serial.println(g);
    // Serial.println(b);
    // Serial.println(c);
  }

  // state machine

  int humid_jump, temp_rise, mag_shift, light_change;
  humid_jump = 0;
  temp_rise = 0;
  mag_shift = 0;
  light_change = 0;

  String FINAL_LABEL = "";

  switch (state) {
    case BASELINE_NORMAL:
      if (temperature > TEMP_THRESHOLD) {
        temp_rise = 1;
        state = EVENT_TRIGGERED;
        FINAL_LABEL = "BREATH_OR_WARM_AIR_EVENT";
      } else if (humidity > HUMIDITY_THRESHOLD) {
        humid_jump = 1;
        state = EVENT_TRIGGERED;
        FINAL_LABEL = "BREATH_OR_WARM_AIR_EVENT";
      } else if (mag > MAGNETIC_THRESHOLD) {
        mag_shift = 1;
        state = EVENT_TRIGGERED;
        FINAL_LABEL = "MAGNETIC_DISTURBANCE_EVENT";
      } else if (r < COLOR_THRESHOLD || g < COLOR_THRESHOLD || b < COLOR_THRESHOLD || c < COLOR_THRESHOLD) {
        state = EVENT_TRIGGERED;
        FINAL_LABEL = "LIGHT_OR_COLOR_CHANGE_EVENT";
        light_change = 1;
      } else {
        state = BASELINE_NORMAL;
        break; // dont print if no event triggered
      }

      // prints
      Serial.print("raw,rh="); Serial.print(humidity);
      Serial.print(",temp="); Serial.print(temperature);
      Serial.print(",mag="); Serial.print(mag);
      Serial.print(",r="); Serial.print(r);
      Serial.print(",g="); Serial.print(g);
      Serial.print(",b="); Serial.print(b);
      Serial.print(",clear="); Serial.println(c);

      Serial.print("flags,humid_jump="); Serial.print(humid_jump);
      Serial.print(",temp_rise="); Serial.print(temp_rise);
      Serial.print(",mag_shift="); Serial.print(mag_shift);
      Serial.print(",light_or_color_change="); Serial.println(light_change);
      
      Serial.print("event,"); Serial.println(FINAL_LABEL);

      break;

    case EVENT_TRIGGERED:
      delay(3000);
      state = BASELINE_NORMAL;
      break;

    default:
      state = BASELINE_NORMAL;
      break;
    
  }
  delay(100);
}
