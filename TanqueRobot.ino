#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

// ======================================================
// PINES
// ======================================================

// DRV8833
#define AIN1 13
#define AIN2 2
#define BIN1 14
#define BIN2 12

// HC-SR04
#define TRIG 5
#define ECHO 4

// ======================================================
// CONFIGURACION
// ======================================================

// Velocidad máxima de los motores (0 - 1023)
int velocidad = 800;

// Corrección para que avance derecho
const float CORRECCION_IZQ = 0.92;
const float CORRECCION_DER = 1.00;

// Modo de funcionamiento
bool modoAuto = false;

// Última distancia válida
long distanciaActual = -1;

// Failsafe para control manual
unsigned long ultimoComando = 0;
const unsigned long TIMEOUT_CONTROL = 700;

// Medición del ultrasónico
unsigned long ultimaMedicion = 0;
const unsigned long INTERVALO_SENSOR = 120;

// Distancia para detectar obstáculo
const int DISTANCIA_MINIMA = 15;

// ======================================================
// ESTADOS DEL MODO AUTOMATICO
// ======================================================

enum EstadoAuto {
  AUTO_AVANZANDO,
  AUTO_FRENANDO,
  AUTO_RETROCEDIENDO,
  AUTO_GIRANDO,
  AUTO_ESCAPANDO
};

EstadoAuto estadoAuto = AUTO_AVANZANDO;

unsigned long inicioEstado = 0;

// ======================================================
// MOTORES
// ======================================================

void motorA(int vel) {

  vel = constrain(vel, -1023, 1023);

  if (vel > 0) {
    analogWrite(AIN1, vel);
    analogWrite(AIN2, 0);
  } else if (vel < 0) {
    analogWrite(AIN1, 0);
    analogWrite(AIN2, -vel);
  } else {
    analogWrite(AIN1, 0);
    analogWrite(AIN2, 0);
  }
}

void motorB(int vel) {
  vel = constrain(vel, -1023, 1023);
  if (vel > 0) {
    analogWrite(BIN1, vel);
    analogWrite(BIN2, 0);
  } else if (vel < 0) {
    analogWrite(BIN1, 0);
    analogWrite(BIN2, -vel);
  } else {
    analogWrite(BIN1, 0);
    analogWrite(BIN2, 0);
  }
}

void mover(int izquierda, int derecha) {
  motorA(izquierda);
  motorB(derecha);
}

void adelante() {
  int velIzq = velocidad * CORRECCION_IZQ;
  int velDer = velocidad * CORRECCION_DER;

  mover(velIzq, velDer);
}

void atras() {
  mover(-velocidad, -velocidad);
}

void izquierda() {
  mover(-velocidad, velocidad);
}

void derecha() {
  mover(velocidad, -velocidad);
}

void detener() {
  mover(0, 0);
}

// ======================================================
// SENSOR ULTRASONICO
// ======================================================

long medirDistancia() {


  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  unsigned long duracion = pulseIn(ECHO, HIGH, 25000);

  // Sin eco
  if (duracion == 0) {
    return -1;
  }

  long distancia = duracion / 58;

  // Filtramos lecturas absurdas
  if (distancia < 2 || distancia > 400) {
    return -1;
  }
  return distancia;
}

// ======================================================
// MODO AUTOMATICO
// ======================================================

void modoAutonomo() {

  unsigned long ahora = millis();

  switch (estadoAuto) {

    // ==========================================
    // AVANZAR NORMALMENTE
    // ==========================================
    case AUTO_AVANZANDO:

      adelante();

      if (ahora - ultimaMedicion >= INTERVALO_SENSOR) {

        ultimaMedicion = ahora;
        distanciaActual = medirDistancia();

        Serial.print("Distancia: ");
        Serial.print(distanciaActual);
        Serial.println(" cm");

        if (distanciaActual > 0 &&
            distanciaActual <= DISTANCIA_MINIMA) {

          detener();

          Serial.println("OBSTACULO");

          estadoAuto = AUTO_FRENANDO;
          inicioEstado = ahora;
        }
      }

      break;


    // ==========================================
    // FRENAR
    // ==========================================
    case AUTO_FRENANDO:

      detener();

      if (ahora - inicioEstado >= 150) {

        estadoAuto = AUTO_RETROCEDIENDO;
        inicioEstado = ahora;
      }

      break;


    // ==========================================
    // RETROCEDER
    // ==========================================
    case AUTO_RETROCEDIENDO:

      atras();

      if (ahora - inicioEstado >= 400) {

        detener();

        estadoAuto = AUTO_GIRANDO;
        inicioEstado = ahora;
      }

      break;


    // ==========================================
    // GIRAR
    // ==========================================
    case AUTO_GIRANDO:

      Serial.println("Girando DERECHA");
      derecha();

      if (ahora - inicioEstado >= 600) {
        detener();
        estadoAuto = AUTO_ESCAPANDO;
        inicioEstado = ahora;
      }

      break;

    // ==========================================
    // ALEJARSE DEL OBSTACULO
    // ==========================================
    case AUTO_ESCAPANDO:

      adelante();

      // Avanza durante un tiempo sin comprobar
      // inmediatamente el mismo obstáculo
      if (ahora - inicioEstado >= 600) {

        estadoAuto = AUTO_AVANZANDO;

        // Fuerza una nueva medición
        ultimaMedicion = 0;
        inicioEstado = ahora;
      }

      break;
  }
}

// ======================================================
// PAGINA WEB
// ======================================================

void pagina() {

  String html = R"rawliteral(

<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport"
  content="width=device-width, initial-scale=1, user-scalable=no">

  <title>Tanque Robot</title>

<style>
* {
  box-sizing: border-box;
  -webkit-user-select: none;
  user-select: none;
}

body {
  margin: 0;
  background: #101114;
  color: white;
  font-family: Arial, sans-serif;
  text-align: center;
}

.container {
  max-width: 500px;
  margin: auto;
  padding: 20px;
}

h1 {
  font-size: 27px;
  margin-bottom: 5px;
}

.subtitulo {
  color: #888;
  margin-bottom: 20px;
}

/* ============================
   PANEL INFORMACION
   ============================ */

.panel {
  background: #1c1e23;
  border-radius: 18px;
  padding: 15px;
  margin-bottom: 20px;
}

.info {
  display: flex;
  justify-content: space-around;
}

.valor {
  font-size: 24px;
  font-weight: bold;
}

.etiqueta {
  font-size: 12px;
  color: #888;
  margin-top: 5px;
}

#estado {
  color: #4CAF50;
}

/* ============================
   JOYSTICK
   ============================ */

.joystick-container {
  margin: 25px auto;
  width: 240px;
  height: 240px;
  position: relative;
}

#joystick {
  width: 220px;
  height: 220px;
  position: absolute;
  left: 10px;
  top: 10px;
  border-radius: 50%;
  background:
    radial-gradient(
      circle,
      #282b31 0%,
      #1b1d21 70%
    );
  border: 3px solid #333;
  touch-action: none;
}

#stick {
  width: 75px;
  height: 75px;
  background: #4CAF50;
  border-radius: 50%;
  position: absolute;
  left: 72px;
  top: 72px;
  box-shadow:
    0 0 20px rgba(76,175,80,.5);
  pointer-events: none;
}

/* ============================
   VELOCIDAD
   ============================ */

.sliderBox {
  background: #1c1e23;
  border-radius: 18px;
  padding: 15px;
  margin-top: 15px;
}

input[type=range] {
  width: 90%;
  margin-top: 15px;
}

/* ============================
   BOTONES
   ============================ */

.botones {
  display: flex;
  gap: 10px;
  margin-top: 20px;
}

button {
  flex: 1;
  height: 55px;
  border: none;
  border-radius: 12px;
  font-size: 16px;
  font-weight: bold;
  color: white;
  background: #303238;
}

button:active {
  transform: scale(.96);
}

.auto {
  background: #1976D2;
}

.stop {
  width: 100%;
  margin-top: 12px;
  background: #D32F2F;
}

.activo {
  outline: 3px solid white;
}

</style>

</head>

<body>

<div class="container">

<h1>🤖 TANQUE ROBOT</h1>

<div class="subtitulo">
ESP8266 CONTROL
</div>

<!-- INFORMACION -->

<div class="panel">

<div class="info">

<div>

<div class="valor" id="distancia">
-- cm
</div>

<div class="etiqueta">
DISTANCIA
</div>
</div>
<div>

<div class="valor" id="modo">
MANUAL
</div>

<div class="etiqueta">
MODO
</div>

</div>

<div>
<div class="valor" id="estado">
●
</div>

<div class="etiqueta">
CONEXIÓN
</div>
</div>
</div>
</div>

<!-- JOYSTICK -->

<div class="joystick-container">
<div id="joystick">
<div id="stick"></div>
</div>
</div>

<!-- VELOCIDAD -->

<div class="sliderBox">

<div>
Velocidad:
<strong>
<span id="velocidadTexto">78</span>%
</strong>
</div>

<input
type="range"
id="velocidad"
min="20"
max="100"
value="78">

</div>

<!-- MODOS -->

<div class="botones">

<button
  id="btnManual"
  class="activo"
  onclick="manual()">
  MANUAL
</button>

<button
id="btnAuto"
class="auto"
onclick="autoMode()">
AUTO
</button>

</div>

<button
class="stop"
onclick="stopRobot()">
⛔ STOP
</button>

</div>

<script>

// ======================================================
// VARIABLES JOYSTICK
// ======================================================

const joystick =
document.getElementById("joystick");

const stick =
document.getElementById("stick");

let activo = false;

let joyX = 0;
let joyY = 0;

let ultimoEnvio = 0;

// ======================================================
// JOYSTICK
// ======================================================

function moverJoystick(clientX, clientY) {

  const rect =
  joystick.getBoundingClientRect();

  const centroX =
  rect.left + rect.width / 2;

  const centroY =
  rect.top + rect.height / 2;

  let dx =
  clientX - centroX;

  let dy =
  clientY - centroY;

  const radio =
  72;

  const distancia =
  Math.sqrt(dx * dx + dy * dy);

  if (distancia > radio) {

    dx =
    dx / distancia * radio;

    dy =
    dy / distancia * radio;
  }

  stick.style.transform =
  `translate(${dx}px, ${dy}px)`;

  joyX =
  Math.round(dx / radio * 100);

  joyY =
  Math.round(-dy / radio * 100);

  enviarJoystick();
}

function enviarJoystick() {

  const ahora = Date.now();

  // Limitamos cantidad de peticiones
  if (ahora - ultimoEnvio < 60) {
    return;
  }

  ultimoEnvio = ahora;

  fetch(
    `/joy?x=${joyX}&y=${joyY}`,
    {cache:"no-store"}
  ).catch(()=>{});
}

// ======================================================
// TOUCH
// ======================================================

joystick.addEventListener(
"touchstart",
function(e) {
  activo = true;
  const t = e.touches[0];
  moverJoystick(
    t.clientX,
    t.clientY
  );

  e.preventDefault();

},
{passive:false});

joystick.addEventListener(
"touchmove",
function(e) {
  if (!activo) return;
  const t = e.touches[0];
  moverJoystick(
    t.clientX,
    t.clientY
  );
  e.preventDefault();
},
{passive:false});

joystick.addEventListener(
"touchend",
function(e) {
  activo = false;
  centrarJoystick();
  e.preventDefault();
},
{passive:false});

// ======================================================
// MOUSE
// ======================================================

joystick.addEventListener(
"mousedown",
function(e) {

  activo = true;

  moverJoystick(
    e.clientX,
    e.clientY
  );
});

document.addEventListener(
"mousemove",
function(e) {

  if (!activo) return;

  moverJoystick(
    e.clientX,
    e.clientY
  );
});

document.addEventListener(
"mouseup",
function() {
  if (activo) {
    activo = false;
    centrarJoystick();
  }
});

// ======================================================
// CENTRAR
// ======================================================

function centrarJoystick() {

  joyX = 0;
  joyY = 0;

  stick.style.transform =
  "translate(0px,0px)";

  fetch(
    "/stop",
    {cache:"no-store"}
  ).catch(()=>{});
}

// ======================================================
// VELOCIDAD
// ======================================================

const slider =
document.getElementById("velocidad");

slider.addEventListener(
"input",
function() {

  document.getElementById(
    "velocidadTexto"
  ).innerText = this.value;

  fetch(
    "/velocidad?v=" + this.value,
    {cache:"no-store"}
  ).catch(()=>{});
});

// ======================================================
// MODOS
// ======================================================

function manual() {
  fetch(
    "/MANUAL",
    {cache:"no-store"}
  );

  document.getElementById(
    "modo"
  ).innerText = "MANUAL";

  document.getElementById(
    "btnManual"
  ).classList.add("activo");

  document.getElementById(
    "btnAuto"
  ).classList.remove("activo");
}

function autoMode() {
  fetch(
    "/AUTO",
    {cache:"no-store"}
  );

  document.getElementById(
    "modo"
  ).innerText = "AUTO";

  document.getElementById(
    "btnAuto"
  ).classList.add("activo");

  document.getElementById(
    "btnManual"
  ).classList.remove("activo");
}

function stopRobot() {
  fetch(
    "/stop",
    {cache:"no-store"}
  );
  manual();
  centrarJoystick();
}

// ======================================================
// DISTANCIA
// ======================================================

function actualizarDistancia() {

  fetch(
    "/distancia",
    {cache:"no-store"}
  )

  .then(response => response.text())

  .then(data => {

    let d = parseInt(data);

    if (d > 0) {

      document.getElementById(
        "distancia"
      ).innerText = d + " cm";

    } else {

      document.getElementById(
        "distancia"
      ).innerText = "-- cm";
    }

    document.getElementById(
      "estado"
    ).style.color = "#4CAF50";

  })

  .catch(() => {

    document.getElementById(
      "estado"
    ).style.color = "#D32F2F";
  });
}

setInterval(
  actualizarDistancia,
  500
);

</script>

</body>
</html>

)rawliteral";

  server.send(200, "text/html", html);
}

// ======================================================
// SETUP
// ======================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("Iniciando Tanque Robot...");

  // ----------------------------------------------------
  // Motores
  // ----------------------------------------------------
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  analogWriteRange(1023);
  detener();

  // ----------------------------------------------------
  // Sensor
  // ----------------------------------------------------
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIG, LOW);

  // ----------------------------------------------------
  // WiFi
  // ----------------------------------------------------
  WiFi.mode(WIFI_AP);
  WiFi.softAP(
    "TanqueRobot",
    "12345678"
  );

  Serial.println();
  Serial.println("WiFi creado");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // ----------------------------------------------------
  // PAGINA
  // ----------------------------------------------------

  server.on("/", pagina);

  // ----------------------------------------------------
  // CONTROL MANUAL
  // ----------------------------------------------------

  server.on("/joy", []() {

    modoAuto = false;

    int x =
    constrain(
      server.arg("x").toInt(),
      -100,
      100
    );

    int y =
    constrain(
      server.arg("y").toInt(),
      -100,
      100
    );

    // Zona muerta
    const int zonaMuerta = 8;

    if (abs(x) < zonaMuerta)
      x = 0;

    if (abs(y) < zonaMuerta)
      y = 0;

    // Mezcla diferencial
    int motorIzq = y + x;
    int motorDer = y - x;
    motorIzq = constrain(motorIzq, -100, 100);
    motorDer = constrain(motorDer, -100, 100);
    motorIzq = map(motorIzq, -100, 100, -velocidad, velocidad);
    motorDer = map(motorDer, -100, 100, -velocidad, velocidad);
    mover(motorIzq, motorDer);
    ultimoComando = millis();
    server.send(200, "text/plain", "OK");
  });

  // ----------------------------------------------------
  // STOP
  // ----------------------------------------------------

  server.on("/stop", []() {
    modoAuto = false;
    detener();
    ultimoComando = millis();
    server.send(200, "text/plain", "STOP");
  });

  // ----------------------------------------------------
  // AUTO
  // ----------------------------------------------------

  server.on("/AUTO", []() {
    detener();
    modoAuto = true;
    estadoAuto = AUTO_AVANZANDO;
    inicioEstado = millis();
    server.send(200, "text/plain", "AUTO");
  });

  // ----------------------------------------------------
  // MANUAL
  // ----------------------------------------------------

  server.on("/MANUAL", []() {
    modoAuto = false;
    detener();
    ultimoComando = millis();
    server.send(200, "text/plain", "MANUAL");
  });

  // ----------------------------------------------------
  // VELOCIDAD
  // ----------------------------------------------------

  server.on("/velocidad", []() {
    int porcentaje = constrain(server.arg("v").toInt(), 20, 100);
    velocidad = map(porcentaje, 0, 100, 0, 1023);
    Serial.print("Velocidad: ");
    Serial.println(velocidad);
    server.send(200, "text/plain", String(velocidad));
  });

  // ----------------------------------------------------
  // DISTANCIA
  // ----------------------------------------------------

  server.on("/distancia", []() {
    long d =
    medirDistancia();
    if (d > 0) {
      distanciaActual = d;
    }

    server.send(
      200,
      "text/plain",
      String(d)
    );
  });

  // ----------------------------------------------------

  server.begin();

  ultimoComando =
  millis();

  Serial.println("Servidor iniciado");
  Serial.println("Conectate a: TanqueRobot");
  Serial.println("Abrir: http://192.168.4.1");
}

// ======================================================
// LOOP
// ======================================================

void loop() {

  server.handleClient();

  if (modoAuto) {
    modoAutonomo();
  } else {
    // Failsafe
    if (millis() - ultimoComando > TIMEOUT_CONTROL) {
      detener();
    }
  }
  yield();
}