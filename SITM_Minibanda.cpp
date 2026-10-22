#include "SITM_Sistema.h"

// ==============================================================================
// FILTRO ÓPTICO INFRARROJO (Lógica específica para TCRT5000: LOW = TUBO)
// ==============================================================================
bool tuboDetectado(uint8_t pin) {
    if (digitalRead(pin) == LOW) { 
        delayMicroseconds(1000);   // Filtro anti-ruido de luz ambiente
        if (digitalRead(pin) == LOW) return true;
    }
    return false;
}

SITM_Minibanda minibanda;

void SITM_Minibanda::init(FastAccelStepperEngine& engine) {
    pinMode(PIN_TCRT1, INPUT_PULLUP);
    pinMode(PIN_TCRT2, INPUT_PULLUP);

    compuerta.attach(PIN_SERVO);
    compuerta.write(anguloLiberacion);      
    vTaskDelay(pdMS_TO_TICKS(500));         
    compuerta.detach();                     
    
    motorBanda = engine.stepperConnectToPin(PIN_STEP_BANDA);
    if (motorBanda) {
        motorBanda->setDirectionPin(PIN_DIR_BANDA);
        motorBanda->setEnablePin(PIN_ENA_BANDA);
        motorBanda->setAutoEnable(true);
        motorBanda->setSpeedInHz(estacionCNC.convertirRpmAHz(velocidadBandaRPM));
        motorBanda->setAcceleration(10000); 
        motorBanda->runForward(); 
        
        estadoBanda = 0; 
        tuboListo = false;
        //Serial.println("[BANDA] Sistema Autonomo Iniciado. Servo ABIERTO.");
    }
}

// ==============================================================================
// MÁQUINA DE ESTADOS: CON GUARDIA, ALINEACIÓN Y PAUSA DE ESTABILIZACIÓN
// ==============================================================================
void SITM_Minibanda::vigilarSensores() {
    
    // 🧠 INNOVACIÓN: Bypass de seguridad manual (Costo $0 RAM)
    // Si el operador apagó la banda, abortamos la vigilancia al instante.
    if (estadoBanda == 99) return; 

    // 1. WATCHDOG TIMER (Ahora el reposo final es el Estado 5)
    if (estadoBanda != 5 && (millis() - tiempoUltimaAccion > TIMEOUT_BANDA)) {
        estadoBanda = 0;                           
        moverServoPrueba(anguloLiberacion);        
        tiempoUltimaAccion = millis();             
        if (motorBanda && !motorBanda->isRunning()) {
            motorBanda->setSpeedInHz(estacionCNC.convertirRpmAHz(velocidadBandaRPM));
            motorBanda->runForward();              
        }
    }

    // ==========================================================================
    // 🚨 EL GUARDIA: PRIORIDAD ABSOLUTA DE TCRT2 (Aplica para Estados 0 al 3)
    // ==========================================================================
    if (estadoBanda < 4 && tuboDetectado(PIN_TCRT2)) {
        cronometroAlineacion = millis();
        estadoBanda = 4; // Saltamos directo a la alineación
        tiempoUltimaAccion = millis(); 
        redSITM.println("[BANDA] ¡Intercepcion! Tubo en TCRT2. Alineando...\r\n");
        return; 
    }

    // ==========================================================================
    // LÓGICA DEL ESCAPE DE DOBLE UÑA
    // ==========================================================================
    
    // ESTADO 0: MODO CARGA. Esperando tubo en TCRT1.
    if (estadoBanda == 0) {
        if (tuboDetectado(PIN_TCRT1)) { 
            moverServoPrueba(anguloReposo); // ZAS! Cierra uña 1, Abre uña 2
            detener();                      // 🧠 INNOVACIÓN: Frenamos la banda en seco
            cronometroPausa = millis();     // Iniciamos cronómetro de estabilización
            estadoBanda = 1;
            tiempoUltimaAccion = millis(); 
            redSITM.println("[BANDA] Tubo liberado. Banda en pausa para estabilizacion (1 seg)...\r\n");
        }
    }
    
    // ESTADO 1: 🧠 PAUSA DE ESTABILIZACIÓN ASÍNCRONA
    else if (estadoBanda == 1) {
        if (millis() - cronometroPausa >= TIEMPO_PAUSA_MS) {
            // Pasó el tiempo de estabilización, encendemos el motor suavemente
            if (motorBanda) {
                motorBanda->setSpeedInHz(estacionCNC.convertirRpmAHz(velocidadBandaRPM));
                motorBanda->runForward();
            }
            estadoBanda = 2;
            tiempoUltimaAccion = millis(); 
            redSITM.println("[BANDA] Estabilizacion completa. Motor ON. Tubo avanzando.\r\n");
        }
    }

    // ESTADO 2: ESPERANDO FLANCO DE BAJADA (El tubo empieza a avanzar)
    else if (estadoBanda == 2) {
        if (!tuboDetectado(PIN_TCRT1)) { 
            moverServoPrueba(anguloLiberacion); // Recarga la uña 1 para el siguiente tubo
            estadoBanda = 3;
            tiempoUltimaAccion = millis(); 
            redSITM.println("[BANDA] Recamara recargada. Esperando TCRT2.\r\n");
        }
    }

    // ESTADO 3: VIAJANDO (El Guardia vigila silenciosamente el TCRT2)

    // ==========================================================================
    // ESTADO 4: ALINEACIÓN ASÍNCRONA (Ajuste fino del centro)
    // ==========================================================================
    else if (estadoBanda == 4) {
        if (millis() - cronometroAlineacion >= TIEMPO_EXTRA_MS) {
            detener();                 
            tuboListo = true;          
            estadoBanda = 5;           // ESTADO 5: Reposo absoluto
            tiempoUltimaAccion = millis(); 
            redSITM.println("[BANDA] Alineacion perfecta. Esperando extraccion CNC...\r\n");
        }
    }
}

// ==============================================================================
// REINICIO DE CICLO (El CNC se llevó el tubo 1)
// ==============================================================================
void SITM_Minibanda::reanudarCiclo() {
    tuboListo = false;
    estadoBanda = 0; 
    tiempoUltimaAccion = millis(); 
    
    // SEGURO MECÁNICO: Si el Guardia interrumpió a la guillotina a la mitad de su ciclo,
    // la forzamos a abrirse (Modo Carga) para asegurar que el siguiente tubo pueda entrar.
    moverServoPrueba(anguloLiberacion); 
    
    if (motorBanda) {
        motorBanda->setSpeedInHz(estacionCNC.convertirRpmAHz(velocidadBandaRPM));
        motorBanda->runForward();
    }
    redSITM.println("[BANDA] CNC retiro el tubo. Motor ON.\r\n");
}

// ==============================================================================
// CONTROLES DE HARDWARE Y SERVO
// ==============================================================================
void SITM_Minibanda::actualizarVelocidadRPM(int rpm) {
    velocidadBandaRPM = rpm;
    if (motorBanda) {
        motorBanda->setSpeedInHz(estacionCNC.convertirRpmAHz(velocidadBandaRPM));
        motorBanda->applySpeedAcceleration(); 
    }
}

void SITM_Minibanda::detener() {
    if (motorBanda) motorBanda->forceStop(); 
}

void SITM_Minibanda::moverServoPrueba(int angulo) {
    compuerta.attach(PIN_SERVO);
    compuerta.write(angulo);
    vTaskDelay(pdMS_TO_TICKS(400)); 
    compuerta.detach();             
}

// ==============================================================================
// NUEVOS CONTROLES MANUALES HMI (Control del Watchdog)
// ==============================================================================
void SITM_Minibanda::detenerManual() {
    if (motorBanda) motorBanda->forceStop(); 
    estadoBanda = 99; // Estado 99: Apagado Manual (Desactiva Watchdog)
    redSITM.println("[BANDA] Apagado MANUAL (DB). Sistema de auto-sanacion suspendido.\r\n");
}

void SITM_Minibanda::arrancarManual() {
    estadoBanda = 0; 
    tiempoUltimaAccion = millis(); 
    if (motorBanda) {
        motorBanda->setSpeedInHz(estacionCNC.convertirRpmAHz(velocidadBandaRPM));
        motorBanda->runForward();
    }
    redSITM.println("[BANDA] Encendido MANUAL (EB). Vigilancia restaurada.\r\n");
}