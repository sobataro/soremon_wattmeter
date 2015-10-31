#include <TimerOne.h>

#define VOLTAGE_PIN        0
#define CURRENT_PIN        1
#define REFERENCE_PIN      2

#define BAUDRATE           115200
#define SAMPLE_PER_SEC     1500          // 50[Hz] と 60[Hz] の公倍数
#define SAMPLE_PERIOD_USEC (1000000 / SAMPLE_PER_SEC)

#define LSB_PER_VOLT       2.837         // 実測による値
#define LSB_PER_AMPERE     29.93         // 同上

#define NOISE_WATT_THRESHOLD 1           // これ未満のW数ならノイズとして捨てる

// 1秒ごとの積算値; 1秒ごとにリセット
int    secCount;                         // カウンタ
double voltSqSec, currentSqSec;          // V^2, A^2 の1秒積算値
double voltCurrentSec;                   // V * A の1秒積算値

void setup()
{
  Serial.begin(BAUDRATE);
  
  Timer1.initialize(SAMPLE_PERIOD_USEC);
  Timer1.attachInterrupt(sample);
  analogReference(INTERNAL);             // 基準電圧は内部生成の1.1[V]

  resetSecValues();
}

void loop() {}

// 1秒積算値をリセット
void resetSecValues()
{
  secCount = 0;
  voltSqSec = currentSqSec = voltCurrentSec = 0.0;
}

// タイマ割り込み; (1 / SAMPLE_PER_SEC) [sec] ごとに実行
void sample()
{
  int reference = analogRead(REFERENCE_PIN);
  int v = analogRead(VOLTAGE_PIN) - reference;
  int c = analogRead(CURRENT_PIN) - reference;
  secCount++;

  double volt = ((double) v) / LSB_PER_VOLT;
  double current = ((double) c) / LSB_PER_AMPERE;

  voltSqSec      += volt * volt;
  currentSqSec   += current * current;
  voltCurrentSec += volt * current;

  if (secCount == SAMPLE_PER_SEC) {
    double vrms = sqrt(voltSqSec / SAMPLE_PER_SEC);
    double irms = sqrt(currentSqSec / SAMPLE_PER_SEC);
    double watt = voltCurrentSec / SAMPLE_PER_SEC;
    double va   = vrms * irms;
    double powerFactor = watt / va;

    if (watt < 0) { // ノイズ or 配線間違い (電圧と電流が逆位相) のとき
      watt = -watt;
      powerFactor = -powerFactor;
    }

    if (watt < NOISE_WATT_THRESHOLD) { // ノイズ
      irms = watt = va = 0;
      powerFactor = 0;
    }

    resetSecValues();
    
    Serial.print("{");
    Serial.print("\"V\": ");
    Serial.print(vrms);
    Serial.print(", \"A\": ");
    Serial.print(irms);
    Serial.print(", \"W\": ");
    Serial.print(watt);
    Serial.print(", \"VA\": ");
    Serial.print(va);
    Serial.print(", \"PF\": ");
    Serial.print(powerFactor);
    Serial.println("}");
  }
}

