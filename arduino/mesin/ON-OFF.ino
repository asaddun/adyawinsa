#define buttonSetup 5   // D1
#define ledSetup 14     // D5
#define buttonSetting 4 // D2
#define ledSetting 15   // D8
#define buttonRun 16    // D0
#define ledRun 0        // D3
                        // Clamp D6
                        // Inject D7

int settingState = 0;
int setupState = 0;
int runState = 0;
int prevSettingState = 0;
int prevSetupState = 0;
int prevRunState = 0;
bool setupOn = false;
bool settingOn = false;
bool runOn = false;
unsigned long setupStartTime;
unsigned long setupDuration;
unsigned long settingStartTime;
unsigned long settingDuration;
unsigned long runStartTime;
unsigned long runDuration;
int setupMinute, setupSecond;
int settingMinute, settingSecond;
int runMinute, runSecond;

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("");
  Serial.println("Ready..");

  pinMode(buttonSetting, INPUT);
  pinMode(buttonSetup, INPUT);
  pinMode(buttonRun, INPUT);
  pinMode(ledRun, OUTPUT);
  pinMode(ledSetting, OUTPUT);
  pinMode(ledSetup, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_BUILTIN, LOW);
}

void loop(){
  settingState = digitalRead(buttonSetting);
  setupState = digitalRead(buttonSetup);
  runState = digitalRead(buttonRun);

  if (setupState == HIGH && prevSetupState == LOW) {
    digitalWrite(ledSetup, !digitalRead(ledSetup));
    if (digitalRead(ledSetup) == HIGH) {
      Serial.println("Setup: ON");
      setupOn = true;
      setupStartTime = millis();
      if (settingOn == true){
        digitalWrite(ledSetting, LOW);
        Serial.println("Setting: OFF");
        settingDuration = millis() - settingStartTime;
        Serial.print("Setting duration: ");
        settingMinute = settingDuration / 1000 / 60;
        Serial.print(settingMinute);
        Serial.print(" min ");
        settingSecond = settingDuration / 1000 % 60;
        Serial.print(settingSecond);
        Serial.println(" sec");
        settingOn = false;
      }
      Serial.println("");
    } else {
      Serial.println("Setup: OFF");
      setupOn = false;
      setupDuration = millis() - setupStartTime;
      Serial.print("Setup duration: ");
      setupMinute = setupDuration / 1000 / 60;
      Serial.print(setupMinute);
      Serial.print(" min ");
      setupSecond = setupDuration / 1000 % 60;
      Serial.print(setupSecond);
      Serial.println(" sec");
      Serial.println("");
    }
  }
  prevSetupState = setupState;
  delay(50);

  if (settingState == HIGH && prevSettingState == LOW) {
    digitalWrite(ledSetting, !digitalRead(ledSetting));
    if (digitalRead(ledSetting) == HIGH) {
      Serial.println("Setting: ON");
      settingOn = true;
      settingStartTime = millis();
      if (setupOn == true){
        digitalWrite(ledSetup, LOW);
        Serial.println("Setup: OFF");
        setupDuration = millis() - setupStartTime;
        Serial.print("Setup duration: ");
        setupMinute = setupDuration / 1000 / 60;
        Serial.print(setupMinute);
        Serial.print(" min ");
        setupSecond = setupDuration / 1000 % 60;
        Serial.print(setupSecond);
        Serial.println(" sec");
        setupOn = false;
      }
      Serial.println("");
    } else {
      Serial.println("Setting: OFF");
      settingOn = false;
      settingDuration = millis() - settingStartTime;
      Serial.print("Setting duration: ");
      settingMinute = settingDuration / 1000 / 60;
        Serial.print(settingMinute);
        Serial.print(" min ");
        settingSecond = settingDuration / 1000 % 60;
        Serial.print(settingSecond);
      Serial.println(" sec");
      Serial.println("");
    }
  }
  prevSettingState = settingState;
  delay(50);

  if (runState == HIGH && prevRunState == LOW) {
    digitalWrite(ledRun, !digitalRead(ledRun));
    if (digitalRead(ledRun) == HIGH) {
      Serial.println("Run: ON");
      runOn = true;
      // setupStartTime = millis();
      // if (setupOn == true){
      //   digitalWrite(ledSetup, LOW);
      //   Serial.println("Setup: OFF");
      //   setupDuration = millis() - setupStartTime;
      //   Serial.print("Setup duration: ");
      //   setupMinute = setupDuration / 1000 / 60;
      //   Serial.print(setupMinute);
      //   Serial.print(" min ");
      //   setupSecond = setupDuration / 1000 % 60;
      //   Serial.print(setupSecond);
      //   Serial.println(" sec");
      //   setupOn = false;
      // }
      Serial.println("");
    } else {
      Serial.println("Run: OFF");
      runOn = false;
      // setupDuration = millis() - setupStartTime;
      // Serial.print("Setup duration: ");
      // setupMinute = setupDuration / 1000 / 60;
      // Serial.print(setupMinute);
      // Serial.print(" min ");
      // setupSecond = setupDuration / 1000 % 60;
      // Serial.print(setupSecond);
      // Serial.println(" sec");
      Serial.println("");
    }
  }
  prevRunState = runState;
  delay(50);
}