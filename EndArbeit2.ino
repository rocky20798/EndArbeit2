/*************************************************************************************************************************************************************************
Ersteller: Schorkops Joé
Datum: 21.03.2017

E-Mail: jschorkops@yahoo.com
Handy: 0032471393542

© 2017 by Schorkops Joé
************************************************************************************************************************************************************************/

/*
Librarys einbinden, ich benutze folgende Librarys:
1) SPI: SPI muss ich mit einbinden, damit ich den SPI-Bus nutzen kann für die Datenübertragung der SD-Karte.
https://github.com/PaulStoffregen/SPI
2) Ethernet: Die Ethernet Library brauche ich, damit ich den Ethernet Port ansprechen kann.
https://github.com/arduino/Arduino/tree/master/libraries/Ethernet
3) Webserver: Diese Library nutze ich, um den Webserver online erreichbar zu machen.
https://github.com/sirleech/Webduino
4) SD: Diese Libary ist zuständig für die Kommunikation mit der Micro_SD_Karte.
https://github.com/adafruit/SD
5) Braccio: Mit dieser Library kann ich den Arduino Braccio ansteuern.
https://github.com/arduino-org/arduino-library-braccio
(Ich habe diese Library umgeschrieben für meine zwecke, die Originale funktioniert nicht!)
6) Servo: Diese Library ist notwenig, damit ich die Servo Motoren ansteuern kann.
http://playground.arduino.cc/ComponentLib/Servo
7) EthernetUdp: Diese Library nutze ich, wenn ich die Uhrzeit aus dem Internet auslese.
https://github.com/codebendercc/arduino-library-files
8) CmdMessenger: Diese Library nutze ich für die Serial1 Kommunikation mit dem PC.
https://github.com/thijse/Arduino-CmdMessenger
*/
#include <SPI.h>
#include <Ethernet.h>
#include "WebServer.h"
#include <SD.h>
#include <Braccio.h>
#include <Servo.h>
#include <EthernetUdp.h>
#include <CmdMessenger.h>
/*
Hier kann man festlegen, wie schnell die einzelnen Abläufe bei der Ablaufsteurung hintereinander erfolgen.
Man muss den Wert hier in ms angeben.
Achtung! wenn man 1000ms angibt (also 1s) dann dauert es in wirklichkeit 2000ms (also 2s),
weil diese WarteZeit 2mal ausgeführt wird.
*/
#define Zykluszeit 1 //Achtung, x2
//Hier wird die Mac Adresse des Arduino Ethernet Shieldes festgelegt.
static uint8_t mac[6] = { 0x02, 0xAA, 0xBB, 0xCC, 0x00, 0x22 };
/*
Hier kann man die IP-Adresyse festlegen.
Man muss die IP-Adresse dem Netzwerk anpassen.
169.254.213.200 Fritz Box Power Line
192.168.1.200 BBox3
172.16.0.200 Fritzbox
*/
IPAddress ip(192, 168, 1, 200);
//Konstanten um die Uhrzeit aus dem Internet zu lesen.
unsigned int localPort = 80;       // local port to listen for UDP packets
char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming && outgoing packets
/*
Desweiteren folgen die Globalen Variablen, wei Int, Char, String und #define
*/
//Int
//Wenn man True setzt, werden alle Servowerte die abgefahren werden gespeichert.
int ServoTXTFreigabe = false;
//Wenn  man hier True setzt, wird auch eine Uhrzeit bei der Speicherung hinzugefügt,
//aber nur mit einer aktiven Internetverbindung.
int UhrZeitFreigabe = false;
//Wenn hier True steht, wird die Bodenbegrenzung bei der Kinect eingeschaltet, damit man den Arm
//nicht auf dem Boden aufsetzten kann, also ein Schutz.
int BegrenzungFreigabe = true;
int rotation = 0;
int rotation2 = 15;
int rotation3 = 180;
int rotation4 = 180;
int rotation5 = 0;
int rotation6 = 10;
int FehlerNummer = 0;
int a = 0;
int MotorWert;
int rotation_auslesen;
int rotation7;
int ii = true;
int Pruefsumme = 9;
//Für die Übertragung Via CmdMessenger muss ich Globale Werte festlegen
int16_t rot1 = 0;
int16_t rot2 = 15;
int16_t rot6 = 73;
int16_t but1 = 0;
int16_t rot45 = 180;

enum
{
	cmdSendRot1,
	cmdSendRot2,
	cmdSendRot6,
	cmdButton1,
	cmdSendRot45
};
//CmdMessenger Start
CmdMessenger cmdMessenger = CmdMessenger(Serial);
//Char
char i; //Ausleser
		//String
String stringOne;
String stringTwo;
//Define
#define PREFIX ""
#define NAMELEN 32
#define VALUELEN 32
/*
1) hier wird der Web-Server eingestellt, also 80 bedeutet, dass er das ganz normale http:// Format verwenden soll.
2)Damit die Braccio Library arbeiten kann müssen die Servomotoren in der Servo Library aufgelistet werden.
3)Hier werden die einzelnen Dateinamen aufgelistet, die für die SD Library notwendig sind.
4) Udp wird bestimmt, man kann Ihm egal welchen Namen geben.
*/
WebServer webserver(PREFIX, 80); //1)
Servo base; //2)
Servo shoulder;
Servo elbow;
Servo wrist_rot;
Servo wrist_ver;
Servo gripper;
File webFile; //3)
File ServoFile;
File Fehlermeldungen;
File Ablauf;
EthernetUDP Udp; //4)
				 /*
				 Als aller erstes wir die Webseite von der SD-Karte geladen wenn sich jemand verbindet mit der Web-Seite
				 und sie muss nur 1 mal geladen werden, das ist auch der Grund, warum ich das AYAX-Protokoll gewählt hatte,
				 weil bei dem AYAX-Protokoll muss man die Seite nur 1mal Laden und nicht bei jedem Tasterdruck wieder neu.

				 Also diese Void indexCmd wird direckt am Start ausgeführt und die eigentliche Web-Seite befindet sich auf
				 der SD-Karte mit dem Namen index.htm
				 */
void indexCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
	//Server erfolgreich verbunden
	server.httpSuccess();
	/*
	Wenn GET ausgeführt wird, dann soll folgende IF-Funktion ausgeführt werden.

	Also GET bedeutet so viel, wenn die Web-Seite gebraucht wird, soll alles in der IF-Funktion ausgeführt.
	werden.
	*/
	if (type == WebServer::GET) {
		//HTM Dokument von der SD-Karte laden
		webFile = SD.open("index.htm");
		//Wenn sie gefunden wurde, wird die IF-Funktion ausgeführt um die Datei schlussendlich zu öffnen.
		if (webFile) {
			//Solange nicht alles aus der HTM gelesen wurde wird diese WHILE-Schleife ausgeführt.
			while (webFile.available()) {
				//Webseite an den Client schicken
				server.write(webFile.read());
			}
			//Am Ende die HTM-Datei wieder schließen.
			webFile.close();
		}
	}
}
/*
Wenn man den Slieder 1 (Base) auf der Webseite bewegt, dann wird diese Viod Funktion ausgeführt.
Der Arm bewegt sich erst, wenn man den Slider los lässt und nicht während dem bewegen, das
habe ich gemacht, damit nicht zuviele Daten gleichzeitig gesendet werden.

rotationCmd ist zuständig für den Servo M1, also die Basis (Base).
*/
void rotationCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool) {
	//Parameter und char Werte fest legen.
	URLPARAM_RESULT rc;
	char name[NAMELEN];
	char value[VALUELEN];
	//Warten bis die Anfrage erfolgreich war.
	server.httpSuccess();
	/*
	Der Rest ist eigentlich nur Daten abfangen, überprüfen ob Sie richtig sind und dann an den Servo Motor ausgeben.
	*/
	if (type == WebServer::GET && strlen(url_tail) > 0) {
		while (strlen(url_tail)) {
			rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
			if (rc == URLPARAM_EOS) {
				// Keine Parameter mehr übrig
			}
			else if (strcmp(name, "value") == 0) {
				rotation = atoi(value);
				// Hier ist nun die Gradzahl verfügbar (0 <= rotation <= 180)
				Braccio.ServoMovement(20, rotation, rotation2, rotation3, rotation4, rotation5, rotation6);
				//SD-Karte Servo Werte schreiben wenn diese Funktion aktiviert wurde.
				ServoTXT();
			}
		}
	}
}
/*
Wenn man den Slieder 2 (Shoulder) auf der Webseite bewegt, dann wird diese Viod Funktion ausgeführt.
Der Arm bewegt sich erst, wenn man den Slider los lässt und nicht während dem bewegen, das
habe ich gemacht, damit nicht zuviele Daten gleichzeitig gesendet werden.

rotation2Cmd ist zuständig für den Servo M2, also die Schulter (Shoulder).
*/
void rotation2Cmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool) {
	//Parameter und char Werte fest legen.
	URLPARAM_RESULT rc;
	char name2[NAMELEN];
	char value2[VALUELEN];
	//Warten bis die Anfrage erfolgreich war.
	server.httpSuccess();
	/*
	Der Rest ist eigentlich nur Daten abfangen, überprüfen ob Sie richtig sind und dann an den Servo Motor ausgeben.
	*/
	if (type == WebServer::GET && strlen(url_tail) > 0) {
		while (strlen(url_tail)) {
			rc = server.nextURLparam(&url_tail, name2, NAMELEN, value2, VALUELEN);
			if (rc == URLPARAM_EOS) {
				// Keine Parameter mehr übrig
			}
			else if (strcmp(name2, "value") == 0) {
				rotation2 = atoi(value2);
				// Hier ist nun die Gradzahl verfügbar (0 <= rotation <= 180)
				Braccio.ServoMovement(20, rotation, rotation2, rotation3, rotation4, rotation5, rotation6);
				//SD-Karte Servo Werte schreiben
				ServoTXT();
			}
		}
	}
}
/*
Wenn man den Slieder 3 (Elbow) auf der Webseite bewegt, dann wird diese Viod Funktion ausgeführt.
Der Arm bewegt sich erst, wenn man den Slider los lässt und nicht während dem bewegen, das
habe ich gemacht, damit nicht zuviele Daten gleichzeitig gesendet werden.

rotation3Cmd ist zuständig für den Servo M3, also den Unteren Elbogen (Elbow).
*/
void rotation3Cmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool) {
	//Parameter und char Werte fest legen.
	URLPARAM_RESULT rc;
	char name3[NAMELEN];
	char value3[VALUELEN];
	//Warten bis die Anfrage erfolgreich war.
	server.httpSuccess();
	/*
	Der Rest ist eigentlich nur Daten abfangen, überprüfen ob Sie richtig sind und dann an den Servo Motor ausgeben.
	*/
	if (type == WebServer::GET && strlen(url_tail) > 0) {
		while (strlen(url_tail)) {
			rc = server.nextURLparam(&url_tail, name3, NAMELEN, value3, VALUELEN);
			if (rc == URLPARAM_EOS) {
				// Keine Parameter mehr übrig
			}
			else if (strcmp(name3, "value") == 0) {
				rotation3 = atoi(value3);
				// Hier ist nun die Gradzahl verfügbar (0 <= rotation <= 180)
				Braccio.ServoMovement(20, rotation, rotation2, rotation3, rotation4, rotation5, rotation6);
				//SD-Karte Servo Werte schreiben
				ServoTXT();
			}
		}
	}
}
/*
Wenn man den Slieder 4 (Wrist-Ver) auf der Webseite bewegt, dann wird diese Viod Funktion ausgeführt.
Der Arm bewegt sich erst, wenn man den Slider los lässt und nicht während dem bewegen, das
habe ich gemacht, damit nicht zuviele Daten gleichzeitig gesendet werden.

rotation4Cmd ist zuständig für den Servo M4, also den Oberen Elbogen (Wrist-Ver).
*/
void rotation4Cmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool) {
	//Parameter und char Werte fest legen.
	URLPARAM_RESULT rc;
	char name4[NAMELEN];
	char value4[VALUELEN];
	//Warten bis die Anfrage erfolgreich war.
	server.httpSuccess();
	/*
	Der Rest ist eigentlich nur Daten abfangen, überprüfen ob Sie richtig sind und dann an den Servo Motor ausgeben.
	*/
	if (type == WebServer::GET && strlen(url_tail) > 0) {
		while (strlen(url_tail)) {
			rc = server.nextURLparam(&url_tail, name4, NAMELEN, value4, VALUELEN);
			if (rc == URLPARAM_EOS) {
				// Keine Parameter mehr übrig
			}
			else if (strcmp(name4, "value") == 0) {
				rotation4 = atoi(value4);
				// Hier ist nun die Gradzahl verfügbar (0 <= rotation <= 180)
				Braccio.ServoMovement(20, rotation, rotation2, rotation3, rotation4, rotation5, rotation6);
				//SD-Karte Servo Werte schreiben
				ServoTXT();
			}
		}
	}
}
/*
Wenn man den Slieder 5 (Wrist-Rot) auf der Webseite bewegt, dann wird diese Viod Funktion ausgeführt.
Der Arm bewegt sich erst, wenn man den Slider los lässt und nicht während dem bewegen, das
habe ich gemacht, damit nicht zuviele Daten gleichzeitig gesendet werden.

rotation5Cmd ist zuständig für den Servo M5, also die Rotation der Zange (Wrist-Rot).
*/
void rotation5Cmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool) {
	//Parameter und char Werte fest legen.
	URLPARAM_RESULT rc;
	char name5[NAMELEN];
	char value5[VALUELEN];
	//Warten bis die Anfrage erfolgreich war.
	server.httpSuccess();
	/*
	Der Rest ist eigentlich nur Daten abfangen, überprüfen ob Sie richtig sind und dann an den Servo Motor ausgeben.
	*/
	if (type == WebServer::GET && strlen(url_tail) > 0) {
		while (strlen(url_tail)) {
			rc = server.nextURLparam(&url_tail, name5, NAMELEN, value5, VALUELEN);
			if (rc == URLPARAM_EOS) {
				// Keine Parameter mehr übrig
			}
			else if (strcmp(name5, "value") == 0) {
				rotation5 = atoi(value5);
				// Hier ist nun die Gradzahl verfügbar (0 <= rotation <= 180)
				Braccio.ServoMovement(20, rotation, rotation2, rotation3, rotation4, rotation5, rotation6);
				//SD-Karte Servo Werte schreiben
				ServoTXT();
			}
		}
	}
}
/*
Wenn man den Slieder 6 (Gripper) auf der Webseite bewegt, dann wird diese Viod Funktion ausgeführt.
Der Arm bewegt sich erst, wenn man den Slider los lässt und nicht während dem bewegen, das
habe ich gemacht, damit nicht zuviele Daten gleichzeitig gesendet werden.

rotation6Cmd ist zuständig für den Servo M6, also die Zange (Gripper).
*/
void rotation6Cmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool) {
	//Parameter und char Werte fest legen.
	URLPARAM_RESULT rc;
	char name6[NAMELEN];
	char value6[VALUELEN];
	//Warten bis die Anfrage erfolgreich war.
	server.httpSuccess();
	/*
	Der Rest ist eigentlich nur Daten abfangen, überprüfen ob Sie richtig sind und dann an den Servo Motor ausgeben.
	*/
	if (type == WebServer::GET && strlen(url_tail) > 0) {
		while (strlen(url_tail)) {
			rc = server.nextURLparam(&url_tail, name6, NAMELEN, value6, VALUELEN);
			if (rc == URLPARAM_EOS) {
				// Keine Parameter mehr übrig
			}
			else if (strcmp(name6, "value") == 0) {
				rotation6 = atoi(value6);
				// Hier ist nun die Gradzahl verfügbar (0 <= rotation <= 180)
				Braccio.ServoMovement(20, rotation, rotation2, rotation3, rotation4, rotation5, rotation6);
				//SD-Karte Servo Werte schreiben
				ServoTXT();
			}
		}
	}
}
/*
Diese Funktion kann man im Setup Ein/Aus schalten, dafür muss man einfach nur den Int ServoTXTFreigabe auf True stellen um es zu
aktivieren und auf False um es zu deaktivieren.

Diese Funktion ist dafür gut, wenn man haben will, dass die einzelenen abgefahrenen Positionen gespeichert werden, diese Funktion
kann hilfreich sein, wenn man auf der Suche nach Fehlern ist.
*/
void ServoTXT() {
	//Ist der Int Wert=True wird diese Funktio ausgeführt, sonst nicht.
	if (ServoTXTFreigabe) {
		//SD-Karte Servo Werte schreiben
		ServoFile = SD.open("servo.txt", FILE_WRITE);
		if (ServoFile) {
			UhrZeit();
			ServoFile.print("Base: ");
			ServoFile.print(rotation);
			ServoFile.print("     Shoulder: ");
			ServoFile.print(rotation2);
			ServoFile.print("     Elbow: ");
			ServoFile.print(rotation3);
			ServoFile.print("     Wrist_ver: ");
			ServoFile.print(rotation4);
			ServoFile.print("     Wrist_rot: ");
			ServoFile.print(rotation5);
			ServoFile.print("     Gripper: ");
			ServoFile.print(rotation6);
			ServoFile.println();
			// Datei schließen.
			ServoFile.close();
		}
		else {
			// Wenn man die Datei nicht öffnen kann wird folgendes Ausgegeben.
			Serial1.println("Error beim öffnen der Text Datei");
		}
	}
}
/*
Diese Funtion kann man im Setup Ein/Aus schalten, dafür muss man einfach den Int UhrZeitFreigabe auf True
setzen um es zu aktivieren und auf False um es zu deaktivieren.

Diese Funktion ist dafür da, wenn man in der ServoTXT Datei auch die Uhrzeit angezeigt werden will haben.

Ich hohle die Uhrzeit aus dem Internet, damit ich sie nicht nachstellen muss und diese Zeit ist normalerweise immer ziemlich genau.
Natürlich funktioniert diese Funktion nur, wenn Internet Aktiv ist, weil sonst kann er die Uhrzeit nicht aus dem Internet
auslesen.
*/
void UhrZeit() {
	if (UhrZeitFreigabe) {
		//Uhr auslesen
		sendNTPpacket(timeServer); // send an NTP packet to a time server
								   // wait to see if a reply is available
		delay(1000);
		if (Udp.parsePacket()) {
			Udp.read(packetBuffer, NTP_PACKET_SIZE);
			unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
			unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
			unsigned long secsSince1900 = highWord << 16 | lowWord;
			const unsigned long seventyYears = 2208988800UL;
			unsigned long epoch = secsSince1900 - seventyYears;
			// print the hour, minute && second:
			ServoFile.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
			ServoFile.print((epoch % 86400L) / 3600); // print the hour (86400 equals secs per day)
			ServoFile.print(':');
			if (((epoch % 3600) / 60) < 10) {
				// In the first 10 minutes of each hour, we'll want a leading '0'
				ServoFile.print('0');
			}
			ServoFile.print((epoch % 3600) / 60); // print the minute (3600 equals secs per minute)
			ServoFile.print(':');
			if ((epoch % 60) < 10) {
				// In the first 10 seconds of each minute, we'll want a leading '0'
				ServoFile.print('0');
			}
			ServoFile.println(epoch % 60); // print the second
		}
		Ethernet.maintain();
	}
}
// send an NTP request to the time server at the given address
void sendNTPpacket(char* address) {
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
							 // 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;

	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	Udp.beginPacket(address, 123); //NTP requests are to port 123
	Udp.write(packetBuffer, NTP_PACKET_SIZE);
	Udp.endPacket();
}
/*
Jetzt kommt die Ablaufsteurng, also wenn man einen Taster drückt auf der Webseite, werden folgende Funktionen
ausgeführt.

Mit dem Schalter 1 (schalter1Cmd) kann man die Alaufdatei, die sich auf der Micro-SD-Karte befindet läschen, also diese
Funktion braucht man, wenn man den alten Ablauf löschen will.
*/
void schalter1Cmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool) {
	//Parameter und Char Werte fest legen.
	URLPARAM_RESULT rc;
	char name[NAMELEN];
	char value[VALUELEN];
	//Warten bis die Anfrage erfolgreich war.
	server.httpSuccess();
	/*
	Der Rest ist eigentlich nur Daten abfangen, überprüfen ob Sie richtig sind und dann die Datei löschen.
	*/
	if (type == WebServer::GET && strlen(url_tail) > 0) {
		while (strlen(url_tail)) {
			rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
			if (rc == URLPARAM_EOS) {
				// Keine Parameter mehr übrig
			}
			else if (strcmp(name, "value") == 0) {
				if (atoi(value) == 1) {
					//Datei löschen
					SD.remove("Ablauf.txt");
				}
			}
		}
	}
}
/*
Mit dem Schalter 2 (schalter2Cmd) kann man die Position, der einzelnen Servo Motoren speichern auf der
Micro-SD-Karte.
*/
void schalter2Cmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool) {
	//Parameter Werte festlegen.
	URLPARAM_RESULT rc;
	char name[NAMELEN];
	char value[VALUELEN];
	//Warten bis die Abfrage erfolgreich war.
	server.httpSuccess();
	//Wenn der Server mit der GET-Anfrage kommt, lese ich hier die Werte aus, die mit gegeben werden, in meinem fall ist das eine 1, also true, die den Vorgang Speicher startet.
	if (type == WebServer::GET && strlen(url_tail) > 0) {
		while (strlen(url_tail)) {
			rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
			if (rc == URLPARAM_EOS) {
				// Keine Parameter mehr übrig
			}
			else if (strcmp(name, "value") == 0) {
				//Wenn der gesendetet Wert mit 1 überein stimmt, werden die Servo Daten auf der SD-Karte gespeichert.
				if (atoi(value) == 1) {
					//Die Ablauf TXT öffnene (TXT bedeutet Text Datei).
					Ablauf = SD.open("Ablauf.txt", FILE_WRITE);
					//Wenn die Datei offen ist, beginnt das Schreiben auf die SD-Karte
					if (Ablauf) {
						/*
						Ich habe meine eigene Methode entwickelt um die Daten auf die SD-Karte zu schreiben, ich habe es versucht einfach genug zu machen, damit jeder diese Datei
						öffnene kann und gegebenfalls bearbeiten.
						Ein gespeichertter Motorwert sieht folgendermaßen aus:
						1|180|9
						Also die erste Zahlt steht für welchen Motor ich ansteuern will, z.B. 1 für Base (M1), 2 für Shoulder (M2), usw.
						Danach kommt ein Strich, damit es für den Menschenschen leserlich bleibt.
						Als nächstes wird der Servowert geschrieben, immer einen 3stellige Zahl, damit es sauberer ausssieht, als ob es eine Tabelle wäre
						Richtig:					Falsch:
						1|180|9						1|180|9
						3|090|9						3|90|9
						6|073|9						6|73|9
						2|165|0						2|165|0
						Die letzte Zahl ist eine Kontrollzahl, die ich zur Kontrolle des Programmes nutze, wenn man eine 9 schreibt, weil der Arduino dass das nicht das Ende ist,
						schreibt man aber eine beliebige zahl unter 9, z.B. 0, dass Endet der Ablauf dort.
						*/
						//Schreibe 1 für Base (M1).
						Ablauf.print(1);
						//Strich machen für die Kolonne.
						Ablauf.print('|');
						//Abgleich Funktion 1.
						Abgleich1();
						//Strich machen für die Kolonne.
						Ablauf.print('|');
						//Jetzt wird die 9 oder einen anderer Zahl geschrieben, bedeutung wird einige Zeilen drüber erklärt.
						Ablauf.print(Pruefsumme);
						//Nächste Zeile.
						Ablauf.println("");
						//Schreibe 2 für Shoulder (M2).
						Ablauf.print(2);
						//Strich machen für die Kolonne.
						Ablauf.print('|');
						//Abgleich Funktion 2.
						Abgleich2();
						//Strich machen für die Kolonne.
						Ablauf.print('|');
						//Jetzt kommt die Prüfsumme, also 9 oder eine andere Zahl.
						Ablauf.print(Pruefsumme);
						//Nächste Zeile.
						Ablauf.println("");
						//Schreibe 3 für Elbow (M3).
						Ablauf.print(3);
						//Strich machen für die Kolonne.
						Ablauf.print('|');
						//Abgleich Funktion 3.
						Abgleich3();
						//Strich machen für Kolonne.
						Ablauf.print('|');
						//Jetzt kommt die Prüfsumme, also 9 oder eine andere Zahl.
						Ablauf.print(Pruefsumme);
						//Nächste Zeile.
						Ablauf.println("");
						//Schreibe 4 für Wrist_Ver (M4).
						Ablauf.print(4);
						//Strich machen für die Kolonne.
						Ablauf.print('|');
						//Abgleich Funktion 4.
						Abgleich4();
						//Strich machen für die Kolonne.
						Ablauf.print('|');
						//Jetzt kommt die Prüfsumme, also 9 oder eine andere Zahl.
						Ablauf.print(Pruefsumme);
						//Nächste Zeile.
						Ablauf.println("");
						//Schreibe 5 für Wrist_Rot (M5).
						Ablauf.print(5);
						//Strich machen für die Kolonne.
						Ablauf.print('|');
						//Abgleich Funktion 5.
						Abgleich5();
						//Strich machen für die Kolonne.
						Ablauf.print('|');
						//Jetzt kommt die Prüfsumme, also 9 oder eine andere Zahl.
						Ablauf.print(Pruefsumme);
						//Nächste Zeile.
						Ablauf.println("");
						//Schreibe 6 für Gripper (M6).
						Ablauf.print(6);
						//Strich machen für die Kolonne.
						Ablauf.print('|');
						//Abgleich Funktion 6.
						Abgleich6();
						//Strich machen für die Kolonne.
						Ablauf.print('|');
						//Jetzt kommt die Prüfsumme, also 9 oder eine andere Zahl.
						Ablauf.print(Pruefsumme);
						//Nächste Zeile.
						Ablauf.println("");
						//Text Datei schließen.
						Ablauf.close();
					}
					else {
						//Wenn ein Fehler passiert, wird eine Fehlermeldung übertragen.
						Serial1.println("error opening test.txt");
					}
				}
			}
		}
	}
}
/*
Für was sind die Abgleichfunktionen gut?
Ich nutze die Abgleichfunktionen um z.B. aus 7 eine 007 zu machen, der Zahlenwert bleibt gleich,
nur wenn ich auf der SD-Karte schreibe, werden die Kolonnen schöner und einheitlicher.
*/
//Abgleich für den M1
void Abgleich1() {
	//Wenn der Servo Wert von M1 größer als 100 ist, dann...
	if (rotation > 100) {
		//wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(rotation);
	}
	//Wenn der Servo Wert von M1 zwischen 10 und 100 ist, dann...
	if (rotation > 10 && rotation < 100) {
		//wird ein String mit "0" erstellt
		stringTwo = String("0");
		//und der Servo Wert wird hinzu gefügt, also z.B. "086"
		stringTwo = stringTwo + rotation;
		//dann wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(stringTwo);
	}
	//Wenn der Servo Wert von M1 unter 10 ist, dann...
	if (rotation < 10) {
		//wird ein String mit "00" erstellt
		stringTwo = String("00");
		//und der Servo Wert wird hinzu gefügt, also z.B. "006"
		stringTwo = stringTwo + rotation;
		//dann wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(stringTwo);
	}
}
//Abgleich für den M1
void Abgleich2() {
	//Wenn der Servo Wert von M2 größer als 100 ist, dann...
	if (rotation2 > 100) {
		//wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(rotation2);
	}
	//Wenn der Servo Wert von M2 zwischen 10 und 100 ist, dann...
	if (rotation2 > 10 && rotation2 < 100) {
		//wird ein String mit "0" erstellt
		stringTwo = String("0");
		//und der Servo Wert wird hinzu gefügt, also z.B. "086"
		stringTwo = stringTwo + rotation2;
		//dann wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(stringTwo);
	}
	//Wenn der Servo Wert von M2 unter 10 ist, dann...
	if (rotation2 < 10) {
		//wird ein String mit "00" erstellt
		stringTwo = String("00");
		//und der Servo Wert wird hinzu gefügt, also z.B. "006"
		stringTwo = stringTwo + rotation2;
		//dann wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(stringTwo);
	}
}
//Abgleich für den M3
void Abgleich3() {
	//Wenn der Servo Wert von M3 größer als 100 ist, dann...
	if (rotation3 > 100) {
		//wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(rotation3);
	}
	//Wenn der Servo Wert von M3 zwischen 10 und 100 ist, dann...
	if (rotation3 > 10 && rotation3 < 100) {
		//wird ein String mit "0" erstellt
		stringTwo = String("0");
		//und der Servo Wert wird hinzu gefügt, also z.B. "086"
		stringTwo = stringTwo + rotation3;
		//dann wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(stringTwo);
	}
	//Wenn der Servo Wert von M3 unter 10 ist, dann...
	if (rotation3 < 10) {
		//wird ein String mit "00" erstellt
		stringTwo = String("00");
		//und der Servo Wert wird hinzu gefügt, also z.B. "006"
		stringTwo = stringTwo + rotation3;
		//dann wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(stringTwo);
	}
}
//Abgleich für den M4
void Abgleich4() {
	//Wenn der Servo Wert von M4 größer als 100 ist, dann...
	if (rotation4 > 100) {
		//wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(rotation4);
	}
	//Wenn der Servo Wert von M4 zwischen 10 und 100 ist, dann...
	if (rotation4 > 10 && rotation4 < 100) {
		//wird ein String mit "0" erstellt
		stringTwo = String("0");
		//und der Servo Wert wird hinzu gefügt, also z.B. "086"
		stringTwo = stringTwo + rotation4;
		//dann wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(stringTwo);
	}
	//Wenn der Servo Wert von M4 unter 10 ist, dann...
	if (rotation4 < 10) {
		//wird ein String mit "00" erstellt
		stringTwo = String("00");
		//und der Servo Wert wird hinzu gefügt, also z.B. "006"
		stringTwo = stringTwo + rotation4;
		//dann wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(stringTwo);
	}
}
//Abgleich für den M5
void Abgleich5() {
	//Wenn der Servo Wert von M5 größer als 100 ist, dann...
	if (rotation5 > 100) {
		//wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(rotation5);
	}
	//Wenn der Servo Wert von M5 zwischen 10 und 100 ist, dann...
	if (rotation5 > 10 && rotation5 < 100) {
		//wird ein String mit "0" erstellt
		stringTwo = String("0");
		//und der Servo Wert wird hinzu gefügt, also z.B. "086"
		stringTwo = stringTwo + rotation5;
		//dann wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(stringTwo);
	}
	//Wenn der Servo Wert von M5 unter 10 ist, dann...
	if (rotation5 < 10) {
		//wird ein String mit "00" erstellt
		stringTwo = String("00");
		//und der Servo Wert wird hinzu gefügt, also z.B. "006"
		stringTwo = stringTwo + rotation5;
		//dann wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(stringTwo);
	}
}
//Abgleich für den M6
void Abgleich6() {
	//Wenn der Servo Wert von M6 größer als 100 ist, dann...
	if (rotation6 > 100) {
		//wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(rotation);
	}
	//Wenn der Servo Wert von M6 zwischen 10 und 100 ist, dann...
	if (rotation6 > 10 && rotation6 < 100) {
		//wird ein String mit "0" erstellt
		stringTwo = String("0");
		//und der Servo Wert wird hinzu gefügt, also z.B. "086"
		stringTwo = stringTwo + rotation6;
		//dann wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(stringTwo);
	}
	//Wenn der Servo Wert von M6 unter 10 ist, dann...
	if (rotation6 < 10) {
		//wird ein String mit "00" erstellt
		stringTwo = String("00");
		//und der Servo Wert wird hinzu gefügt, also z.B. "006"
		stringTwo = stringTwo + rotation6;
		//dann wird der Servo Wert auf die SD-Karte geschrieben.
		Ablauf.print(stringTwo);
	}
}
/*
Für den gespeicherten Ablauf zu starten, muss man den Taster 3 auf der Webseite drücken.
*/
void schalter3Cmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool) {
	//Parameter und char Werte fest legen.
	URLPARAM_RESULT rc;
	char name[NAMELEN];
	char value[VALUELEN];
	//Parameter und char Werte fest legen.
	server.httpSuccess();
	/*
	Der Rest ist eigentlich nur Daten abfangen, überprüfen ob Sie richtig sind und dann an den Servo Motor ausgeben.
	Also hier werden dann die Daten der SD-Karte ausgelesen und verarbeitet.
	*/
	if (type == WebServer::GET && strlen(url_tail) > 0) {
		while (strlen(url_tail)) {
			rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
			if (rc == URLPARAM_EOS) {
				// Keine Parameter mehr übrig
			}
			else if (strcmp(name, "value") == 0) {
				//Wenn der Wert 1 gesendet wurde, wird folgende Funktion ausgeführt.
				if (atoi(value) == 1) {
					//Der Int ii wird hier auf True gesetzt.
					int ii = true;
					//Jetzt wird die Funktion lesen ausgeführt, sie dient dazu die Daten zu lesen von der SD-Karte
					lesen();
				}
			}
		}
	}
}

void lesen() {
	//Zuerst wird die Ablauf.txt geöffnet, wo sich der ganze Ablaufplan drin befindet.
	Ablauf = SD.open("Ablauf.txt");
	//Solange Werte verfügbar sind, wird folgende Funktion ausgeführt.
	if (Ablauf) {
		//Der Serial Monitor 1 schreibt jetzt, dass die Text Datei ausgelesen wird.
		Serial1.println("Ablauf.txt:");
		//Solange ein Ablauf verfügbar ist, wird diese While-Schleife ausgeführt.
		while (Ablauf.available()) {
			//Der Int a wird hier auf den Wert 0 gesetzt.
			a = 0;
			//Solange a kleiner ist als 7 wird diese Schleife ausgeführt, also insgesamt 6 mal wird das geschehen,
			//weil ich habe ja 6 unterschiedliche Servo-Motoren.
			while (a < 7) {
				//Wenn a gleich an 0 ist, dann...
				if (a == 0) {
					//...wird gelesen, welcher Servo Motor angesteuert werden soll...
					i = Ablauf.read();
					//..., der Serial Port 1 schreibt den Motor Wert, damit man das auch extren mitverfolgen kann.
					Serial1.print("Motor:  ");
					//Um aus einem Char-Wert einen Int-Wert zu machen, muss man einfach minus 48 machen.
					MotorWert = i - 48;
					//Der Servo-Wert wird jetzt im Serial Monitor 1 geschrieben. 
					Serial1.print(MotorWert);
					Serial1.println();
				}
				//Wenn a 1 oder 5 ist, wird einfach gelesen, aber danch wird die Zahl einfach wieder vergessen.
				if (a == 1 || a == 5) {
					//Wert lesen, aber damit wird nicht angefangen, ist das Trennzeichen, also '|'
					i = Ablauf.read();
				}
				//Wenn a gleich an 2 ist, ...
				if (a == 2) {
					//...wird der Int b auf 0 gesetzt...
					int b = 0;
					//... und ein String wird erstellt.
					stringOne = String();
					//b nutze ich um 3 mal rauf zu Zählen, weil mein MotorWert ja einen 3 stellige Zahl ist.
					while (b < 3) {
						//b + 1 machen
						b++;
						//Wert lesen von der SD-Karte
						i = Ablauf.read();
						//Alles in einen String verpacken.
						stringOne = stringOne + i;
					}
					//Den a-Wert auf 4 setzten..
					a = 4;
					//...und im Serial Monitor 1 "Rotation: " schreiben.
					Serial1.print("Rotation:  ");
					//Hier wird der String in einen Int umgewandelt.
					rotation7 = stringOne.toInt();
					//Der Serial Monitor 1 schreibt jetzt den RotationsWert.
					Serial1.print(rotation7);
					Serial1.println();
				}
				//Wenn a gleich an 6 ist, wird folgende Funktion augeführt, sie dient zur Überprüfung.
				if (a == 6) {
					//zuerst wird der Wert gelesen.
					i = Ablauf.read();
					//Danach wird aus einem Char-Wert ein Int-Wert gemacht.
					i = i - 48;
					//Wenn der Int-Wert nicht gleich an 9 ist, wird ii false gesetzt und der Ablauf stoppt.
					if (i = !9) {
						// ii auf der Wert false setzten
						ii = false;
						//zurück in die Loop-Schleife
						loop();
					}
					//Lesen + Lesen
					i = Ablauf.read();
					i = Ablauf.read();
					//Ausganbe, hier werden die Servomotoren beschrieben.
					Ausgabe();
				}
				//a um 1 addieren.
				a++;
				//Wartezeit für den Ablauf, kann man am Anfang vom Programmcode festlegen.
				delay(Zykluszeit);
			}
		}
		//Textdatei "Ablauf.txt" schließen.
		Ablauf.close();
	}
	else {
		//Wenn ein Fehler auftritt, wird eine Fehlermeldung geschrieben.
		Serial1.println("error opening test.txt");
	}
}
/*
Nachdem der Servo-Wert von der SD-Karte ausgelesen wurde, muss er mal ausgegeben werden, das geschieht hier in der
folgenden Funktion.
*/
void Ausgabe() {
	//Serial Monitor schreibt, dass die Ausgabe beginnt.
	Serial1.print("Ausgabe: ");
	Serial1.println();
	//Wenn der ausgelsene Motorwert gleich an 1 ist, wird die rotation beschrieben, also M1.
	if (MotorWert == 1) {
		rotation = rotation7;
	}
	//Wenn der ausgelsene Motorwert gleich an 2 ist, wird die rotation2 beschrieben, also M2.
	if (MotorWert == 2) {
		rotation2 = rotation7;
	}
	//Wenn der ausgelsene Motorwert gleich an 3 ist, wird die rotation3 beschrieben, also M3.
	if (MotorWert == 3) {
		rotation3 = rotation7;
	}
	//Wenn der ausgelsene Motorwert gleich an 4 ist, wird die rotation4 beschrieben, also M4.
	if (MotorWert == 4) {
		rotation4 = rotation7;
	}
	//Wenn der ausgelsene Motorwert gleich an 5 ist, wird die rotation5 beschrieben, also M5.
	if (MotorWert == 5) {
		rotation5 = rotation7;
	}
	//Wenn der ausgelsene Motorwert gleich an 6 ist, wird die rotation beschrieben, also M6.
	if (MotorWert == 6) {
		rotation6 = rotation7;
	}
	//Hier werden die einzelnen Servomotoren geschrieben.
	Braccio.ServoMovement(20, rotation, rotation2, rotation3, rotation4, rotation5, rotation6);
}
/*
Mit dem 4ten und letzten Schlater kann man auslösen, dass aus der Prüfsumme 9 eine 0 wird und dadurch wird der Ablauf unterbrochen.
*/
void schalter4Cmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool) {
	//Parameter und Char Werte fest legen.
	URLPARAM_RESULT rc;
	char name[NAMELEN];
	char value[VALUELEN];
	//Warten bis die Anfrage erfolgreich war.
	server.httpSuccess();
	/*
	* Der Rest ist eigentlich nur Daten abfangen, überprüfen ob Sie richtig sind und dann die Prüfsumme verändern..
	*/
	if (type == WebServer::GET && strlen(url_tail) > 0) {
		while (strlen(url_tail)) {
			rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
			if (rc == URLPARAM_EOS) {
				// Keine Parameter mehr übrig
			}
			else if (strcmp(name, "value") == 0) {
				//Wenn der Wert 1 übertragen wird, dann wird folgende Funktion ausgeführt.
				if (atoi(value) == 1) {
					//Wenn die Prüfsumme auf 9 steht, dann wird sie zu 0 geändert oder...
					if (Pruefsumme == 9) {
						Pruefsumme = 0;
						Serial1.println("Nicht schreiben");
					}
					//...wenn sie auf 0 steht, dann wird sie zu 9 gändert.
					else {
						Pruefsumme = 9;
						Serial1.println("schreiben");
					}
				}
			}
		}
	}
}
/*
Setup
*/
void setup() {
	//Baud Rate für die CMD Kommunikation wird hier festgelegt.
	Serial.begin(250000);
	//Call Backs Aktivieren für die Kommunikation über CmdMessenger.
	attachCommandCallbacks();
	//Ausgänge festlegen für LED's
	pinMode(52, OUTPUT);
	pinMode(53, OUTPUT);
	pinMode(40, OUTPUT);
	pinMode(41, OUTPUT);
	pinMode(42, OUTPUT);
	//Not-Aus Funktin wird aufgerufen
	NotAus();
	//Der Serielle Monitor 1 wir eingeschaltet mit 9600Baut, also einer Übertragungsrate von 9600Bit pro sec.
	Serial1.begin(9600);
	while (!Serial1) {
		; // Wartet so lange, bis der Serial1 Monitor gestartet ist.
	}
	//Die Arduino Braccio Library wird gestartet.
	Braccio.begin();
	//Der Roboterarm führt die Grundposition an.
	Braccio.ServoMovement(20, 0, 15, 180, 180, 0, 10);
	//Die String Werte werden definiert.
	stringTwo = String();
	stringOne = String();
	//SD-Karte wird Initialisiert
	Serial1.print("Initialisieren der SD-Karte");
	//Wenn das Initialisieren nicht geklappt hat, wird ein Fehler ausgegeben.
	if (!SD.begin(4)) {
		Serial1.println("Initialisieren fehlgeschlagen");
		return;
	}
	//Aber wenn das Initialisieren erfolgreich war, wird folgendes ausgegeben.
	Serial1.println("Initialisieren Erfolgreich");
	//Ethernet Port starten.
	Ethernet.begin(mac, ip);
	// Die index.html ausliefern
	webserver.setDefaultCommand(&indexCmd);
	// Die Rotation handeln
	webserver.addCommand("rotationBase", &rotationCmd);
	webserver.addCommand("rotationShoulder", &rotation2Cmd);
	webserver.addCommand("rotationElbow", &rotation3Cmd);
	webserver.addCommand("rotationVer", &rotation4Cmd);
	webserver.addCommand("rotationRot", &rotation5Cmd);
	webserver.addCommand("rotationGripper", &rotation6Cmd);
	// Die Schalter handeln
	webserver.addCommand("schalter1", &schalter1Cmd);
	webserver.addCommand("schalter2", &schalter2Cmd);
	webserver.addCommand("schalter3", &schalter3Cmd);
	webserver.addCommand("schalter4", &schalter4Cmd);
	//Webserver starten
	webserver.begin();
	//Uhrzeit auslesung starten
	Udp.begin(localPort);
}
/*
NotAus Funktion, also er bleibt so lange im Not-Stand, bis man Reset drückt.
Die Rote LED leuchtet so lange, wie man den Not-Aus Taster drückt.
Die Orange Led blinkt im sec. Tackt wenn man den Reset drücken soll
Die Grüne LED leuchtet bei normalem Betrieb.
*/
void NotAus() {
	//Solange RESET nicht gedrückt wird folgende While-Schleife ausgeführt.
	while (analogRead(0) < 700) {
		// Ich benutze hier für For-Schleifen, damit ich den RESET-Taster 1000 mal pro Sec. auslesen kann.
		//Zuerst ist die Orange LED 1 sec. an
		for (int i = 0; i < 1000; i++) {
			if (analogRead(0) > 700) {
				return;
			}
			delay(1);
			digitalWrite(52, HIGH);
		}
		//Danach wird die Orange Led 1 sec. lang aus sein.
		for (int i = 0; i < 1000; i++) {
			if (analogRead(0) > 700) {
				return;
			}
			delay(1);
			digitalWrite(52, LOW);
		}
	}
}
// Die Call Backs definieren
void attachCommandCallbacks()
{
	cmdMessenger.attach (cmdSendRot1, OnCmdSendRot1);
	cmdMessenger.attach(cmdSendRot2, OnCmdSendRot2);
	cmdMessenger.attach(cmdSendRot6, OnCmdSendRot6);
	cmdMessenger.attach(cmdButton1, OnCmdButton1);
	cmdMessenger.attach(cmdSendRot45, OnCmdSendRot45);
}
/*
Hier werden die Funktionen aufgerufen, die Stattfinden, wenn man die Funktion aufruft
*/
//Hier werden die Daten der Base empfangen und verarbeitet wenn es nötig ist.
void OnCmdSendRot1()
{
	rot1 = cmdMessenger.readBinArg<int16_t>();
}
//Hier werden die Daten der Shoulder empfangen und verarbeitet wenn es nötig ist.
void OnCmdSendRot2()
{
	rot2 = cmdMessenger.readBinArg<int16_t>();
	rot2 = rot2 + 15;
	if (rot2 > 165) {
		rot2 = 165;
	}
	rot2 = 165 - rot2;
}
//Hier werden die Daten dem Gripper empfangen und verarbeitet wenn es nötig ist.
void OnCmdSendRot6()
{
	rot6 = cmdMessenger.readBinArg<int16_t>();
}
//Hier werden die Daten des Wahl-Button empfangen und verarbeitet wenn es nötig ist.
void OnCmdButton1()
{
	but1 = cmdMessenger.readBinArg<int16_t>();
}
//Hier werden die Daten dem Elbow und dem Wrist_Ver empfangen und verarbeitet wenn es nötig ist.
void OnCmdSendRot45() {
	rot45 = cmdMessenger.readBinArg<int16_t>();
	rot45 = 180 - rot45;
}
/*
Loop
*/
void loop() {
	//Cmd Messenger SerialData Transfer starten
	cmdMessenger.feedinSerialData();
	//Anzeigelampen setzen.
	//Orange Lampe ausschalten
	digitalWrite(52, LOW);
	//Grüne Lampe Ein Schalten
	digitalWrite(53, HIGH);
	//Kinect Funktion Aktivierung wenn der Wahlschalter auf Kinect steht.
	if (but1 == 1) {
		KinectFunktion();
	}
	//AYAX Protokoll übertragen
	webserver.processConnection();
}
void KinectFunktion() {
	//Hier werden die einzelnen Positionen der Servomotoren geschrieben.
	if (BegrenzungFreigabe) {
		Begrenzung();
	}
	Braccio.ServoMovement(20, rot1, rot2, rot45, rot45, 90, rot6);
}

//Hier wird die maximale Tiefe begrenzt, damit man nicht zu tief den Arm auflegen kann.
void Begrenzung() {
	if (rot45 >= 180) {
		if (rot2 > 76) {
			rot2 = 76;
		}
	}
	else {
		if (rot45 < 180 && rot45 >= 174) {
			if (rot2 > 81) {
				rot2 = 81;
			}
		}
		else {
			if (rot45 < 174 && rot45 >= 168) {
				if (rot2 > 86) {
					rot2 = 86;
				}
			}
			else {
				if (rot45 < 168 && rot45 >= 162) {
					if (rot2 > 91) {
						rot2 = 91;
					}
				}
				else {
					if (rot45 < 162 && rot45 >= 156) {
						if (rot2 > 96) {
							rot2 = 96;
						}
					}
					else {
						if (rot45 < 156 && rot45 >= 150) {
							if (rot2 > 101) {
								rot2 = 101;
							}
						}
						else {
							if (rot45 < 150 && rot45 >= 144) {
								if (rot2 > 106) {
									rot2 = 106;
								}
							}
							else {
								if (rot45 < 144 && rot45 >= 138) {
									if (rot2 > 111) {
										rot2 = 111;
									}
								}
								else {
									if (rot45 < 138 && rot45 >= 132) {
										if (rot2 > 116) {
											rot2 = 116;
										}
									}
									else {
										if (rot45 < 132 && rot45 >= 125) {
											if (rot2 > 121) {
												rot2 = 121;
											}
										}
										else {
											if (rot45 < 125 && rot45 >= 118) {
												if (rot2 > 126) {
													rot2 = 126;
												}
											}
											else {
												if (rot45 < 118 && rot45 >= 111) {
													if (rot2 > 131) {
														rot2 = 131;
													}
												}
												else {
													if (rot45 < 111 && rot45 >= 104) {
														if (rot2 > 136) {
															rot2 = 136;
														}
													}
													else {
														if (rot45 < 104 && rot45 >= 97) {
															if (rot2 > 141) {
																rot2 = 141;
															}
														}
														else {
															if (rot45 < 97) {
																if (rot2 > 146) {
																	rot2 = 146;
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}