const int CurrentPin = A0;
const int VoltagePin = A1;
const int BoostPin   = A2;
const int pwmPin = 9;

// Turbine divider
float R1 = 10000.0;
float R2 = 4700.0;

// Boost divider
float Rb1 = 47000.0;
float Rb2 = 10000.0;

float sensitivity_ACS712 = 0.066;
float offset_voltage_ACS712 = 0.0;

// MPPT variables
int duty = 80;   // start mid-range (important)
int stepSize = 3;
int direction = 1;
float prev_power = 0.0;

// Voltage limits
float V_max = 14.2;
float V_min = 12.5;

void setup() {
  Serial.begin(9600);
  pinMode(pwmPin, OUTPUT);

  long sum_offset = 0;
  for (int i = 0; i < 500; i++) {
    sum_offset += analogRead(CurrentPin);
  }

  float avg_offset = sum_offset / 500.0;
  offset_voltage_ACS712 = (avg_offset * 5.0) / 1023;
}

void loop() {

  // --- Current ---
  float adc_current = analogRead(CurrentPin);
  float voltage_ACS712 = (adc_current * 5.0) / 1023;
  float sys_current = (voltage_ACS712 - offset_voltage_ACS712) / sensitivity_ACS712;

  // --- Turbine Voltage ---
  float adc_turbine = analogRead(VoltagePin);
  float Vsense = (adc_turbine * 5.0) / 1023;
  float v_turbine = Vsense * ((R1 + R2) / R2);

  // --- Boost Output Voltage ---
  long sum_boost = 0;
  for (int i = 0; i < 20; i++) {
    sum_boost += analogRead(BoostPin);
  }
  float adc_boost = sum_boost / 20.0;

  float Vout_pin = (adc_boost * 5.0) / 1023.0;
  float v_boost = Vout_pin * ((Rb1 + Rb2) / Rb2);

  // --- Power ---
  float power = v_turbine * sys_current;

  // ==============================
  // 🔥 PRIORITY CONTROL
  // ==============================

  if (v_boost > V_max) {
    // 🔴 OVERVOLTAGE → force duty down
    duty -= 10;
  }
  else if (v_boost < V_min) {
    // 🟢 Safe → allow MPPT
    if (power > prev_power) {
      duty += stepSize * direction;
    } else {
      direction = -direction;
      duty += stepSize * direction;
    }
  }
  else {
    // 🟡 Near target → fine tuning
    if (v_boost > 13.8) {
      duty -= 2;
    } else {
      duty += 1;
    }
  }

  duty = constrain(duty, 10, 230);
  analogWrite(pwmPin, duty);

  prev_power = power;

  // --- Display ---
  Serial.print("I: "); Serial.print(sys_current, 2);
  Serial.print(" A | Vin: "); Serial.print(v_turbine, 2);
  Serial.print(" V | Vout: "); Serial.print(v_boost, 2);
  Serial.print(" V | PWM: "); Serial.println(duty);

  delay(200);
}
