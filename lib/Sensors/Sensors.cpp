#include <Sensors.h>

#include <RTClib.h>
extern RTC_DS3231 *ds3231;

SensorsRecord::SensorsRecord(uint32_t timestamp) {
  this->ts = timestamp;
  return;
}
SensorsRecord::SensorsRecord(uint32_t timestamp, float temperatureC, float pressurePa, float humidityPct, int16_t adc0, int16_t adc1, int16_t adc2, int16_t adc3) {
  this->setValues(timestamp, temperatureC, pressurePa, humidityPct, adc0, adc1, adc2, adc3);
  return;
}

void SensorsRecord::setTemperature(int16_t temperature) { this->temperature = temperature; }
void SensorsRecord::setTemperatureC(float temperatureC) { this->temperature = (int16_t)(temperatureC*10); }
void SensorsRecord::setPressure(uint16_t pressure)      { this->pressure = pressure; }
void SensorsRecord::setPressurePa(float pressurePa)     { this->pressure = (uint16_t)(pressurePa/10); }
void SensorsRecord::setPressurehPa(float pressurehPa)   { this->pressure = (uint16_t)(pressurehPa*10); };
void SensorsRecord::setHumidity(uint8_t humidity)       { this->humidity = humidity; };
void SensorsRecord::setAdc(uint8_t channel, int16_t value) { this->adc_channel[channel] = value; }

void SensorsRecord::setValues(uint32_t timestamp, float temperatureC, float pressurePa, float humidityPct, int16_t adc0, int16_t adc1, int16_t adc2, int16_t adc3) {
  this->ts = timestamp;
  this->setTemperatureC(temperatureC);
  this->setPressurePa(pressurePa);
  this->setHumidity(humidityPct);
  this->setAdc(0, adc0);
  this->setAdc(1, adc1);
  this->setAdc(2, adc2);
  this->setAdc(3, adc3);
  return;
}

uint32_t SensorsRecord::timestamp()        { return this->ts; } 
int16_t  SensorsRecord::rawTemperature()   { return this->temperature; }
float    SensorsRecord::temperatureC()     { return (float)this->temperature/10; }
uint16_t SensorsRecord::rawPressure()      { return this->pressure; }
float    SensorsRecord::pressurePa()       { return (float)this->pressure*10; }
float    SensorsRecord::pressurehPa()      { return (float)this->pressure/10; }
uint8_t  SensorsRecord::rawHumidity()      { return this->humidity; }
int      SensorsRecord::humidityPct()      { return (int)this->humidity; }
int16_t  SensorsRecord::adc(uint8_t index) { return this->adc_channel[index]; }

void SensorsRecord::print() {
  Serial.print(F("Timestamp   : ")); Serial.println(this->timestamp());
  Serial.print(F("Temperature : ")); Serial.print(this->temperatureC()); Serial.println("C");
  Serial.print(F("Pressure    : ")); Serial.print(this->pressurehPa());  Serial.println("hPa");
  Serial.print(F("Humidity    : ")); Serial.print(this->humidityPct());  Serial.println("%");
  for (uint8_t c=0; c<4; c++) {
    Serial.print(F("ADC chan")); Serial.print(c); Serial.print(F(" : ")); Serial.println(this->adc(c));
  }
}

const char *SensorsRecord::csvhead() {
  const char *csvhead = "Timestamp;TemperatureC;PressurehPa;HumidityPct;ADC0;ADC1;ADC2;ADC3";
  return csvhead;
}

char *SensorsRecord::csvline() {
  char *csv = (char *)calloc(128, sizeof(char));
  sprintf(csv, "%d;%.1f;%.1f;%d;%d;%d;%d;%d", this->timestamp(), this->temperatureC(), this->pressurehPa(), this->humidityPct(), this->adc(0), this->adc(1), this->adc(2), this->adc(3));
  return csv;
}

char *SensorsRecord::json() {
  char *json = (char *)calloc(256, sizeof(char));
  sprintf(json, "{\"timestamp\":\"%d\", \"temperature\":\"%.1f\", \"pressure\":\"%.1f\", \"humidity\":\"%d\", \"adc0\":\"%d\", \"adc1\":\"%d\", \"adc2\":\"%d\", \"adc3\":\"%d\"}",
                this->timestamp(), this->temperatureC(), this->pressurehPa(), this->humidityPct(), this->adc(0), this->adc(1), this->adc(2), this->adc(3));
  return json;
}







Sensors::Sensors(uint16_t ram_records, uint8_t BMEaddr, uint8_t ADCaddr) {
  this->bme280 = new BME280();
  this->bme280->setI2CAddress(BMEaddr);
  this->ads1115 = new ADS1115(ADCaddr);
  this->records_index = 0;
  this->records_size = ram_records;
  this->records = (SensorsRecord *)calloc(this->records_size, sizeof(SensorsRecord));
  for (uint16_t i=0; i<this->records_size; i++) { this->records[i] = SensorsRecord(); }
  return;
}

bool Sensors::begin() {
  bool success = true;
  
  if (this->bme280->beginI2C()) { Serial.println(F("Sensors::BME280 initialized")); }
  else { Serial.println(F("Error : Sensors::BME280 initialization failed")); free(this->bme280); this->bme280 = nullptr; success = false; }
  
  if (this->ads1115->begin()) { Serial.println(F("Sensors::ADS1115 ADC initialized")); this->ads1115->setGain(ADS1115_GAIN); this->ads1115->setMode(ADS1115_MODE); this->ads1115->setDataRate(ADS1115_RATE); }
  else { Serial.println(F("Error : Sensors::ADS1115 ADC initialization failed")); free(this->ads1115); this->ads1115 = nullptr; success = false; }

  return success;
}


uint8_t Sensors::update() {
  uint32_t timestamp;
  float temperatureC, pressurePa, humidityPct;
  int16_t adc_values[4];
  uint8_t errors = 0;
  if (ds3231!=nullptr) timestamp = ds3231->now().unixtime();
  else { timestamp = 0; errors += 1; }
  if (bme280!=nullptr) {
    temperatureC = bme280->readTempC();
    pressurePa   = bme280->readFloatPressure();
    humidityPct  = bme280->readFloatHumidity();
  } else {
    temperatureC = 0;
    pressurePa   = 0;
    humidityPct  = 0;
    errors += 2;
  }
  if (ads1115!=nullptr) {
    for (uint8_t c=0; c<4; c++) adc_values[c] = ads1115->readADC(c);
  } else {
    for (uint8_t c=0; c<4; c++) adc_values[c] = ads1115->readADC(c);
    errors += 4;
  }
  /*Serial.println(F("Raw sensors readings :"));
  Serial.print(F("  BME280 (float) : T=")); Serial.print(temperatureC); Serial.print(F(" P=")); Serial.print(pressurePa); Serial.print(F(" H=")); Serial.println(humidityPct);
  Serial.print(F("  ADS1115 (uint16_t) :")); for (uint8_t i=0; i<4; i++) { Serial.print(F(" Ch")); Serial.print(i); Serial.print("="); Serial.print(adc_values[i]); } Serial.println();*/
  this->records[this->records_index].setValues(timestamp, temperatureC, pressurePa, humidityPct, adc_values[0], adc_values[1], adc_values[2], adc_values[3]);
  return errors;
}

SensorsRecord Sensors::read() {
  uint32_t timestamp = 0;
  if (ds3231!=nullptr) timestamp = ds3231->now().unixtime();
  SensorsRecord record(timestamp);
  if (bme280!=nullptr) {
    record.setTemperatureC(bme280->readTempC());
    record.setPressurePa(bme280->readFloatPressure());
    record.setHumidity(bme280->readFloatHumidity());
  }
  if (ads1115!=nullptr) {
    for (uint8_t c=0; c<4; c++) record.setAdc(c, ads1115->readADC(c));
  }
  return record;
}

SensorsRecord Sensors::lastRecord() {
  return this->records[this->records_index];
}

SensorsRecord Sensors::getRecord(uint16_t record_index) {
  return this->records[record_index];
}

SensorsRecord Sensors::getRecord(uint32_t timestamp) {
  uint16_t closest=0;
  uint32_t cdiff=0xFFFFFFFF, diff;
  for (uint16_t cursor=0; cursor<this->records_size; cursor++) {
    if (timestamp > this->records[cursor].timestamp()) diff = timestamp - this->records[cursor].timestamp();
    else diff = this->records[cursor].timestamp() - timestamp;
    if (diff<cdiff) closest = cursor;
  }
  return this->records[closest];
}

SensorsRecord Sensors::maxValues() {
  SensorsRecord max = SensorsRecord(0, 0x00000000, 0x00000000, 0x00000000, 0x0000, 0x0000, 0x0000, 0x0000);
  for (int16_t c=0; c<this->records_size; c++) {
    if (this->records[c].timestamp() == 0) continue;
    if (this->records[c].rawTemperature()>max.rawTemperature()) max.setTemperature(this->records[c].rawTemperature());
    if (this->records[c].rawPressure()>max.rawPressure()) max.setPressure(this->records[c].rawPressure());
    if (this->records[c].rawHumidity()>max.rawHumidity()) max.setHumidity(this->records[c].rawHumidity());
    for (uint8_t ch=0; ch<4; ch++) {
      if (this->records[c].adc(ch)>max.adc(ch)) max.setAdc(ch, this->records[c].adc(ch));
    }
  }
  return max;
}

SensorsRecord Sensors::minValues() {
  SensorsRecord min = SensorsRecord(0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF);
  for (uint16_t c=0; c<this->records_size; c++) {
    if (this->records[c].timestamp() == 0) continue;
    if (this->records[c].rawTemperature()<min.rawTemperature()) min.setTemperature(this->records[c].rawTemperature());
    if (this->records[c].rawPressure()<min.rawPressure()) min.setPressure(this->records[c].rawPressure());
    if (this->records[c].rawHumidity()<min.rawHumidity()) min.setHumidity(this->records[c].rawHumidity());
    for (uint8_t ch=0; ch<4; ch++) { if (this->records[c].adc(ch)<min.adc(ch)) min.setAdc(ch, this->records[c].adc(ch)); }
  }
  return min;
}

SensorsRecord Sensors::avgValues() {
  int32_t t;
  uint32_t p;
  uint16_t h;
  int32_t a[4];
  uint16_t tv=0;
  for (int16_t c=0; c<this->records_size; c++) {
    if (this->records[c].timestamp() == 0) continue;
    t += this->records[c].rawTemperature();
    p += this->records[c].rawPressure();
    h += this->records[c].rawHumidity();
    for (uint8_t ch=0; ch<4; ch++) { a[ch] += this->records[c].adc(ch); }
    tv++;
  }
  return SensorsRecord(0, (float)t/tv, (float)p/tv, float(h)/tv, (int16_t)(a[0]/tv), (int16_t)(a[1]/tv), (int16_t)(a[2]/tv), (int16_t)(a[3]/tv));
}
