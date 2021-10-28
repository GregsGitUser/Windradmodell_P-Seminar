# Windradmodell_P-Seminar
CAD-Dateien der von mir erstellten Teile und der Code, der auf dem Arduino Nano läuft. Außerdem noch die source-Dateien für die Platine, die ich für dieses Projekt gemacht habe

Im Rahmen des P-Seminars Physik am ITG 2020-22 mit dem Thema Klimawandel

Wir wollten ein Windradmodell bauen und an diesem verschiedene Messungen durchführen. Als Zusatzfunktion sollte es noch der Windrichtung folgen,
was durch einen Schrittmotor (NEMA 17-03 mit 20Nm und 0,2A) in der Basis des Windrads und einen Zweipunktregler möglich wurde. Zur Ansteuerung wird ein A4988 Treiber verwendet.
Der Microcontroller ist ein Arduino Nano, der als Interface einen Drehenkoder (KY-040) und Kippschalter hat. So kann auch die Regelungsgeschwindigkeit für den Zweipunktregler
eingestellt werden. Informationen können auf einem 16x2 LCD-Bildschirm abgelesen werden, der über I2c (mit I2c Bus) mit dem Microcontroller kommuniziert. 
Ein generisches Gleichstromnetzteil dient als Stromversorgung.

Oben in der Gondel des Windrads ist ein kleiner Generator, dessen Ausgangsleistung an einem Lastwiderstand (>2 Ohm) abfällt.
Die Spannung, welche über dem Widerstand abfällt, kann direkt analog mit dem internen 10-Bit ADC eingelesen werden, weil sie 5V (die Logikspannung des Nanos) nie überschreitet.
Aus diesen Werten kann die insgesamte elektrische Leistung abgeleitet werden. (meistens unter 200mW)

Eine andere Möglichkeit, die Leistung zu bestimmen ist mittels der detektierten Rotorblätter. Am Mast sitz ein HC-SR04 Ultraschallsensor, der zeitgleich in 5-Sekunden Intervallen
die Rotorblätter zählt und das Ergebnis dann auf dem LCD ausgibt. So kann die auf zwei Arten gemessene Leistung verglichen werden.

Alle hier aufgelisteten 3d-Modelle habe ich in Autodesk Fusion 360 oder SOLIDWORKS designed und auf einem Prusa Mk2 gedruckt. Das Design der Platine habe ich in 
Autodesk EAGLE erstellt und diese anschließend mit dem PCB-GCODE ulp-plugin zuhause gefertigt. Der Arduino Nano wurde mit der Arduino IDE programmiert.
Die Aluteile, die Propellernaben für unterschiedlich viele Rotorblätter, und die Basis für den Schrittmotor haben wir in Kooperation mit Autoliv gefertigt.
