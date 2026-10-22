#include "SITM_Sistema.h"

// ==============================================================================
// FILTROS ANTI-RUIDO POR SOFTWARE (Lógica Invertida: HIGH = TOCADO)
// ==============================================================================
bool sensorTocado(uint8_t pin) {
    if (digitalRead(pin) == HIGH) { 
        delayMicroseconds(1000);   // Filtro Debounce 1ms
        if (digitalRead(pin) == HIGH) return true;
    }
    return false;
}

// ==============================================================================
// CONFIGURACIÓN Y MEMORIA EN RPM / 3200 PPR
// ==============================================================================
void SITM_CNC::configurarMotor(FastAccelStepper* &motor, uint8_t stepPin, uint8_t dirPin, uint8_t enPin, int speedRPM, uint32_t accel) {
    motor = engine.stepperConnectToPin(stepPin);
    if (motor) {
        motor->setDirectionPin(dirPin);
        motor->setEnablePin(enPin);
        motor->setAutoEnable(true);
        motor->setSpeedInHz(convertirRpmAHz(speedRPM)); 
        motor->setAcceleration(accel);
    }
}

void SITM_CNC::cargarConfiguracion() {
    memoria.begin("sitm_config", false);
    
    // Leemos la memoria
    velocidadX = memoria.getInt("velX", 400); 
    velocidadY = memoria.getInt("velY", 400);
    velocidadZ = memoria.getInt("velZ", 300);
    
    // --- SEGURO ANTI-CERO (Protección de Memoria Corrupta) ---
    if (velocidadX <= 0) velocidadX = 400;
    if (velocidadY <= 0) velocidadY = 400;
    if (velocidadZ <= 0) velocidadZ = 300;

    accelX = memoria.getInt("accX", 24000); 
    accelY = memoria.getInt("accY", 40000);
    accelZ = memoria.getInt("accZ", 40000);

    if (accelX <= 0) accelX = 24000;
    if (accelY <= 0) accelY = 40000;
    if (accelZ <= 0) accelZ = 40000;
    // ---------------------------------------------------------
    
    pasosRecogerZ = memoria.getInt("pasosZ", 11500); 
    pasosDejarZ = memoria.getInt("pasosDZ", 12500);
    posPruebaX = memoria.getInt("posX", 50800);
    posPruebaY = memoria.getInt("posY", 28000);

    // Las posiciones X/Y sí pueden ser 0
    rack1X = memoria.getInt("r1X", 0);
    rack1Y = memoria.getInt("r1Y", 26800);
    pasoX = memoria.getInt("pX", 3200); 
    pasoY = memoria.getInt("pY", 2400); 
    
    memoria.end();
}

void SITM_CNC::guardarConfiguracion() {
    memoria.begin("sitm_config", false);
    memoria.putInt("velX", velocidadX);  memoria.putInt("velY", velocidadY);  memoria.putInt("velZ", velocidadZ);
    memoria.putInt("accX", accelX);      memoria.putInt("accY", accelY);      memoria.putInt("accZ", accelZ);
    memoria.putInt("pasosZ", pasosRecogerZ); memoria.putInt("pasosDZ", pasosDejarZ);
    memoria.putInt("posX", posPruebaX);  memoria.putInt("posY", posPruebaY);
    memoria.putInt("r1X", rack1X);       memoria.putInt("r1Y", rack1Y);
    memoria.putInt("pX", pasoX);         memoria.putInt("pY", pasoY);
    memoria.end();
    redSITM.println("--> OK: Configuracion guardada en Flash NVRAM.\r\n");
}

void SITM_CNC::init() {
    pinMode(X_LIMIT_PIN, INPUT_PULLUP);
    pinMode(Y_LIMIT_PIN, INPUT_PULLUP);
    pinMode(Z_LIMIT_PIN, INPUT_PULLUP);
    pinMode(VALVULA_1, OUTPUT);
    pinMode(VALVULA_2, OUTPUT);
    
    apagarGripper(); 
    cargarConfiguracion();
    engine.init();
    
    configurarMotor(stepperX, X_STEP_PIN, X_DIR_PIN, X_ENABLE_PIN, velocidadX, accelX);
    configurarMotor(stepperY, Y_STEP_PIN, Y_DIR_PIN, Y_ENABLE_PIN, velocidadY, accelY);
    configurarMotor(stepperZ, Z_STEP_PIN, Z_DIR_PIN, Z_ENABLE_PIN, velocidadZ, accelZ);
    
    //Serial.println("[SISTEMA] Nucleo CNC (3200 PPR - Predictive Homing 3D) Iniciado.");
}

// ==============================================================================
// CONTROL NEUMÁTICO
// ==============================================================================
void SITM_CNC::abrirGripper() {
    digitalWrite(VALVULA_1, LOW);
    digitalWrite(VALVULA_2, HIGH);
    vTaskDelay(pdMS_TO_TICKS(250)); 
}

void SITM_CNC::cerrarGripper() {
    digitalWrite(VALVULA_1, HIGH);
    digitalWrite(VALVULA_2, LOW);
    vTaskDelay(pdMS_TO_TICKS(250)); 
}

void SITM_CNC::apagarGripper() {
    digitalWrite(VALVULA_1, HIGH); 
    digitalWrite(VALVULA_2, HIGH); 
}

// ==============================================================================
// BANDERAS DE SEGURIDAD (GUARDAESPALDAS EN TIEMPO REAL)
// ==============================================================================
void SITM_CNC::moverZEsperar(int destinoZ) {
    stepperZ->moveTo(destinoZ);
    while(stepperZ->isRunning()) { 
        if (stepperZ->getCurrentPosition() > destinoZ && sensorTocado(Z_LIMIT_PIN)) {
            stepperZ->forceStopAndNewPosition(0);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(2)); 
    }
}

void SITM_CNC::moverXYEsperar(int destinoX, int destinoY) {
    stepperX->moveTo(destinoX);
    stepperY->moveTo(destinoY);
    while(stepperX->isRunning() || stepperY->isRunning()) { 
        if (stepperX->isRunning() && stepperX->getCurrentPosition() > destinoX && sensorTocado(X_LIMIT_PIN)) {
            stepperX->forceStopAndNewPosition(0);
            redSITM.println("[ALERTA] Sensor X interceptado. Evadiendo colision.\r\n");
        }
        if (stepperY->isRunning() && stepperY->getCurrentPosition() > destinoY && sensorTocado(Y_LIMIT_PIN)) {
            stepperY->forceStopAndNewPosition(0);
            redSITM.println("[ALERTA] Sensor Y interceptado. Evadiendo colision.\r\n");
        }
        vTaskDelay(pdMS_TO_TICKS(2)); 
    }
}

// ==============================================================================
// CALIBRACIÓN HÍBRIDA PREDICTIVA 3D
// ==============================================================================
void SITM_CNC::homeAxis() {
    maquinaEnReposo = false; 

    // --- FASE 1: Z SUBE A 0 CON PERFIL TRAPEZOIDAL RÁPIDO ---
    stepperZ->moveTo(0); 
    while (stepperZ->isRunning()) { 
        if (sensorTocado(Z_LIMIT_PIN)) {
            stepperZ->forceStopAndNewPosition(0);
            break;
        }
        vTaskDelay(1); 
    }

    if (!sensorTocado(Z_LIMIT_PIN)) {
        stepperZ->setSpeedInHz(convertirRpmAHz(velocidadZ / 4)); 
        stepperZ->runBackward();
        while (!sensorTocado(Z_LIMIT_PIN)) { vTaskDelay(1); }
        stepperZ->forceStopAndNewPosition(0);
        stepperZ->setSpeedInHz(convertirRpmAHz(velocidadZ)); 
    }

    // --- FASE 2: DIAGONAL X/Y A MÁXIMA VELOCIDAD ---
    stepperX->moveTo(0);     
    stepperY->moveTo(0);     

    while (stepperX->isRunning() || stepperY->isRunning()) {
        if (stepperX->isRunning() && sensorTocado(X_LIMIT_PIN)) { 
            stepperX->forceStopAndNewPosition(0);
        }
        if (stepperY->isRunning() && sensorTocado(Y_LIMIT_PIN)) { 
            stepperY->forceStopAndNewPosition(0);
        }
        vTaskDelay(1); 
    }

    // --- FASE 3: BÚSQUEDA FINA INDEPENDIENTE X e Y ---
    if (!sensorTocado(X_LIMIT_PIN)) {
        stepperX->setSpeedInHz(convertirRpmAHz(velocidadX / 4)); 
        stepperX->runBackward();
        while (!sensorTocado(X_LIMIT_PIN)) { vTaskDelay(1); }
        stepperX->forceStopAndNewPosition(0);
        stepperX->setSpeedInHz(convertirRpmAHz(velocidadX)); 
    }

    if (!sensorTocado(Y_LIMIT_PIN)) {
        stepperY->setSpeedInHz(convertirRpmAHz(velocidadY / 4)); 
        stepperY->runBackward();
        while (!sensorTocado(Y_LIMIT_PIN)) { vTaskDelay(1); }
        stepperY->forceStopAndNewPosition(0);
        stepperY->setSpeedInHz(convertirRpmAHz(velocidadY)); 
    }

    apagarGripper(); 
    maquinaEnReposo = true; 
}