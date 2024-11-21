#include <Wire.h>
#include <MPU6050.h>
#include <TimerThree.h>  // Añadimos la librería TimerThree

// Configuración de pines BPM
const int ecgPin = A0;          // El AD8232 está conectado al pin A0 (Entrada analógica)
const int ledPin = LED_BUILTIN; // LED incorporado para indicar cuando detectamos un latido

// Variables del ECG
unsigned int buffer1[100];            // Buffer para almacenar datos del ECG
unsigned int buffer2[100];            // Segundo buffer para transferencia
volatile bool bufferFlag = false;     // Indica si hay datos listos para procesar

int threshold_BPM = 520;              // Umbral para detectar los picos (ajustar según tu sensor)
int bpm = 0;                          // Variable para almacenar los BPM
unsigned long lastBeatTime = 0;       // Tiempo en milisegundos del último latido detectado
unsigned long currentTime_BPM;        // Tiempo actual en milisegundos
bool imprimirBPM = false;

int beatInterval[10];                 // Matriz para almacenar los últimos 10 intervalos entre latidos
int beatCount = 0;                    // Contador de latidos válidos registrados
unsigned long totalInterval = 0;      // Suma acumulada de los intervalos
volatile unsigned int pos = 0;        // Posición para almacenar en el buffer

// Configuración de pines RPM
MPU6050 mpu;

// Variables para el cálculo de respiración
int16_t ax, ay, az;
float threshold_RPM = 0.025;          // Umbral para detectar respiración (en unidades de gravedad)
unsigned long lastBreathTime = 0;
int breathCount = 0;
float lastAz = 0;
unsigned long currentTime_RPM;

// Variables para el monitoreo de la frecuencia respiratoria
const unsigned long monitoringInterval = 60000; // Intervalo de monitoreo de 60 segundos
unsigned long startTime = 0;
int respiratoryRateThreshold = 20;    // Límite de frecuencia respiratoria (respiraciones por minuto)
int respiratoryRate = 0;              // Variable para almacenar la frecuencia respiratoria

// Configuración de pines de la válvula y bomba
const int valvulaPin = 8;  // Pin de la válvula
const int bombaPin = 9;    // Pin de la bomba (relay)
bool activarBombaBPM = false;
bool activarBombaRPM = false;
bool cicloEnCurso = false; // Indica si el ciclo de la bomba está en curso

// Variables para controlar el ciclo de la bomba
enum CicloFase { FASE_INACTIVA, FASE_INFLADO, FASE_DESINFLADO };
CicloFase faseActual = FASE_INACTIVA;
unsigned long faseInicio = 0;

// Declaración de la interrupción
void rts();

void setup() {
  // Configuración inicial BPM
  Serial.begin(115200);            // Inicializa la comunicación serie
  pinMode(ledPin, OUTPUT);         // Configura el pin del LED como salida
  Serial.println("Iniciando monitor de frecuencia cardíaca con AD8232...");

  // Configuración inicial RPM
  Wire.begin();
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 conectado exitosamente.");
  } else {
    Serial.println("No se pudo conectar con el MPU6050.");
  }
  startTime = millis();

  // Configuración inicial de la válvula y bomba
  pinMode(valvulaPin, OUTPUT);
  pinMode(bombaPin, OUTPUT);
  Serial.println("Prueba de Bomba y Válvula con control de tiempo");

  // Configuración del Timer3 para lectura de ECG
  Timer3.initialize(5000);       // Configura interrupción cada 5 ms
  Timer3.attachInterrupt(rts);   // Asocia la interrupción
}

void loop() {
  loop_BPM();   // Ejecutar el monitoreo de BPM
  loop_RPM();   // Ejecutar el monitoreo de frecuencia respiratoria

  // Si se cumplen las condiciones, activar la bomba y la válvula
  if (activarBombaBPM && activarBombaRPM && !cicloEnCurso) {
    activarBombaBPM = false;
    activarBombaRPM = false;
    activarBombaYValvula();
  }

  // Manejo del ciclo de la bomba
  if (cicloEnCurso) {
    unsigned long tiempoTranscurrido = millis() - faseInicio;

    if (faseActual == FASE_INFLADO && tiempoTranscurrido >= 15000) {
      // Pasar a fase de desinflado después de 20 segundos
      Serial.println("Apagando bomba, manteniendo válvula cerrada");
      digitalWrite(bombaPin, LOW);    // Apagar bomba
      faseActual = FASE_DESINFLADO;
      faseInicio = millis();          // Reiniciar el tiempo de la fase
    } else if (faseActual == FASE_DESINFLADO && tiempoTranscurrido >= 40000) {
      // Finalizar ciclo después de 40 segundos adicionales (total 60 segundos)
      Serial.println("Abriendo válvula y reiniciando mediciones");
      digitalWrite(valvulaPin, LOW);  // Abrir válvula

      // Reiniciar variables y asegurar que las mediciones se reanuden
      cicloEnCurso = false;
      faseActual = FASE_INACTIVA;
      // No es necesario reiniciar las variables de BPM y RPM aquí
      Serial.println("Mediciones de BPM y RPM continúan.");
    }
  }
}

void loop_BPM() {
  // Procesa los datos del buffer si está lleno
  if (bufferFlag) {
    bufferFlag = false;

    for (int i = 0; i < 100; i++) {
      int signal = buffer2[i];  // Obtén la señal analógica desde el buffer

      // Detecta picos R
      if (signal > threshold_BPM) {
        currentTime_BPM = millis(); // Tiempo actual en milisegundos

        // Calcula el intervalo entre latidos
        unsigned long interval = currentTime_BPM - lastBeatTime;

        // Filtra latidos demasiado rápidos o lentos (30 BPM - 200 BPM)
        if (interval > 300 && interval < 2000) {
          lastBeatTime = currentTime_BPM;

          // Parpadea el LED para indicar detección de latido
          digitalWrite(ledPin, HIGH);
          delay(50);
          digitalWrite(ledPin, LOW);

          // Almacena el intervalo y calcula BPM
          if (beatCount < 10) { // Llenar matriz inicialmente
            beatInterval[beatCount++] = interval;
            totalInterval += interval;
          } else { // Reemplazo circular en la matriz
            totalInterval -= beatInterval[beatCount % 10];
            beatInterval[beatCount % 10] = interval;
            totalInterval += interval;
            beatCount++;
          }

          bpm = 60000 / (totalInterval / min(beatCount, 10));
          imprimirBPM = true;

          // Verificar si la frecuencia cardíaca excede los 80 BPM y la bomba no está en curso
          if (bpm > 80 && !cicloEnCurso) {
            activarBombaBPM = true;
            Serial.println("Frecuencia cardíaca supera los 80 BPM!");
          }
        }
      }
    }
  }

  // Imprime los BPM si hay nuevos valores
  if (imprimirBPM) {
    Serial.print("Frecuencia cardíaca: ");
    Serial.print(bpm);
    Serial.println(" latidos por minuto");
    imprimirBPM = false;
  }
}

void rts() {
  // Lee el valor analógico y lo almacena en el buffer
  buffer1[pos++] = analogRead(ecgPin);

  if (pos == 100) { // Cuando se llena el buffer, transfiere los datos
    for (unsigned int i = 0; i < 100; i++) {
      buffer2[i] = buffer1[i];
    }
    bufferFlag = true; // Indica que los datos están listos
    pos = 0;
  }
}

void loop_RPM() {
  if (!cicloEnCurso) {
    // Leer valores del acelerómetro
    mpu.getAcceleration(&ax, &ay, &az);

    // Convertir az a unidades de gravedad (1g = 9.81 m/s^2)
    float az_g = az / 16384.0;

    // Detectar si hay un cambio significativo en la aceleración Z
    currentTime_RPM = millis();
    if (abs(az_g - lastAz) > threshold_RPM) {
      // Detectar inhalación o exhalación cuando se excede el umbral
      if (currentTime_RPM - lastBreathTime > 1500) {  // Limitar la detección a un intervalo de al menos 1.5 segundos para evitar falsos positivos
        breathCount++;
        lastBreathTime = currentTime_RPM;
      }
    }

    // Calcular la frecuencia respiratoria cada 60 segundos
    if (currentTime_RPM - startTime >= monitoringInterval) {
      // Calcular la frecuencia respiratoria en respiraciones por minuto
      respiratoryRate = (breathCount * 60) / ((currentTime_RPM - startTime) / 1000);
      Serial.print("Frecuencia respiratoria: ");
      Serial.print(respiratoryRate);
      Serial.println(" respiraciones por minuto");

      // Detectar si la frecuencia respiratoria excede el umbral definido
      if (respiratoryRate > respiratoryRateThreshold && !cicloEnCurso) {
        Serial.println("Frecuencia respiratoria supera los 20 RPM!");
        activarBombaRPM = true;
      }

      // Reiniciar el contador de respiraciones y el tiempo de inicio para el siguiente minuto
      breathCount = 0;
      startTime = currentTime_RPM;
    }

    // Actualizar el último valor de aceleración Z
    lastAz = az_g;

    // Pequeña pausa para evitar sobrecargar la lectura
    delay(100);
  }
}

void activarBombaYValvula() {
  cicloEnCurso = true; // Indicar que el ciclo de la bomba ha iniciado
  faseActual = FASE_INFLADO;
  faseInicio = millis();

  // Paso 1: Cierro válvula y activo bomba
  Serial.println("Inicio inflado");
  Serial.println("Cierro válvula y activo bomba");
  digitalWrite(valvulaPin, HIGH); // Cerrar válvula
  digitalWrite(bombaPin, HIGH);   // Encender bomba
}

