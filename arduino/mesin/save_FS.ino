#include <FS.h>

#define  pinInject D7
#define  pinStatus D6

bool status = true;
bool print = false;
int staInj = 0;
int laststateInject = 0, laststateStatus = 0;
unsigned long cycleTime;
int stateInject = 0, stateStatus = 0;
int i = 0;
int array[] = {60, 50, 55, 75, 70};
int random_index;

void setup() {
  Serial.begin(115200);
  SPIFFS.begin();

  pinMode(pinInject, INPUT);
  pinMode(pinStatus, INPUT);
  Serial.println("Status: ON");
}

void writefile(){
  // Write to file if `status` is false
  File file = SPIFFS.open("/file.txt", "a");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  // Write cycletime data to file
  String line = "{\"shoot\":\"";
  line += i;
  line += "\",\"cycletime\":\"";
  line += cycleTime;
  line += "\"}";
  file.println(line);

  // API required: {"action":"shoot","id":"10000000","cycletime":00,"ip":"192.168.1.1"}
  // Node-RED need "inj":1

  // Close file
  file.close();
}

void readfile(){
  // Open the file for reading
  File file = SPIFFS.open("/file.txt", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  // Read the file contents
  while (file.available()) {
    String line = file.readStringUntil('\n');
    Serial.println(line);
    delay(100);
  }
  // Close the file
  file.close();
  SPIFFS.remove("/file.txt");
}

void statusact(){
  stateStatus = digitalRead(pinStatus);

  if (stateStatus != laststateStatus && stateStatus == LOW) {
    status = !status;
    // Serial.println(status);
    if (status == 1){
      Serial.println("Status: ON");
      readfile();
      i = 0;
    } else {
      Serial.println("Status: OFF");
    }
  }

  laststateStatus = stateStatus;
  delay(50);
}

void injectact(){
  stateInject = digitalRead(pinInject);
  if (stateInject != laststateInject) {
    if (stateInject == LOW) {
      // Serial.println("Button released");
      // print = true;
    } else {
      // Serial.println("Button pressed");
      random_index = random(0, 5);
      cycleTime = array[random_index];
      if (status == true){
        print = true;
      } else {
        i++;
        writefile();
      }
    }
  }
  laststateInject = stateInject;
}

void loop() {
  statusact();
  injectact();
  if (print == true){
    Serial.println(cycleTime);
    cycleTime = 0;
    print = false;
  }
}