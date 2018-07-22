#define acs712 A0

long lastSample = 0;
long sampleSum =0;
int sampleCount = 0;

float vpc = 4.8828125;

void setup() {
  pinMode(acs712, INPUT);
  Serial.begin(115200);

}

void loop() {
  if(millis()>lastSample + 1){
    sampleSum += sq(analogRead(acs712)-507);
    sampleCount++;
    lastSample=millis();
    }
   if(sampleCount==1000){
    float mean = sampleSum / sampleCount;
    float rmsValue = sqrt(mean);
    float mv = rmsValue*vpc;
    float amperage = mv / 66;
    float wattage = amperage * 230;
    //Serial.println(analogRead(acs712));
    //Serial.println("Amperage: "+String(amperage)+"  Wattage: "+String(wattage));
    Serial.println(wattage);

    sampleSum = 0;
    sampleCount = 0;
    
    }
}
