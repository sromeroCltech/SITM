#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include "FastAccelStepper.h"
#include <Preferences.h>
#include <RoboServo.h>

#define MAX_CLIENTS 2
// ==========================================
// 1. MAPA DE HARDWARE CNC (3 Motores, 2 Sensores)
// ==========================================
#define X_STEP_PIN 4
#define X_DIR_PIN 5
#define X_ENABLE_PIN 6
#define X_LIMIT_PIN 11 // Sensor Óptico de Herradura (LOW = Tocado)

#define Y_STEP_PIN 7
#define Y_DIR_PIN 19
#define Y_ENABLE_PIN 20
#define Y_LIMIT_PIN 12 // Ajusta este número al pin real que conectaste
// Eje Y se maneja puramente matemático (Ahorro de sensor y cableado)

#define Z_STEP_PIN 8
#define Z_DIR_PIN 9
#define Z_ENABLE_PIN 10
#define Z_LIMIT_PIN 13 // Sensor Óptico de Herradura (LOW = Tocado)

#define VALVULA_1 15 
#define VALVULA_2 16 

// --- PINES DE LA MINIBANDA ---
#define PIN_STEP_BANDA 39
#define PIN_DIR_BANDA 40
#define PIN_ENA_BANDA 41

#define PIN_TCRT1 17 // Ajusta a los pines que vayas a usar
#define PIN_TCRT2 18
#define PIN_SERVO 14

// --- TRADUCCIÓN MECÁNICA (Costo Cero) ---
#define PPR 3200 // Resolución de los drivers. 3200 = Suavidad médica extrema

// ==========================================
// 2. DEFINICIÓN DE CLASES Y MÓDULOS
// ==========================================
class SITM_CNC {
public:
    void init();
    void homeAxis();            
    void procesarComandoSerial(String comando);
    void moverZEsperar(int destinoZ);
    void moverXYEsperar(int destinoX, int destinoY);
    void abrirGripper();
    void cerrarGripper();
    void apagarGripper(); 

    void cargarConfiguracion();
    void guardarConfiguracion();
    
    // Función Inline superrápida para traducir RPM humanos a Hz del Driver
    uint32_t convertirRpmAHz(int rpm) {
        return (uint32_t)((abs(rpm) * PPR) / 60.0);
    }
    
    bool maquinaEnReposo = true; 
    FastAccelStepperEngine& obtenerEngine() { return engine; }
    
    // Variables de configuración de cinemática
    int velocidadX, velocidadY, velocidadZ; // AHORA GUARDAN RPM
    int accelX, accelY, accelZ; 
    int pasosRecogerZ, pasosDejarZ;
    int posPruebaX, posPruebaY;
    int rack1X, rack1Y, pasoX, pasoY;   

private:
    FastAccelStepperEngine engine;
    FastAccelStepper *stepperX = NULL;
    FastAccelStepper *stepperY = NULL;
    FastAccelStepper *stepperZ = NULL;
    Preferences memoria;

    void configurarMotor(FastAccelStepper* &motor, uint8_t stepPin, uint8_t dirPin, uint8_t enPin, int speedRPM, uint32_t accel);
};

class SITM_Rutinas {
public:
    // La única "puerta de entrada" pública para los comandos del Telnet
    void procesarRutina(String comando); 

    // 🧠 INNOVACIÓN: MATRICES DINÁMICAS EN MEMORIA RAM (Costo $0 - Cero latencia)
    // Al estar aquí, toda la máquina las lee a velocidad de procesador y se modifican en vivo
    int coordenadasY[10] = { 12800, 11900, 10700, 9600, 8500, 7400, 6300, 5200, 4100, 3000 };
    int coordenadasX_G1[5] = {0, 1100, 2200, 3250, 4350}; 
    int coordenadasX_G2[5] = {6900, 8000, 9100, 10200, 11200};
    int coordenadasX_G3[5] = {13900, 14950, 16050, 17100, 18200}; 
    int coordenadasX_G4[5] = {20800, 21900, 23000, 24050, 25400}; 

private:
    // Los motores de trabajo internos (Invisibles y protegidos desde el exterior)
    void ejecutarRecoleccion(int indexTubo);
    void ejecutarTestCalibracion(); // 🧠 <--- ¡AQUÍ ESTÁ LA CORRECCIÓN DEL NOMBRE!
};

class SITM_OTA {
public:
    void init();
    void loop();
    String leerComandos();
    void print(String msg);
    void println(String msg);

private:
    WebServer server{80};
    WiFiServer tcpServer{8888};
    WiFiClient tcpClients[MAX_CLIENTS];
    int clientCount = 0;

    void conectarWiFi();
    void initOTAServer();
    void aceptarNuevasConexiones();
    void limpiarClientesDesconectados();
    
    const char* serverIndex = 
        "<form method='POST' action='/update' enctype='multipart/form-data'>"
        "<input type='file' name='update'>"
        "<input type='submit' value='Actualizar Firmware'>"
        "</form>";
};
// ==========================================
// 2. DEFINICIÓN DE CLASES
// ==========================================
class SITM_Minibanda {
public:
    void init(FastAccelStepperEngine& engine);
    void actualizarVelocidadRPM(int rpm);
    
    // --- NUEVA LÓGICA AUTÓNOMA ---
    void vigilarSensores(); // Se ejecuta siempre en segundo plano
    void reanudarCiclo();   // Arranca la banda tras recoger el tubo
    
    // --- CONTROL MANUAL Y CALIBRACIÓN ---
    void detenerManual();  // <--- NUEVA
    void arrancarManual(); // <--- NUEVA
    void detener();
    void moverServoPrueba(int angulo);
    
    int velocidadBandaRPM = 30; 
    int anguloReposo = 20;      // (SR) Bloqueando
    int anguloLiberacion = 180; // (SL) Dejando pasar

    bool tuboListo = false;     // Bandera para avisar al CNC
    int estadoBanda = 0;       // 0=Alimentando, 1=Viajando, 2=Detenido
// --- NUEVO: WATCHDOG DE AUTO-SANACIÓN ---
    unsigned long tiempoUltimaAccion = 0; // Memoria del cronómetro
    const unsigned long TIMEOUT_BANDA = 10000; // 10,000 milisegundos (10 seg)
    // NUEVO: Variables para el centrado perfecto del tubo
    unsigned long cronometroAlineacion = 0;
    const unsigned long TIEMPO_EXTRA_MS = 1000; // 2000 milisegundos = 2 segundos
// 🧠 NUEVO: Variables para la estabilización de caída
    unsigned long cronometroPausa = 0;
    const unsigned long TIEMPO_PAUSA_MS = 1000; // 1000 milisegundos = 1 segundo
private:
    FastAccelStepper *motorBanda = NULL;
    RoboServo compuerta; 
};
extern SITM_Minibanda minibanda;
extern SITM_CNC estacionCNC;
extern SITM_Rutinas controlRutinas;
extern SITM_OTA redSITM;
extern bool sensorTocado(uint8_t pin); // Para el CNC (HIGH = Bloqueado)
extern bool tuboDetectado(uint8_t pin); // NUEVO: Para TCRT5000 (LOW = Reflejo)