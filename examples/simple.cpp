#include <Homie.h>
#include <LoggerNode.h>

void setup() {
  Homie_setFirmware("LoggerNodeExample", "1.0.0");
  Homie.setLoggingPrinter(&Serial);
  Serial.begin(74880);
  LN.log(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Before Homie setup())");
  Homie.setup();
}

void loop() {
  Homie.loop();
  static int last = 0;

  if (millis()-last > 2000) {
    LN.logf("loop()", LoggerNode::INFO, "Loop: %d millis", millis());
    last = millis();
 }
}
