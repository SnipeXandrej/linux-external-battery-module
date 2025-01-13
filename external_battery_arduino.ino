// int BatVoltageGroundRaw = 0;
int BatVoltageRaw = 0;
// int BatCurrentRaw = 0;
// float BatVoltageGround = 0;
float BatVoltage = 0;
// float BatCurrent = 0;

float BatVoltageMultiplier = (82000 / 6800) + 1; // (R2 / R1) + 1
int BatVoltagePercentage;

double map_value;
int map(double x, double in_min, double in_max, double out_min, double out_max) {
    map_value = (x - in_min) * (out_max - out_min + 1) / (in_max - in_min + 1) + out_min;

    if (map_value > out_max) {
        map_value = out_max;
    }

    if (map_value < out_min) {
        map_value = out_min;
    }

    return map_value;
}

void setup() {
  Serial.begin(9600);

  // pinMode(A0, INPUT); // Battery Voltage Virtual Ground
  pinMode(A1, INPUT); // Battery Voltage Positive
  // pinMode(A2, INPUT); // Battery Current
}

void loop() {
  // BatVoltageGroundRaw = analogRead(A0);
  BatVoltageRaw = analogRead(A1);
  // BatCurrentRaw = analogRead(A2);

  // (5 / 1023) = 0.004887585532746823
  // BatVoltageGround = (0.004887585532746823 * BatVoltageGroundRaw);
  BatVoltage = (0.004887585532746823 * BatVoltageRaw) * BatVoltageMultiplier + 0.8; 
  BatVoltagePercentage = map(BatVoltage, 32, 41, 0, 100);

  Serial.print("Battery Voltage: ");
  Serial.print(BatVoltagePercentage);
  Serial.print("\n");

  // TODO: Implement current sensing
  // Serial.print("Battery Current: ");
  // Serial.print(BatCurrent);
  // Serial.print("\r\n");

  delay(1000);
}
