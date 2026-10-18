# Tanque Robot WiFi con ESP8266

Tanque robot controlado desde el navegador del celular, con joystick virtual, velocidad regulable y un modo autónomo que detecta obstáculos mediante un sensor ultrasónico HC-SR04.

<img width="601" height="338" alt="Miniatura-tanque2 - copia" src="https://github.com/user-attachments/assets/f85af5a6-a5dc-457d-b2bf-c6eb66a11255" />

El ESP8266 crea su propia red WiFi y aloja la interfaz de control. No necesita una aplicación instalada, router ni conexión a Internet para funcionar.

Este documento describe el comportamiento de `TanqueRobot.ino`.

## Funciones

- Control manual con joystick táctil o mouse y mezcla diferencial de dos motores.
- Regulación de velocidad entre 20 % y 100 % desde la interfaz.
- Modo automático con retroceso y giro a la derecha al detectar obstáculos.
- Visualización de distancia e indicador de conexión.
- Botón **STOP** para detener los motores y volver al modo manual.
- Detención en modo manual después de 700 ms sin recibir comandos de movimiento.
- Interfaz HTML, CSS y JavaScript incluida en el sketch.

## Componentes

| Cantidad | Componente |
| --- | --- |
| 1 | Placa ESP8266, como Wemos D1 mini o NodeMCU |
| 1 | Driver de dos motores DRV8833 |
| 2 | Motores DC con reductora, compatibles con la alimentación elegida |
| 1 | Sensor ultrasónico HC-SR04 |
| 1 | Chasis de tanque con ruedas/orugas |
| 1 | Alimentación para motores y alimentación regulada para electrónica |
| 2 | Resistencias para adaptar ECHO: 1 kΩ y 2 kΩ |
| — | Cables, conectores y cable USB de datos |

La capacidad y la tensión de la batería dependen de los motores y de la placa utilizada.

## Conexiones

Los números del código corresponden a **GPIO**, no a la numeración física de los pines. Las etiquetas D indicadas corresponden a placas D1 mini/NodeMCU habituales.

### ESP8266 → DRV8833

| Señal del driver | GPIO ESP8266 | Etiqueta de placa | Función |
| --- | --- | --- | --- |
| AIN1 | 13 | D7 | Motor izquierdo, entrada 1 |
| AIN2 | 2 | D4 | Motor izquierdo, entrada 2 |
| BIN1 | 14 | D5 | Motor derecho, entrada 1 |
| BIN2 | 12 | D6 | Motor derecho, entrada 2 |
| GND | GND | GND | Tierra común |

Conectar el motor izquierdo a las salidas del canal A y el derecho a las del canal B. Los nombres impresos de las salidas pueden variar según el módulo.

- Conectar VM/VIN del driver a la fuente adecuada para los motores.
- Unir GND del ESP8266, driver, sensor y fuentes de alimentación.
- Si el módulo expone `nSLEEP`, debe estar en nivel alto para habilitarlo; comprobar el pinout del módulo concreto. El programa no controla esta señal.
- GPIO2/D4 interviene en el arranque del ESP8266: el circuito conectado no debe forzarlo a nivel bajo durante el encendido.

### ESP8266 → HC-SR04

| Señal del sensor | Conexión |
| --- | --- |
| VCC | 5 V regulados para un HC-SR04 estándar |
| GND | Tierra común |
| TRIG | GPIO5 / D1 |
| ECHO | GPIO4 / D2, mediante adaptación a 3,3 V |

**No conectar ECHO de 5 V directamente al ESP8266.** Para el divisor resistivo, conectar una resistencia de 1 kΩ entre ECHO y GPIO4, y otra de 2 kΩ entre GPIO4 y GND. Esto reduce aproximadamente 5 V a 3,3 V.

### Alimentación

No alimentar los motores desde el pin 3V3 del ESP8266. Usar una fuente capaz de soportar la corriente de arranque de los motores y una alimentación regulada adecuada para la placa. Una celda de litio de 3,7 V nominales alcanza aproximadamente 4,2 V cargada: no conectarla directamente a 3V3. Para la entrada USB/5 V de una placa de desarrollo, utilizar una alimentación regulada de 5 V y comprobar las especificaciones de esa placa.

## Instalación y carga

1. Instalar Arduino IDE.
2. En **Preferencias → URLs adicionales del Gestor de placas**, agregar:

   ```text
   https://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```

3. En el Gestor de placas, instalar **esp8266 by ESP8266 Community**.
4. Seleccionar la placa correspondiente al hardware, por ejemplo **LOLIN(WEMOS) D1 R2 & mini** o **NodeMCU 1.0 (ESP-12E Module)**.
5. Guardar el sketch dentro de una carpeta llamada `TanqueRobot`, con el archivo `TanqueRobot.ino` en su interior, y abrirlo en Arduino IDE.
6. Conectar la placa con un cable USB de datos, seleccionar el puerto y cargar el programa.
7. Abrir el Monitor Serie a **115200 baudios** para ver el inicio y la dirección IP.

Las bibliotecas `ESP8266WiFi.h` y `ESP8266WebServer.h` vienen con el paquete de placas ESP8266. No se requieren bibliotecas externas adicionales para este sketch.

Referencia: [instalación oficial del core ESP8266 para Arduino](https://arduino-esp8266.readthedocs.io/en/latest/installing.html).

## Conexión desde el celular

1. Encender el tanque.
2. Conectarse a la red WiFi **TanqueRobot**.
3. Ingresar la contraseña **12345678**.
4. Si el teléfono avisa que la red no tiene Internet, mantener la conexión.
5. Abrir [http://192.168.4.1](http://192.168.4.1) en el navegador.

La IP también se imprime en el Monitor Serie. Las credenciales se pueden cambiar en `WiFi.softAP()` dentro de `setup()`.

## Uso

### Modo manual

El robot inicia detenido y en modo manual. Desplazar el joystick hacia arriba para avanzar, hacia abajo para retroceder y hacia los lados para girar. Las diagonales combinan avance o retroceso con giro. Al soltarlo se envía una orden de parada.

El deslizador establece la velocidad máxima; el desplazamiento del joystick determina la potencia aplicada dentro de ese límite. El botón **MANUAL** desactiva el modo autónomo y detiene el robot.

**Comportamiento de esta versión:** el navegador envía comandos al tocar o mover el joystick, pero no los repite mientras permanece inmóvil. Por eso, si se sostiene en una posición fija, el robot puede detenerse después de 700 ms. Esto se debe al temporizador de seguridad implementado en el firmware.

### Modo automático

Al pulsar **AUTO**, el robot comienza a avanzar. Cuando mide una distancia válida de 15 cm o menos, ejecuta esta secuencia:

| Etapa | Acción | Duración aproximada |
| --- | --- | --- |
| Frenando | Detiene los motores | 150 ms |
| Retrocediendo | Retrocede | 400 ms |
| Girando | Gira a la derecha sobre su eje | 600 ms |
| Escapando | Avanza | 600 ms |
| Avanzando | Retoma el avance y la detección | Hasta el próximo obstáculo |

La lógica de evasión comprueba el sensor cada 120 ms durante el estado de avance normal. El giro siempre es hacia la derecha y su ángulo depende de la velocidad, la superficie y la mecánica.

Pulsar **STOP** para detenerlo y volver a manual. Enviar un comando del joystick también cancela el modo autónomo.

### Distancia y conexión

La página consulta la distancia cada 500 ms. Muestra `-- cm` cuando la medición no es válida. El indicador cambia a verde al recibir una respuesta de distancia y a rojo si falla esa consulta; no comprueba directamente el estado de los motores.

## Parámetros del código

| Parámetro | Valor inicial | Descripción |
| --- | --- | --- |
| `velocidad` | `800` | PWM inicial sobre 1023, aproximadamente 78 % |
| `CORRECCION_IZQ` | `0.92` | Factor aplicado al motor izquierdo en `adelante()` |
| `CORRECCION_DER` | `1.00` | Factor aplicado al motor derecho en `adelante()` |
| `TIMEOUT_CONTROL` | `700` ms | Tiempo sin comandos antes de detenerse en manual |
| `INTERVALO_SENSOR` | `120` ms | Intervalo de detección en avance autónomo |
| `DISTANCIA_MINIMA` | `15` cm | Umbral para iniciar la evasión |
| `zonaMuerta` | `8` | Valores del joystick con magnitud menor a 8 se anulan |

Las correcciones izquierda/derecha se aplican únicamente en `adelante()`, usada por el modo automático. No se aplican a la mezcla del joystick, al retroceso ni a los giros.

`medirDistancia()` espera el eco hasta 25 000 µs y devuelve `-1` si no recibe eco o si la distancia calculada queda fuera del rango de 2 a 400 cm.

## Rutas de control web

La interfaz utiliza peticiones HTTP GET al servidor del ESP8266, en el puerto 80. Las rutas distinguen mayúsculas y minúsculas.

| Ruta | Parámetros | Resultado |
| --- | --- | --- |
| `/` | — | Devuelve la interfaz web |
| `/joy` | `x`, `y`: de -100 a 100 | Activa manual, mueve los motores y devuelve `OK` |
| `/stop` | — | Detiene el robot, activa manual y devuelve `STOP` |
| `/AUTO` | — | Inicia el modo autónomo y devuelve `AUTO` |
| `/MANUAL` | — | Activa manual, detiene los motores y devuelve `MANUAL` |
| `/velocidad` | `v`: de 20 a 100 | Ajusta la velocidad y devuelve el valor PWM |
| `/distancia` | — | Realiza una medición y devuelve centímetros o `-1` |

## Alcance y limitaciones

- El temporizador de parada de 700 ms funciona solamente en modo manual. El modo automático continúa aunque se desconecte el celular.
- Una lectura ultrasónica inválida no provoca una parada en automático: el robot sigue avanzando.
- Durante retroceso, giro y escape, las lecturas del sensor no interrumpen la maniobra, aunque la página siga consultando la distancia.
- El sensor frontal no detecta caídas ni protege la parte trasera. Hacer las pruebas en un área despejada, lejos de escaleras y bordes.
- El estado visual de los botones no se sincroniza con el firmware mediante una consulta de modo; puede quedar desactualizado, por ejemplo al cancelar AUTO con el joystick.
- Las rutas HTTP no tienen autenticación adicional a la contraseña de la red WiFi.

## Solución de problemas

| Problema | Qué revisar |
| --- | --- |
| No aparece la red WiFi | Alimentación, carga del sketch y mensajes a 115200 baudios |
| El celular no abre la página | Conexión a `TanqueRobot`, permanencia en la red sin Internet y uso de `http://192.168.4.1` |
| Los motores no se mueven | Alimentación del driver, tierra común, cableado, soldaduras y habilitación de `nSLEEP` si está expuesto |
| Un motor gira al revés | Con el equipo apagado, invertir los dos cables de ese motor |
| Funciona por USB pero falla con batería | Tensión regulada, corriente disponible y caídas de tensión al arrancar los motores |
| El ESP8266 no arranca | Revisar que el circuito conectado a GPIO2/D4 no fuerce un nivel bajo durante el arranque |
| La distancia muestra `-- cm` | Alimentación del sensor, TRIG/ECHO, divisor resistivo y posición del objeto |
| Se detiene con el joystick sostenido | El navegador no reenvía comandos mientras el joystick permanece inmóvil; actúa el timeout de 700 ms |
| Se desvía al avanzar en AUTO | Ajustar los factores de corrección y comprobar rozamiento, motores y transmisión |

## Archivos del proyecto

- `TanqueRobot.ino`: firmware, servidor web e interfaz de control.
- `README.md`: componentes, conexiones, instalación y funcionamiento.

La documentación se elaboró mediante revisión del sketch. No implica una prueba de compilación ni una validación física del montaje.
