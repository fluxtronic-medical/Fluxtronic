#include <Adafruit_MCP4725.h>
#include <UnicViewAD.h>
#include <Wire.h>


LCM Lcm(Serial);
Adafruit_MCP4725 dacToProg;

#define     num    10         //número de iterações da média móvel
#define     rangePmax 1020    //Pressão maxima que o sensor mpx5010 consegue medir (mmH2O)
#define     rangePmin 0       //Pressão minima que o sensor mpx5010 consegue medir (mmH2O)
#define     maxPressure 1000.l  //Range máximo de pressão

#define REDPIN 9
#define GREENPIN 10
#define BLUEPIN 11 

long moving_average(int sig);
int values[num];              //vetor com num posições, armazena os valores para cálculo da média móvel


int ins_valve = 2;         // rele que aciona valvula de inspiração
int ex_valve = 4;          // rele que aciona valvula de expiração
int aO_progress = 3;       //saída analógica para clp que controla o avanço maximo do pistao
int aO_speed = 5;          //saida analogica para clp que controla a velocidade do pistão
int a1_MPX5010 = 1;        //sensor de pressao

unsigned long intervalEx = 0;
unsigned long intervalIns = 0;
unsigned long previousMillisIns = 0;
unsigned long previousMillisEx = 0;
unsigned long startMillis;
unsigned long currentMillis = 0;
unsigned long timerGraph = 20;
unsigned long previousMillisGraph = 0;
unsigned long previousMillisPage = 0;
unsigned long previousMillisAlarm = 0;
unsigned long timerPage = 5;
unsigned long lapsusIns = 0;
unsigned long lapsusEx = 0;

LcmVar t_ins(0);
LcmVar freq_res(6);
LcmVar volume(12);
LcmVar debug1(30);
LcmVar debug2(36);
LcmVar debug3(18);
LcmVar t_ex(24);
LcmVar i(42);
LcmVar e(48);
LcmVar refresh_button(100);
LcmVar pressure(54);
LcmVar p_max(60);
LcmVar p_min(70);

const int pic_intro1 = 0;
const int pic_intro2 = 1;
const int pic_intro3 = 2;
const int pic_intro4 = 3;
const int pic_intro5 = 4;
const int pic_intro6 = 5;
const int pic_intro7 = 6;
const int pic_setup = 7;
const int pic_monitor = 9;
const int refresh_value = 1000;
int pic_Id = 0;


int val_p_max = rangePmin;
int val_p_min = rangePmax;
int val_t_ins = 0;
int val_t_ex = 0;
int val_freq_res = 0;
int val_volume = 0;
int val_debug1 = 0;
int val_debug2 = 0;
int val_debug3 = 0;
const int val_i = 1;
int val_e = 0;
//---------------
int last_t_ins = 0;
int last_t_ex = 0;
int last_freq_res = 0;
int last_volume = 0;
int updateSum = 0;
//---------------
int analog_volume=0;
int analog_pressure = 0;

void setup() {
  Lcm.begin(115200);
  dacToProg.begin(0x60);
  pinMode(aO_progress, OUTPUT);
  pinMode(aO_speed, OUTPUT);
  pinMode(ins_valve, OUTPUT);
  pinMode(ex_valve, OUTPUT);
  pinMode(REDPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);

  digitalWrite(ins_valve, LOW);
  digitalWrite(ex_valve, LOW);
  byte version = 0xFF;
  while (version == 0xFF)
  {
    version = Lcm.readFirmwareVersion();
  }
  
  
  Lcm.changePicId(pic_intro1);
  delay(500);
  Lcm.changePicId(pic_intro2);
  delay(700);
  Lcm.changePicId(pic_intro3);
  delay(300);
  Lcm.changePicId(pic_intro4);
  delay(500);
  Lcm.changePicId(pic_intro5);
  delay(1000);
  Lcm.changePicId(pic_intro6);
  delay(1500);
  Lcm.changePicId(pic_intro7);
  delay(500);
  Lcm.changePicId(pic_setup);
  startMillis = millis();
}

int arduinoToDisplay(float f , int fator) {
  int inteiro = 0;
  inteiro = f * fator;
  return inteiro;
}

float displayToArduino(int inteiro, int divisor) {
  float f;
  f = inteiro / divisor;
  return f;
}

void loop() {
  currentMillis = millis();
  pic_Id = Lcm.readPicId();
  lapsusIns = currentMillis - previousMillisIns;
  lapsusEx = currentMillis - previousMillisEx;

  if (pic_Id == 7) {
    updateSetup();
    analogWrite(REDPIN, 0);
    analogWrite(GREENPIN, 0);
    analogWrite(BLUEPIN, 255);

  }
  if (pic_Id == 9) {
    updatePressure();
  }
  val_volume     = volume.read();
  analog_volume = map(val_volume,0,1000,0,4095);
  dacToProg.setVoltage(analog_volume, false);
}


// ========================================================================================================
// --- Desenvolvimento das Funções ---
long moving_average(int sig)
{
  int i;               //variável auxiliar para iterações
  long acc = 0;        //acumulador
  //Desloca o vetor completamente eliminando o valor mais antigo
  for (i = num - 1; i > 0; i--) values[i] = values[i - 1];
  values[0] = sig;           //carrega o sinal no primeiro elemento do vetor
  for (i = 0; i < num; i++) acc += values[i];
  return acc / num;         //Retorna a média móvel
}


void updatePressure() {
  int grafico = 0;             //armazena a leitura filtrada da entrada analógica
  int adc_noise1 = 0;             //armazena a leitura sem filtro da entrada analógica

  if (currentMillis - previousMillisGraph >=  timerGraph) {
    analogWrite(REDPIN, 0);
    analogWrite(GREENPIN, 255);
    analogWrite(BLUEPIN, 0);
    adc_noise1 = analogRead(a1_MPX5010);
    grafico = map( moving_average(adc_noise1), 0 , 1023, 0, 1020);

    Lcm.writeTrendCurve0(grafico);
    previousMillisGraph = currentMillis;
    pressure.write(grafico);

    if (grafico > val_p_max) {
      val_p_max = grafico;
      p_max.write(val_p_max);
    }//if

    if (grafico < val_p_min) {
      val_p_min = grafico;
      p_min.write(val_p_min);
    }//if
    if (grafico >= (maxPressure)) {
      Lcm.beepBuzzer(255);
      analogWrite(REDPIN, 255);
      analogWrite(GREENPIN, 0);
      analogWrite(BLUEPIN, 0);
      Lcm.activateSoftwareControl(200);
    }//if

  }//if
}//updatePressure

void updateSetup() {
  val_t_ins     = t_ins.read();
  val_t_ex      = t_ex.read();
  val_freq_res  = freq_res.read();
  //val_e         = e.read();
  //if (t_ins.read() != 0 && freq_res.read() != 0) {
  //val_t_ex = ((6000 - (val_t_ins * val_freq_res)) / val_freq_res);
  // calcValTempEx();
  //t_ex.write(val_t_ex);
  //val_e = val_t_ex / val_t_ins;
  //calcValIE();
  //calcValTempInsp();
  //e.write(val_e * 100);
  // }
  updateVariables();
}

// *volume (0,1000) ml
// *tempo inspiracao (0,5) seg (dec,2)
// *tempo expiracao inspiracao (0,5) seg (dec,2)
// *pausa inspiratoria (0,1) seg (dec,2)
// *freq respiratoria (0,30) int
// rel ins vs ex 1 : (tExp / tInsp)
/*
  [t-ins] [f-res] [t-exp] [I:E]
  [vol]
*/

void calcValFreqResp() {
  int dividor = val_t_ins + val_t_ex;
  if (dividor > 0 && val_t_ins > 0 && val_t_ex > 0) {
    val_freq_res = 6000 / dividor;
    freq_res.write(val_freq_res);
  }
}
void calcValTempEx() {
  if (val_t_ins > 0 && val_freq_res > 0) {
    val_t_ex = ((6000 - (val_t_ins * val_freq_res)) / val_freq_res);
    if (val_t_ex == 0) {
      calcValFreqResp();
    }
    else {
      t_ex.write(val_t_ex);
    }
  }
}

void calcValTempInsp() {
  if (val_t_ex > 0 && val_freq_res > 0) {
    val_t_ins = ((6000 - (val_t_ex * val_freq_res)) / val_freq_res);
    t_ins.write(val_t_ins);
  }
}
void calcValIE() {
  if (val_t_ins > 0) {
    val_e = val_t_ex / val_t_ins;
    e.write(val_e * 100);
  }
}

void updateVariables() {
  // val_e
  // last_volume = 0;
  //if(last_t_ins > 0 && last_t_ex > 0 && last_freq_res > 0){
  if (val_t_ins != last_t_ins) { //-- calc [f-res] [t-exp] [I:E]
    updateSum++;
    if (updateSum > 1) {
      calcValTempEx();
      calcValFreqResp();
      calcValIE();
    }
  }
  if (val_t_ex != last_t_ex) { //-- calc [t-ins] [f-res] [I:E]
    updateSum++;
    if (updateSum > 1) {
      calcValFreqResp();
      calcValTempInsp();
      calcValIE();
    }
  }
  if (val_freq_res != last_freq_res) { //-- calc [t-ins] [t-exp] [I:E]
    updateSum++;
    if (updateSum > 1) {
      calcValTempInsp();
      calcValTempEx();
      calcValIE();
    }
  }
  // }//if
  last_t_ins = val_t_ins;
  last_t_ex = val_t_ex;
  last_freq_res = val_freq_res;
}//
