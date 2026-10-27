// 28/10/2026 00:30 - REM TIME calcule en course (il n'etait alimente qu'en mode demo, donc toujours "--:--") : tours de carburant restants x temps moyen des tours propres (hors pit) de l'historique
// 27/10/2026 19:30 - Nettoyage : theme Radar seul (autres themes, menu de selection et apercu supprimes), carte 3248S035 seule (support 2432S028 retire), reglages ouverts directement sur l'ecran SETTINGS (luminosite, reset Wi-Fi) ; aides d'ecran (fondu, toucher) reprises de ClassicTheme.inc
// 21/09/2026 14:54 - Selection du profil LGFX via BOARD_ESP32_3248S035 pour supporter la carte ESP32-3248S035R (ST7796 320x480) en plus de la 2432S028.
// 02/10/2026 20:46 - Ajout du mode demo (flag DASHBOARD_DEMO_MODE) : etat du dashboard anime sans telemetrie GT7 + cadre de test des limites de l'ecran.
// 06/10/2026 16:37 - Historique des derniers tours pour le bandeau du thème Radar (8 = LAP_STRIP_COUNT)
// 06/10/2026 23:09 - Rayons des pneus et temps restant dans DashboardState, demo: temperatures 50-90 et suppression du cadre de test
// 07/10/2026 01:02 - Historique glissant des 8 derniers tours (+ detection d'arret au stand) pour le bandeau du theme Radar
// 07/10/2026 14:06 - Suivi des tours terminés et détection du passage au pit (GT7 n'envoie aucun drapeau pit)
// 08/10/2026 12:16 - Rapport 0 ou 15: point mort (N) ou marche arriere (R), distingues par le drapeau InGear (a verifier en jeu)
//                    PIT Motif recherché: vitesse stabilisée (limiteur) -> arrêt moteur au ralenti -> vitesse stabilisée.
// 14/10/2026 15:59 - Suppression de tyreRadiusMm (zone RADIUS retiree du theme Radar : le rayon n'evolue pas avec l'usure)
// 14/10/2026 16:21 - Pression turbo (boost - 1, en bar, negative possible) et maximum de la course dans DashboardState, simulee en mode demo
// 14/10/2026 16:49 - Marche arriere: rapport 0 = R, 15 = N (le drapeau InGear ne distinguait pas R de N) ; historique ramene a 7 tours
// 16/10/2026 14:08 - Pit: suppression du test du regime moteur a l'arret (ralenti de plusieurs milliers de tr/min sur certaines voitures)
// 16/10/2026 14:37 - LED RGB (carte 3248S035) : clignotement bleu a l'approche de minAlertRPM quand le regime monte ; rpmAlertMinRpm dans DashboardState ; remise en forme de la phase d'arret du pit
// 25/10/2026 20:50 - Analyse HANDLING et debit remis a zero a chaque nouvelle session (retour en piste) et nouvelle course ; debit en % du reservoir par minute (fuelLevel GT7 = pourcentage)
// 25/10/2026 16:40 - Debit de carburant instantane en L/min (FuelFlow, essai "debitmetre") dans DashboardState.fuelFlow, calcule a chaque paquet hors pause et simule en mode demo
// 25/10/2026 16:20 - Pression turbo minimale de la course (turboBoostMinBar) : GT7 peut descendre a -2 bar ou plus, l'echelle de depression du cadran TURBO s'y adapte
// 25/10/2026 14:45 - Mode demo : lever de pied (ni frein ni accelerateur) juste avant le freinage, pour alimenter la nouvelle phase L de l'analyse de comportement
// 23/10/2026 15:12 - Ecran d'attente : "By Alain Vautrain" centre sous le titre, a la hauteur de la version
// 23/10/2026 15:05 - Parametres de detection du pit deplaces dans include/tuning.h (valeurs d'Alain reprises : LAP_PIT_STABLE_MS 1000, LAP_PIT_MAX_KMH 80)
// 23/10/2026 15:00 - Pit: LAP_PIT_MAX_KMH porte a 80 km/h (reglage d'Alain, limiteur plus rapide sur certains circuits)
// 23/10/2026 14:47 - Ecran de configuration Wi-Fi : nom et version du fork + credit du projet d'origine en bas ; titre, credit et position du credit mis en commun avec l'ecran d'attente
// 23/10/2026 14:44 - Ecran d'attente : titre "GT7 Extended Dashboard", version du fork (extended_version.h) sous le titre a droite, credit "Based on GT7 Dash by caa1211" descendu de 20 px
// 23/10/2026 00:06 - Mode demo : accelerateur partiel en fin de rapport pour alimenter la phase "milieu de virage" de l'analyse de comportement
// 21/10/2026 10:51 - Trace de comportement (lacet mesure / neutre, braquage, ecarts de vitesse des roues) dans DashboardState.handling, alimentee par la telemetrie et simulee en mode demo
#ifndef __SHCUSTOMPROTOCOL_H__
#define __SHCUSTOMPROTOCOL_H__

// Mode demo : 0 par defaut, passe a 1 par l'environnement esp32-3248s035-demo (platformio.ini).
#ifndef DASHBOARD_DEMO_MODE
#define DASHBOARD_DEMO_MODE 0
#endif

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
// selezione la configurazione nella cartella lgfx_user
// #include <LGFX_AUTODETECT.hpp>  // Preparare la classe "LGFX"
// #include <lgfx_user/LGFX_ESP8266_sample.hpp>
#include <lgfx_user/LGFX_ESP32_esp32-3248s035.hpp>
//  dashboard + free deck grafica
#include <Arduino.h>
#include <Preferences.h>
#include <map>
#include <BleGamepad.h> // libreria bluetooth
#include <GT7DerivedMetrics.h>
#include "version.h"
#include "extended_version.h"
#include "tuning.h"
#include "dashboard/HandlingTrace.h"
#include "dashboard/FuelFlow.h"
#if defined(BOARD_RGB_LED_BLUE_PIN)
#include "dashboard/ShiftLed.h"
#endif

static LGFX tft;

static const int RESET_WAITING_TIME = 3;
static const int SCREEN_WIDTH = 320;
static const int SCREEN_HEIGHT = 240;
static const int X_CENTER = SCREEN_WIDTH / 2;

// Nom du fork et credit du projet d'origine (ecran d'attente et ecran Wi-Fi).
static const char *const GT7_EXTENDED_TITLE = "GT7 Extended Dashboard";
static const char *const GT7_EXTENDED_AUTHOR = "By Alain Vautrain";
static const char *const GT7_ORIGIN_CREDIT = "Based on GT7 Dash by caa1211";
// Ligne du credit, en bas d'ecran : 20 px sous la zone 320x240 des ecrans d'attente
// et de reglages (l'ecran de la carte 3248S035 fait 320 px de haut).
static const int CREDIT_Y = SCREEN_HEIGHT - 12 + 20;

// Canonical dashboard state. Telemetry parsing and derived-value logic update
// this state; the Radar theme decides only how to present it.
struct DashboardState
{
	int rpmPercent = 50;
	int rpmRedLineSetting = 90;
	int engineRpm = 0;
	int rpmAlertMinRpm = 0;		// seuil d'alerte du jeu (minAlertRPM) en tr/min, 0 = inconnu
	bool rpmAlertRangeValid = false;
	bool revLimitAlertActive = false;
	String gear = "N";
	String speed = "0";
	String currentLapTime = "00:00.00";
	String lastLapTime = "00:00.00";
	String bestLapTime = "00.00.00";
	String sessionBestLiveDeltaSeconds = "+0.000";
	String sessionBestLiveDeltaProgressSeconds = "0.00";
	String tyrePressureFrontLeft = "00.0";
	String tyrePressureFrontRight = "00.0";
	String tyrePressureRearLeft = "00.0";
	String fuelAlertActive = "False";
	String tcLevel = "0";
	String tcFilteredLevel = "0";
	String tcActive = "0";
	String absLevel = "0";
	String absFilteredLevel = "0";
	String absActive = "0";
	String isTCCutNull = "True";
	String tcTcCut = "0  0";
	String brakeBias = "0";
	bool fuelIsEV = false;
	bool fuelValueValid = false;
	String fuelLabel = "FUEL";
	String fuelDisplayValue = "--";
	int fuelProgressPercent = 100;
	String brake = "0";
	String lapInvalidated = "False";
	float tyreTemperatures[4] = {NAN, NAN, NAN, NAN};
	String remainingTime = "--:--";	
	// Pression turbo relative en bar (negative en deceleration) ; NAN = voiture sans turbo.
	float turboBoostBar = NAN;
	// Plus haute pression vue depuis le debut de la course (echelle du cadran du theme Radar).
	float turboBoostMaxBar = 0.0f;
	// Plus forte depression vue depuis le debut de la course. Physiquement on ne peut pas
	// descendre sous -1 bar (vide absolu), mais GT7 envoie jusqu'a -2 bar ou plus sur
	// certaines voitures : l'echelle de depression du cadran suit donc la valeur vue.
	float turboBoostMinBar = 0.0f;
	// Analyse de comportement (sous/survirage, DGL) par phase de virage, affichee par le theme Radar.
	HandlingTrace handling;
	// Debit de carburant instantane (% du reservoir par minute), affiche en petit dans la case FUEL du theme Radar.
	FuelFlow fuelFlow;
	String gameRunning = "False";
	// Historique glissant des derniers tours (affiche par le bandeau du theme Radar).
	static constexpr int LAP_HISTORY_SIZE = 7;
	int lapHistoryCount = 0;
	int lapHistoryNumber[LAP_HISTORY_SIZE] = {};
	int32_t lapHistoryMs[LAP_HISTORY_SIZE] = {};
	bool lapHistoryPit[LAP_HISTORY_SIZE] = {};
	int32_t bestLapMs = -1;

	void updateTurbo(bool hasTurbo, float boostBar)
	{
		if (!hasTurbo || !isfinite(boostBar))
		{
			turboBoostBar = NAN;
			return;
		}
		turboBoostBar = boostBar;
		if (boostBar > turboBoostMaxBar) turboBoostMaxBar = boostBar;
		if (boostBar < turboBoostMinBar) turboBoostMinBar = boostBar;
	}

	void clearLapHistory()
	{
		lapHistoryCount = 0;
	}

	// Temps moyen des tours de l'historique sans passage au pit (ms), -1 si aucun.
	int32_t averageCleanLapMs() const
	{
		int64_t total = 0;
		int count = 0;
		for (int i = 0; i < lapHistoryCount; ++i)
		{
			if (lapHistoryPit[i] || lapHistoryMs[i] <= 0) continue;
			total += lapHistoryMs[i];
			++count;
		}
		return count > 0 ? static_cast<int32_t>(total / count) : -1;
	}

	// Ajoute un tour a droite ; quand la fenetre est pleine, le plus ancien sort.
	void pushLapHistory(int lapNumber, int32_t lapMs, bool pit)
	{
		if (lapHistoryCount == LAP_HISTORY_SIZE)
		{
			for (int i = 1; i < LAP_HISTORY_SIZE; ++i)
			{
				lapHistoryNumber[i - 1] = lapHistoryNumber[i];
				lapHistoryMs[i - 1] = lapHistoryMs[i];
				lapHistoryPit[i - 1] = lapHistoryPit[i];
			}
			--lapHistoryCount;
		}
		lapHistoryNumber[lapHistoryCount] = lapNumber;
		lapHistoryMs[lapHistoryCount] = lapMs;
		lapHistoryPit[lapHistoryCount] = pit;
		++lapHistoryCount;
	}	
};

std::map<String, String> prevData;

// configuro ble per gamepad

BleGamepad bleGamepad("ESP32 Touch Gamepad", "YourCompany", 100);
BleGamepadConfiguration bleGamepadConfig;

static void showWifiConnectionFailedScreen()
{
	tft.fillScreen(TFT_BLACK);
	tft.setTextDatum(MC_DATUM);

	tft.setTextColor(TFT_RED);
	tft.drawString("WI-FI FAILED", 160, 75);

	tft.setTextColor(TFT_WHITE);
	tft.drawString("Check your password", 160, 125);
	tft.drawString("and try again", 160, 155);

	tft.setTextColor(TFT_LIGHTGREY);
	tft.drawString("Setup portal will restart", 160, 205);
}

static void showWifiSetupScreen()
{
	tft.fillScreen(TFT_BLACK);

	tft.setTextDatum(MC_DATUM);

	tft.setTextColor(TFT_CYAN, TFT_BLACK);
	tft.setTextSize(2);
	tft.drawString("WI-FI SETUP", 160, 35);

	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.setTextSize(1.5);
	tft.drawString("Connect phone to:", 160, 85);

	tft.setTextColor(TFT_YELLOW, TFT_BLACK);
	tft.setTextSize(2);
	tft.drawString("GT7-DASH-SETUP", 160, 120);

	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.setTextSize(1.5);
	tft.drawString("Open:", 160, 160);

	tft.setTextColor(TFT_GREEN, TFT_BLACK);
	tft.setTextSize(2);
	tft.drawString("192.168.4.1", 160, 195);

	// Nom et version du fork, puis credit du projet d'origine juste en dessous.
	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.setTextSize(1);
	tft.drawCentreString(
		String(GT7_EXTENDED_TITLE) + "  v" + GT7_EXTENDED_DASH_VERSION,
		SCREEN_WIDTH / 2,
		CREDIT_Y - 12,
		1);
	tft.setTextColor(tft.color565(120, 120, 120), TFT_BLACK);
	tft.drawCentreString(
		GT7_ORIGIN_CREDIT,
		SCREEN_WIDTH / 2,
		CREDIT_Y,
		1);
}

bool forceUpdate = false; // Variabile per forzare l'aggiornamento delle celle

class SHCustomProtocol : private DashboardState
{
private:
	Preferences dashboardPreferences;
	bool dashboardPreferencesReady = false;

	uint16_t touchX, touchY; // definisce i due interi per gestione touchscreen

	// A lightweight correction applied after the panel driver's own touch
	// mapping. Persisting only four rotations keeps recovery deterministic.
	enum class TouchRotation : uint8_t
	{
		Deg0 = 0,
		Deg90 = 1,
		Deg180 = 2,
		Deg270 = 3,
	};
	TouchRotation touchRotation = TouchRotation::Deg0;
	TouchRotation pendingTouchRotation = TouchRotation::Deg0;
	bool touchCalibrationVerified = false;
	bool touchCalibrationEntryArmed = false;
	bool touchCalibrationAwaitingSecondTap = false;
	unsigned long touchCalibrationFirstTapTime = 0;
	uint16_t originalTouchX = 0;
	uint16_t originalTouchY = 0;

	static constexpr uint8_t FADE_STEP = 5;
	static constexpr uint8_t FADE_DELAY_MS = 4;
	static constexpr unsigned long SCREEN_SLEEP_TIMEOUT = 5UL * 60UL * 1000UL;
	// static constexpr unsigned long SCREEN_SLEEP_TIMEOUT = 10000;
	static constexpr uint8_t DEFAULT_BRIGHTNESS_PERCENT = 80;
	static constexpr uint8_t MIN_BRIGHTNESS_PERCENT = 20;
	static constexpr uint8_t BRIGHTNESS_STEP_PERCENT = 10;
	static constexpr unsigned long BRIGHTNESS_SAVE_DELAY_MS = 750UL;
	static constexpr bool TOUCH_SCREEN_CONTROL_ENABLED = true;

	// Connecting 畫面動畫設定
	static constexpr unsigned long CONNECT_ANIMATION_INTERVAL = 1000;
	static constexpr int CONNECT_DOT_COUNT = 5;

	uint8_t userBrightnessPercent = DEFAULT_BRIGHTNESS_PERCENT;
	uint8_t currentBrightness = 255;
	bool brightnessSavePending = false;
	unsigned long brightnessChangedTime = 0;
	unsigned long gameStoppedTime = 0;

	bool gameStoppedTimerStarted = false;
	bool screenSleeping = false;

	// 是否由使用者手動關閉
	bool screenOffByUser = false;

	// Tap the active dashboard to open the touch-driven Settings menu.
	enum class SettingsScreen : uint8_t
	{
		Closed,
		DeviceSettings,
		WifiResetConfirmation,
		TouchCalibration,
	};

	static constexpr unsigned long SETTINGS_TIMEOUT_MS = 15000UL;
	static constexpr unsigned long TOUCH_CALIBRATION_DOUBLE_TAP_MS = 3000UL;
	SettingsScreen settingsScreen = SettingsScreen::Closed;
	unsigned long settingsLastInteractionTime = 0;
	int settingsPressedButton = -1;
	bool wifiResetConfirmOpen = false;
	bool wifiResetRequested = false;

	// 記錄上一筆 GT7 執行狀態，用來偵測 False -> True
	bool previousGameRunning = false;
#if defined(BOARD_RGB_LED_BLUE_PIN)
	ShiftLed shiftLed;
#endif

	// 避免第一筆資料被誤判為狀態切換
	bool gameRunningInitialized = false;

	static void applyTouchRotation(
		uint16_t sourceX,
		uint16_t sourceY,
		TouchRotation rotation,
		uint16_t &resultX,
		uint16_t &resultY)
	{
		const uint32_t x = constrain(sourceX, 0, SCREEN_WIDTH - 1);
		const uint32_t y = constrain(sourceY, 0, SCREEN_HEIGHT - 1);
		switch (rotation)
		{
		case TouchRotation::Deg90:
			resultX = static_cast<uint16_t>(
				(y * (SCREEN_WIDTH - 1) + (SCREEN_HEIGHT - 1) / 2) /
				(SCREEN_HEIGHT - 1));
			resultY = static_cast<uint16_t>(
				(((SCREEN_WIDTH - 1) - x) * (SCREEN_HEIGHT - 1) +
					(SCREEN_WIDTH - 1) / 2) /
				(SCREEN_WIDTH - 1));
			break;
		case TouchRotation::Deg180:
			resultX = SCREEN_WIDTH - 1 - x;
			resultY = SCREEN_HEIGHT - 1 - y;
			break;
		case TouchRotation::Deg270:
			resultX = static_cast<uint16_t>(
				(((SCREEN_HEIGHT - 1) - y) * (SCREEN_WIDTH - 1) +
					(SCREEN_HEIGHT - 1) / 2) /
				(SCREEN_HEIGHT - 1));
			resultY = static_cast<uint16_t>(
				(x * (SCREEN_HEIGHT - 1) + (SCREEN_WIDTH - 1) / 2) /
				(SCREEN_WIDTH - 1));
			break;
		case TouchRotation::Deg0:
		default:
			resultX = static_cast<uint16_t>(x);
			resultY = static_cast<uint16_t>(y);
			break;
		}
	}

	int calibrationRotationForTouch(uint16_t rawX, uint16_t rawY) const
	{
		constexpr int targetX = SCREEN_WIDTH / 2;
		constexpr int targetY = 18;
		constexpr int hitHalfWidth = 48;
		constexpr int hitHeight = 26;
		int closestRotation = -1;
		uint32_t closestDistance = UINT32_MAX;
		for (uint8_t value = 0; value < 4; ++value)
		{
			uint16_t candidateX = 0;
			uint16_t candidateY = 0;
			applyTouchRotation(rawX, rawY,
				static_cast<TouchRotation>(value), candidateX, candidateY);
			if (abs(static_cast<int>(candidateX) - targetX) > hitHalfWidth ||
				candidateY > hitHeight)
				continue;
			const int dx = static_cast<int>(candidateX) - targetX;
			const int dy = static_cast<int>(candidateY) - targetY;
			const uint32_t distance = dx * dx + dy * dy;
			if (distance < closestDistance)
			{
				closestDistance = distance;
				closestRotation = value;
			}
		}
		return closestRotation;
	}

	uint8_t normalBrightness() const
	{
		return static_cast<uint8_t>(
			(userBrightnessPercent * 255U + 50U) / 100U);
	}

	void scheduleBrightnessSave()
	{
		brightnessSavePending = true;
		brightnessChangedTime = millis();
	}

	void saveBrightnessIfDue()
	{
		if (!brightnessSavePending ||
			millis() - brightnessChangedTime < BRIGHTNESS_SAVE_DELAY_MS)
			return;
		if (dashboardPreferencesReady)
			dashboardPreferences.putUChar("brightness", userBrightnessPercent);
		brightnessSavePending = false;
	}

	// Reglages memorises en flash (NVS) : luminosite et correction du toucher.
	void loadDashboardPreferences()
	{
		dashboardPreferencesReady = dashboardPreferences.begin("gt7dash", false);
		if (dashboardPreferencesReady)
		{
			// Cle du theme choisi dans l'ancien menu : plus utilisee (theme Radar seul).
			if (dashboardPreferences.isKey("theme"))
				dashboardPreferences.remove("theme");
			const uint8_t storedBrightness = dashboardPreferences.getUChar(
				"brightness", DEFAULT_BRIGHTNESS_PERCENT);
			userBrightnessPercent =
				storedBrightness >= MIN_BRIGHTNESS_PERCENT && storedBrightness <= 100
					? storedBrightness
					: DEFAULT_BRIGHTNESS_PERCENT;
			const uint8_t storedTouchRotation = dashboardPreferences.getUChar(
				"touchRot", static_cast<uint8_t>(TouchRotation::Deg0));
			touchRotation = storedTouchRotation <= static_cast<uint8_t>(TouchRotation::Deg270)
				? static_cast<TouchRotation>(storedTouchRotation)
				: TouchRotation::Deg0;
		}
		pendingTouchRotation = touchRotation;
	}

#if INCLUDE_GT7_WIFI
	// 最近一次真正收到新 GT7 UDP 封包的時間
	uint32_t lastGT7PacketTime = 0;
	int32_t lastGT7PacketId = -1;
	bool hasReceivedGT7Packet = false;
	bool gt7CarOnTrack = false;

	// GT7 衍生數據：ABS 與剩餘油量圈數。
	GT7DerivedMetrics derivedMetrics;

#endif

	void resetLapDifference()
	{
		previousBestLapMs = -1;
		lastProcessedLapMs = -1;
		sessionBestLiveDeltaSeconds = "+0.000";
	}

	String formatLapTimeMs(int32_t milliseconds)
	{
		if (milliseconds < 0)
		{
			return "--:--.---";
		}

		const uint32_t value = static_cast<uint32_t>(milliseconds);
		const uint32_t minutes = value / 60000UL;
		const uint32_t seconds = (value % 60000UL) / 1000UL;
		const uint32_t millisPart = value % 1000UL;

		char buffer[16];
		snprintf(
			buffer,
			sizeof(buffer),
			"%02lu:%02lu.%03lu",
			static_cast<unsigned long>(minutes),
			static_cast<unsigned long>(seconds),
			static_cast<unsigned long>(millisPart));

		return String(buffer);
	}

	// "m:ss" sous l'heure, "h:mm:ss" au-dela (courses d'endurance) ; "--:--" si inconnu.
	String formatRemainingTime(float laps, int32_t lapMs)
	{
		if (laps < 0.0f || lapMs <= 0)
			return "--:--";

		const uint32_t totalSeconds =
			static_cast<uint32_t>(lroundf(laps * lapMs / 1000.0f));
		const uint32_t hours = totalSeconds / 3600UL;
		const uint32_t minutes = (totalSeconds % 3600UL) / 60UL;
		const uint32_t seconds = totalSeconds % 60UL;

		char buffer[16];
		if (hours > 0)
			snprintf(buffer, sizeof(buffer), "%lu:%02lu:%02lu",
				static_cast<unsigned long>(hours),
				static_cast<unsigned long>(minutes),
				static_cast<unsigned long>(seconds));
		else
			snprintf(buffer, sizeof(buffer), "%lu:%02lu",
				static_cast<unsigned long>(minutes),
				static_cast<unsigned long>(seconds));
		return String(buffer);
	}

	String formatDeltaSeconds(float value)
	{
		if (isnan(value))
		{
			return "+0.000";
		}

		char buffer[16];
		snprintf(buffer, sizeof(buffer), "%+.3f", value);
		return String(buffer);
	}

	String formatLastBestDifference(int32_t lastLapMs, int32_t bestLapMs)
	{
		if (lastLapMs <= 0 || bestLapMs <= 0)
		{
			return "+0.000";
		}

		const float differenceSeconds =
			static_cast<float>(lastLapMs - bestLapMs) / 1000.0f;

		return formatDeltaSeconds(differenceSeconds);
	}

	int32_t parseLapTimeStringMs(String value)
	{
		value.trim();

		if (value.length() == 0 || value.indexOf('-') >= 0)
		{
			return -1;
		}

		const int colon = value.indexOf(':');
		const int dot = value.lastIndexOf('.');

		if (colon < 0 || dot < colon)
		{
			return -1;
		}

		const int32_t minutes = value.substring(0, colon).toInt();
		const int32_t seconds = value.substring(colon + 1, dot).toInt();
		String millisText = value.substring(dot + 1);

		while (millisText.length() < 3)
		{
			millisText += "0";
		}

		if (millisText.length() > 3)
		{
			millisText = millisText.substring(0, 3);
		}

		return minutes * 60000L + seconds * 1000L + millisText.toInt();
	}

	// Connecting 畫面狀態
	bool connectingScreenActive = false;
	// int connectingDotIndex = 0;
	int connectingAnimationStep = 0;
	unsigned long lastConnectingAnimationTime = 0;

public:
	/*
	CUSTOM PROTOCOL CLASS
	SEE https://github.com/zegreatclan/SimHub/wiki/Custom-Arduino-hardware-support

	GENERAL RULES :
		- ALWAYS BACKUP THIS FILE, reinstalling/updating SimHub would overwrite it with the default version.
		- Read data AS FAST AS POSSIBLE in the read function
		- NEVER block the arduino (using delay for instance)
		- Make sure the data read in "read()" function READS ALL THE DATA from the serial port matching the custom protocol definition
		- Idle function is called hundreds of times per second, never use it for slow code, arduino performances would fall
		- If you use library suspending interrupts make sure to use it only in the "read" function when ALL data has been read from the serial port.
			It is the only interrupt safe place

	COMMON FUNCTIONS :
		- FlowSerialReadStringUntil('\n')
			Read the incoming data up to the end (\n) won't be included
		- FlowSerialReadStringUntil(';')
			Read the incoming data up to the separator (;) separator won't be included
		- FlowSerialDebugPrintLn(string)
			Send a debug message to simhub which will display in the log panel and log file (only use it when debugging, it would slow down arduino in run conditions)

	*/

	// Called when starting the arduino (setup method in main sketch)
	void setup()
	{
		tft.init();
#if defined(BOARD_RGB_LED_BLUE_PIN)
		shiftLed.begin(BOARD_RGB_LED_RED_PIN, BOARD_RGB_LED_GREEN_PIN,
			BOARD_RGB_LED_BLUE_PIN, BOARD_RGB_LED_ACTIVE_LOW);
#endif
		loadDashboardPreferences();
		tft.setRotation(DASHBOARD_DISPLAY_ROTATION);
		tft.setBrightness(normalBrightness());
		currentBrightness = normalBrightness();
		tft.fillScreen(TFT_BLACK);
		screenSleeping = false;
		//    Serial.begin(115200);   //x debug
		//    Serial.println("Test seriale avviato!"); //x debug
		gameStoppedTimerStarted = false;
#if INCLUDE_GT7_WIFI
		derivedMetrics.reset();
#endif

		// bleGamepadConfig.setAutoReport(true); // in false non invia i comandi a windows
		// bleGamepadConfig.setAxesMax(32760);
		// bleGamepadConfig.setIncludeSlider1(false);
		// bleGamepadConfig.setIncludeXAxis(false);
		// bleGamepadConfig.setIncludeYAxis(false);
		// bleGamepadConfig.setIncludeZAxis(false);
		// bleGamepadConfig.setIncludeRxAxis(false);
		// bleGamepadConfig.setIncludeRyAxis(false);
		// bleGamepadConfig.setIncludeRzAxis(false);
		// bleGamepadConfig.setButtonCount(numButtons); // Variabile per il numero di pulsanti
		// bleGamepad.begin(&bleGamepadConfig);
		// Serial.println("Configurazione BleGamepad completata.");   //x debug
	}

#if INCLUDE_GT7_WIFI

#ifndef DASHBOARD_PIT_DEBUG
#define DASHBOARD_PIT_DEBUG 0	// 1 = trace les changements de phase sur le port série
#endif
	// Parametres de detection du pit (LAP_PIT_*) : voir include/tuning.h.


	enum PitPhase : uint8_t { PIT_IDLE, PIT_APPROACH, PIT_STOPPED, PIT_LEAVING, PIT_DONE };

	struct LapTracker
	{
		int previousLapCount = 0;
		bool pitSeen = false;			// pit confirmé pendant le tour en cours
		PitPhase pitPhase = PIT_IDLE;
		int pitEntryLap = 0;			// tour pendant lequel la phase 1 a commencé
		float stableRefKmh = 0.0f;
		uint32_t stableSince = 0;
		uint32_t phaseSince = 0;
		uint32_t idleSince = 0;			// début de la période de ralenti en cours (0 = aucune)
		uint32_t longestIdleMs = 0;		// plus longue période de ralenti pendant cet arrêt
	} lapTracker;

	void setPitPhase(PitPhase phase, float speedKmh, float rpm)
	{
		lapTracker.pitPhase = phase;
#if DASHBOARD_PIT_DEBUG
		Serial.printf("[PIT] phase=%d vitesse=%.0f km/h rpm=%.0f tour=%d\n",
			(int)phase, speedKmh, rpm, lapTracker.previousLapCount);
#else
		(void)speedKmh;
		(void)rpm;
#endif
	}

	// Marque le tour d'entrée au pit: tour en cours, ou tour déjà affiché dans l'historique.
	void markPitLap(int lap, int currentLapCount)
	{
		if (lap >= currentLapCount)
		{
			lapTracker.pitSeen = true;
			return;
		}
		for (int i = 0; i < lapHistoryCount; ++i)
			if (lapHistoryNumber[i] == lap)
				lapHistoryPit[i] = true;
	}

	void trackPitStop(int lapCount, float speedKmh, float rpm, uint32_t now)
	{
		LapTracker &t = lapTracker;
		if (fabsf(speedKmh - t.stableRefKmh) > LAP_PIT_STABLE_BAND_KMH)
		{
			t.stableRefKmh = speedKmh;
			t.stableSince = now;
		}
		const bool stable = now - t.stableSince >= LAP_PIT_STABLE_MS &&
			speedKmh >= LAP_PIT_MIN_KMH && speedKmh <= LAP_PIT_MAX_KMH;

		switch (t.pitPhase)
		{
		case PIT_IDLE:		// phase 1: vitesse stabilisée
			if (stable)
			{
				t.pitEntryLap = lapCount;
				t.phaseSince = now;
				setPitPhase(PIT_APPROACH, speedKmh, rpm);
			}
			break;
		case PIT_APPROACH:	// attente de l'arrêt
			if (stable)
				t.phaseSince = now;	// dernier instant stable
			if (speedKmh < LAP_PIT_STOPPED_KMH)
			{
				t.idleSince = 0;
				t.longestIdleMs = 0;
				setPitPhase(PIT_STOPPED, speedKmh, rpm);
			}
			else if (now - t.phaseSince > LAP_PIT_APPROACH_TIMEOUT_MS)
				setPitPhase(PIT_IDLE, speedKmh, rpm);
			break;
		case PIT_STOPPED:	// phase 2: arrêt (vitesse nulle, le régime n'est pas testé)
			if (speedKmh < LAP_PIT_STOPPED_KMH)
			{
				// Seule la vitesse nulle compte (le regime au ralenti varie trop selon les voitures).
				if (t.idleSince == 0)
					t.idleSince = now;
				const uint32_t idleMs = now - t.idleSince;
				if (idleMs > t.longestIdleMs)
					t.longestIdleMs = idleMs;
			}
			else
			{
				// La voiture repart: l'arrêt n'est valide que s'il y a eu assez de ralenti continu.
				if (t.longestIdleMs >= LAP_PIT_STOP_MS)
				{
					t.phaseSince = now;
					setPitPhase(PIT_LEAVING, speedKmh, rpm);
				}
				else
					setPitPhase(PIT_IDLE, speedKmh, rpm);
			}
			break;
		case PIT_LEAVING:	// phase 3: vitesse de nouveau stabilisée
			if (stable)
			{
				markPitLap(t.pitEntryLap, lapCount);
				setPitPhase(PIT_DONE, speedKmh, rpm);
			}
			else if (now - t.phaseSince > LAP_PIT_LEAVE_TIMEOUT_MS)
				setPitPhase(PIT_IDLE, speedKmh, rpm);
			break;
		case PIT_DONE:		// attend la fin de la vitesse stable avant de réarmer
			if (!stable)
				setPitPhase(PIT_IDLE, speedKmh, rpm);
			break;
		}
	}

	void updateLapHistory(int lapCount, int32_t lastLapMs, int32_t bestLapMsValue,
		float speedMs, float rpm, uint16_t flags)
	{
		const uint32_t now = millis();
		bestLapMs = bestLapMsValue > 0 ? bestLapMsValue : -1;

		// Le compteur de tours recule: nouvelle course, on repart de zéro.
		if (lapCount < lapTracker.previousLapCount)
		{
			clearLapHistory();
			lapTracker = LapTracker();
			turboBoostMaxBar = 0.0f;
			turboBoostMinBar = 0.0f;
			handling.reset();
		}

		// Fin de tour: lapCount augmente (le faux passage 0 -> 1 du départ est ignoré).
		if (lapCount > lapTracker.previousLapCount)
		{
			if (lapTracker.previousLapCount >= 1 && lastLapMs > 0)
				pushLapHistory(lapTracker.previousLapCount, lastLapMs, lapTracker.pitSeen);
			lapTracker.pitSeen = false;
		}
		lapTracker.previousLapCount = lapCount;

		// Voiture en piste et jeu non en pause.
		const bool carActive = (flags & (1U << 0)) != 0 && (flags & (1U << 1)) == 0;
		if (carActive && lapCount >= 1)
		{
			trackPitStop(lapCount, speedMs * 3.6f, rpm, now);
		}
		else
		{
			lapTracker.pitPhase = PIT_IDLE;
			lapTracker.stableSince = now;
		}
	}

	bool readGT7Wifi()
	{
		gt7Packet = gt7Telem.readData();

		const int32_t packetId =
			gt7Packet.packetContent.packetId;

		/*
		 * readData() 沒收到新資料時會保留上一筆 Packet。
		 * 因此用 packetId 是否改變，判斷這一圈是否真的收到新封包。
		 */
		if (hasReceivedGT7Packet && packetId == lastGT7PacketId)
		{
			return false;
		}

		// 尚未收到任何有效資料時，忽略初始化的空 Packet。
		if (!hasReceivedGT7Packet && packetId == 0 &&
			gt7Packet.packetContent.magic == 0)
		{
			return false;
		}

		hasReceivedGT7Packet = true;
		lastGT7PacketId = packetId;
		lastGT7PacketTime = millis();

		const auto &data = gt7Packet.packetContent;

		// 速度與檔位
		speed = String(static_cast<int>(data.speed * 3.6f));

		const int currentGear =
			gt7Telem.getCurrentGearFromByte();

		// Codage GT7 du rapport engage: 0 = marche arriere, 15 = point mort.
		// (Le drapeau InGear restait a 0 en marche arriere: R s'affichait N.)
		gear = currentGear == 0    ? "R"
			   : currentGear == 15 ? "N"
								   : String(currentGear);

		// RPM Bar
		const float rpm = data.EngineRPM;
		const float maxRpm = data.maxAlertRPM;
		const float redLineRpm = data.minAlertRPM;
		engineRpm = max(0, static_cast<int>(lroundf(rpm)));
		static constexpr float FALLBACK_MAX_RPM = 10000.0f;
		static constexpr float MIN_REASONABLE_MAX_RPM = 1000.0f;
		static constexpr float MAX_REASONABLE_MAX_RPM = 25000.0f;
		const bool maxRpmValid = maxRpm >= MIN_REASONABLE_MAX_RPM &&
			maxRpm <= MAX_REASONABLE_MAX_RPM;
		const float effectiveMaxRpm = maxRpmValid ? maxRpm : FALLBACK_MAX_RPM;

		rpmAlertRangeValid = maxRpmValid && redLineRpm > 0.0f &&
			redLineRpm < maxRpm;

		rpmPercent = constrain(
			static_cast<int>((rpm / effectiveMaxRpm) * 100.0f),
			0,
			100);

		rpmRedLineSetting = rpmAlertRangeValid
			? constrain(
				static_cast<int>((redLineRpm / maxRpm) * 100.0f),
				1,
				99)
			: 90;
		rpmAlertMinRpm = rpmAlertRangeValid ? static_cast<int>(lroundf(redLineRpm)) : 0;

		// 圈速：currentLap 只有使用 C Packet 時才有值
		currentLapTime = formatLapTimeMs(data.currentLap);
		lastLapTime = formatLapTimeMs(data.lastLaptime);
		bestLapTime = formatLapTimeMs(data.bestLaptime);

		// 圈數
		const int currentLap = max(0, static_cast<int>(data.lapCount));
		const int totalLapCount = max(0, static_cast<int>(data.totalLaps));

		// 上一圈與最佳圈的圈速差。新最佳圈會顯示 +0.000。
		sessionBestLiveDeltaSeconds =
			updateLastBestDifference(
				data.lastLaptime,
				data.bestLaptime);

		// 此欄位不再顯示 Delta P，畫面改為油門／煞車圖條。
		sessionBestLiveDeltaProgressSeconds = "";

		if (totalLapCount > 0)
		{
			tyrePressureRearLeft = String(currentLap) + "/" + String(totalLapCount);
		}
		else
		{
			tyrePressureRearLeft = String(currentLap);
		}

		// 目前封包只有賽前起跑位置，沒有比賽中的即時排名
		if (data.RaceStartPosition > 0)
		{
			tyrePressureFrontRight = String(data.RaceStartPosition);
		}
		else
		{
			tyrePressureFrontRight = "--";
		}

		// 油量百分比
		// Normalize ICE and EV energy into separate display and progress values.
		// For EVs, fuelLevel is treated as remaining kWh. The fixed 60 kWh
		// reference is only a visual scale because GT7 does not expose capacity.
		static constexpr float EV_CAPACITY_THRESHOLD = 0.1f;
		static constexpr float EV_VISUAL_FULL_KWH = 60.0f;
		static constexpr float MAX_REASONABLE_EV_KWH = 500.0f;
		const bool validCapacity = isfinite(data.fuelCapacity) &&
			data.fuelCapacity >= 0.0f;
		const bool validFuelLevel = isfinite(data.fuelLevel) &&
			data.fuelLevel >= 0.0f && data.fuelLevel < MAX_REASONABLE_EV_KWH;
		// Powertrain classification comes only from the capacity field (0x48).
		// Keep it independent from fuelLevel so a transient/invalid energy value
		// cannot make an EV label flicker back to FUEL.
		const bool detectedEV = validCapacity &&
			data.fuelCapacity < EV_CAPACITY_THRESHOLD;
		const bool validEvFuel = detectedEV && validFuelLevel &&
			data.fuelLevel > 0.0f;
		const bool validIceFuel = validCapacity && validFuelLevel &&
			data.fuelCapacity >= EV_CAPACITY_THRESHOLD;
		const bool nextFuelValueValid = validEvFuel || validIceFuel;
		const String nextFuelLabel = detectedEV ? "EV" : "FUEL";

		if (fuelIsEV != detectedEV || fuelValueValid != nextFuelValueValid)
		{
			derivedMetrics.fuel.reset();
			if (fuelLabel != nextFuelLabel)
				forceUpdate = true;
		}

		fuelIsEV = detectedEV;
		fuelValueValid = nextFuelValueValid;
		fuelLabel = nextFuelLabel;

		if (validEvFuel)
		{
			// GT7 updates EV energy in coarse steps. A whole-number display keeps
			// every theme compact and avoids implying unavailable precision.
			fuelDisplayValue = String(data.fuelLevel, 0);
			fuelProgressPercent = constrain(
				static_cast<int>(lroundf(
					data.fuelLevel / EV_VISUAL_FULL_KWH * 100.0f)),
				0,
				100);
		}
		else if (validIceFuel)
		{
			const float fuelPercent = constrain(
				(data.fuelLevel / data.fuelCapacity) * 100.0f,
				0.0f,
				100.0f);
			fuelDisplayValue = String(fuelPercent, 0);
			fuelProgressPercent = constrain(
				static_cast<int>(lroundf(fuelPercent)), 0, 100);
		}
		else
		{
			fuelDisplayValue = "--";
			fuelProgressPercent = 100;
		}

		// Keep legacy fields synchronized for existing protocol helpers.
		brakeBias = fuelDisplayValue;
		fuelAlertActive = String(fuelProgressPercent);

		for (int tyreIndex = 0; tyreIndex < 4; tyreIndex++)
		{
			tyreTemperatures[tyreIndex] = data.tyreTemp[tyreIndex];
		}

		// 由獨立 library 估算剩餘油量圈數。
		// 注意：GT7FuelEstimator 接收的是公升，不是百分比。
		if (validIceFuel)
			derivedMetrics.fuel.update(data.fuelLevel, currentLap);

		// Debit instantane (% du reservoir par minute) : thermique uniquement, et pas
		// pendant la pause (le temps s'arrete mais les paquets continuent d'arriver).
		const bool gamePaused = (static_cast<uint16_t>(data.flags) &
			static_cast<uint16_t>(SimulatorFlags::Paused)) != 0;
		fuelFlow.update(validIceFuel ? data.fuelLevel / data.fuelCapacity * 100.0f : NAN,
			validIceFuel && !gamePaused, millis());

		const float estimatedFuelLaps = validIceFuel
			? derivedMetrics.fuel.remainingLaps()
			: -1.0f;

		if (!fuelIsEV && estimatedFuelLaps >= 0.0f)
		{
			tyrePressureFrontLeft = String(estimatedFuelLaps, 1);
		}
		else
		{
			tyrePressureFrontLeft = "--";
		}

		// REM TIME : autonomie en temps = tours de carburant restants x temps moyen des
		// tours propres de l'historique. Il faut un tour complet pour l'estimation de
		// consommation et un tour sans pit dans l'historique : "--:--" en attendant.
		remainingTime = formatRemainingTime(
			!fuelIsEV && estimatedFuelLaps >= 0.0f ? estimatedFuelLaps : -1.0f,
			averageCleanLapMs());

		// 油門、煞車：UDP 範圍 0～255，轉為百分比
		const int throttlePercent =
			constrain(static_cast<int>(data.throttle * 100.0f / 255.0f), 0, 100);

		const int brakePercent =
			constrain(static_cast<int>(data.brake * 100.0f / 255.0f), 0, 100);

		handling.update({data.speed, data.angularVelocity, data.wheelSteeringAngle,
			data.wheelBase, data.wheelRPS, data.tyreRadius,
			throttlePercent, brakePercent}, millis());

		tcLevel = String(throttlePercent);
		absLevel = String(brakePercent);
		// GT7 exposes both driver pedal position and the filtered output actually
		// applied by the simulation (after traction/ABS intervention).
		tcFilteredLevel = String(constrain(
			static_cast<int>(data.throttleFiltered * 100.0f / 255.0f), 0, 100));
		absFilteredLevel = String(constrain(
			static_cast<int>(data.brakeFiltered * 100.0f / 255.0f), 0, 100));

		const uint16_t flags = static_cast<uint16_t>(data.flags);
		revLimitAlertActive =
			(flags & static_cast<uint16_t>(SimulatorFlags::RevLimiterBlinkAlertActive)) != 0;

		// GT7 flags bit 0：車輛目前位於賽道／駕駛畫面中。
		// 回到選單、離開賽道或載入畫面時會變成 false。
		gt7CarOnTrack = (flags & (1U << 0)) != 0;

		updateLapHistory(currentLap, data.lastLaptime, data.bestLaptime, data.speed, data.EngineRPM, flags);

		// Pression turbo: GT7 envoie boost decale de +1 (1.0 = pression atmospherique).
		updateTurbo((flags & static_cast<uint16_t>(SimulatorFlags::HasTurbo)) != 0,
			data.boost - 1.0f);

		const bool tcsIsActive = (flags & (1U << 11)) != 0;

		tcActive = tcsIsActive ? "True" : "False";

		// GT7 沒有直接提供 ABS Active，使用四輪角速度與實際輪胎半徑估算。
		derivedMetrics.abs.update(
			data.speed * 3.6f,
			static_cast<float>(brakePercent),
			data.wheelRPS,
			data.tyreRadius,
			millis());

		absActive = derivedMetrics.abs.isActive()
						? "True"
						: "False";

		isTCCutNull = "True";
		tcTcCut = "0";
		brake = "0";
		lapInvalidated = "False";

		return true;
	}

#endif

	void read()
	{
#if !INCLUDE_GT7_WIFI

		// 1～9：主要行車及圈速資料
		speed = FlowSerialReadStringUntil(';').toInt();
		gear = FlowSerialReadStringUntil(';');
		rpmPercent = FlowSerialReadStringUntil(';').toInt();
		rpmRedLineSetting = FlowSerialReadStringUntil(';').toInt();

		currentLapTime = FlowSerialReadStringUntil(';');
		lastLapTime = FlowSerialReadStringUntil(';');
		bestLapTime = FlowSerialReadStringUntil(';');

		sessionBestLiveDeltaSeconds =
			FlowSerialReadStringUntil(';');

		sessionBestLiveDeltaProgressSeconds =
			FlowSerialReadStringUntil(';');

		// Delta 欄改為上一圈－最佳圈；仍讀取舊欄位以維持 Protocol 相容。
		sessionBestLiveDeltaSeconds = formatLastBestDifference(
			parseLapTimeStringMs(lastLapTime),
			parseLapTimeStringMs(bestLapTime));

		// 10：預估剩餘油量圈數
		tyrePressureFrontLeft =
			FlowSerialReadStringUntil(';');

		// 11：起跑位置／排位位置
		tyrePressureFrontRight =
			FlowSerialReadStringUntil(';');

		// 12：目前圈數／總圈數，例如 3/10
		tyrePressureRearLeft =
			FlowSerialReadStringUntil(';');

		// 13：低油量警示
		fuelAlertActive =
			FlowSerialReadStringUntil(';');

		// 14：油門百分比
		tcLevel =
			FlowSerialReadStringUntil(';');

		// 15：TC 是否介入
		tcActive =
			FlowSerialReadStringUntil(';');

		// 16：煞車百分比
		absLevel =
			FlowSerialReadStringUntil(';');
		// SimHub's existing protocol has no separate filtered pedal channels.
		// Mirror the input so the dual-layer renderer remains backward compatible.
		tcFilteredLevel = tcLevel;
		absFilteredLevel = absLevel;

		// 17：ABS 是否介入
		absActive =
			FlowSerialReadStringUntil(';');

		// 18：固定為 True
		isTCCutNull =
			FlowSerialReadStringUntil(';');

		// 19：保留欄位，固定為 0
		tcTcCut =
			FlowSerialReadStringUntil(';');

		// 20：剩餘油量百分比
		brakeBias =
			FlowSerialReadStringUntil(';');
		fuelIsEV = false;
		fuelValueValid = brakeBias != "--" && brakeBias.length() > 0;
		fuelLabel = "FUEL";
		fuelDisplayValue = brakeBias;
		fuelProgressPercent = fuelValueValid
			? constrain(static_cast<int>(lroundf(fuelAlertActive.toFloat())), 0, 100)
			: 100;

		// 21：保留欄位，固定為 0
		brake =
			FlowSerialReadStringUntil(';');

		// 22：本圈是否無效
		lapInvalidated =
			FlowSerialReadStringUntil(';');

		// 23：SimHub 是否正在接收遊戲資料
		gameRunning =
			FlowSerialReadStringUntil(';');

		// 清除可能存在的空白、\r
		gameRunning.trim();

		// Protocol 最後一欄也有分號，因此再讀掉封包結尾的換行
		FlowSerialReadStringUntil('\n');
#endif
		const bool isGameRunning =
			gameRunning == "True" ||
			gameRunning == "true" ||
			gameRunning == "TRUE" ||
			gameRunning == "1";

		/*
		 * 只有狀態真的從 False -> True，
		 * 才視為 GT7 新的一次啟動。
		 */
		bool gameJustStarted = false;

		if (gameRunningInitialized)
		{
			gameJustStarted =
				!previousGameRunning &&
				isGameRunning;
		}
		else
		{
			// 第一筆資料只用來建立初始狀態
			gameRunningInitialized = true;
		}

		/*
		 * GT7 從未執行變成執行：
		 * 無論之前是自動或手動關屏，都自動亮起。
		 */
		if (gameJustStarted)
		{
            resetLapDifference();

			screenSleeping = false;
			screenOffByUser = false;

			fadeScreenOn();

			forceUpdate = true;
			gameStoppedTimerStarted = false;
		}

		if (isGameRunning)
		{
			/*
			 * 遊戲持續執行時，只取消自動休眠倒數。
			 * 不直接開背光，避免手動關屏後馬上又亮。
			 */
			gameStoppedTimerStarted = false;
		}
		else
		{
			/*
			 * GT7 未執行時開始五分鐘倒數。
			 */
			if (!gameStoppedTimerStarted)
			{
				gameStoppedTimerStarted = true;
				gameStoppedTime = millis();
			}
		}

		// 最後再保存本次狀態，供下一筆資料比較
		previousGameRunning = isGameRunning;
	}

#if INCLUDE_GT7_WIFI
	void updateGT7GameState()
	{
		const bool telemetryIsAlive =
			lastGT7PacketTime != 0 &&
			millis() - lastGT7PacketTime < RESET_WAITING_TIME * 1000;

		// 不只要求 UDP 還活著，也要求車輛真的在賽道上。
		// 因此離開賽道但 GT7 仍持續送封包時，也會切回等待畫面。
		const bool isGameRunning =
			telemetryIsAlive && gt7CarOnTrack;

		bool gameJustStarted = false;

		if (gameRunningInitialized)
		{
			gameJustStarted =
				!previousGameRunning &&
				isGameRunning;
		}
		else
		{
			gameRunningInitialized = true;

			// 開機時 GT7 已經在執行，也直接亮屏
			gameJustStarted = isGameRunning;
		}

		if (gameJustStarted)
		{
            resetLapDifference();
			// Nouvelle session (retour en piste) : pas de valeurs de la session precedente.
			handling.reset();
			fuelFlow.reset();
			screenSleeping = false;
			screenOffByUser = false;

			fadeScreenOn();

			forceUpdate = true;
			gameStoppedTimerStarted = false;
		}

		if (isGameRunning)
		{
			gameStoppedTimerStarted = false;
		}
		else if (!gameStoppedTimerStarted)
		{
			gameStoppedTimerStarted = true;
			gameStoppedTime = millis();
		}

		previousGameRunning = isGameRunning;
	}

#endif

#if DASHBOARD_DEMO_MODE
	// Mode demo : anime l'etat du dashboard sans telemetrie GT7 (cycle de 12 s :
	// 6 rapports, regime qui monte, pedales, delta, carburant, temperatures pneus).
	void updateDemoState()
	{
		static uint32_t lastDemoUpdate = 0;
		const uint32_t now = millis();
		if (lastDemoUpdate != 0 && now - lastDemoUpdate < 33)
		{
			return; // ~30 images/s, proche du rythme de la telemetrie reelle
		}
		const bool firstUpdate = (lastDemoUpdate == 0);
		lastDemoUpdate = now;

		const float cycle = (now % 12000) / 12000.0f;
		const float gearPos = cycle * 6.0f;
		const int gearIndex = static_cast<int>(gearPos) + 1; // 1..6
		const float gearPhase = gearPos - static_cast<float>(gearIndex - 1); // 0..1

		rpmPercent = static_cast<int>(45.0f + gearPhase * 53.0f);
		rpmRedLineSetting = 78;
		rpmAlertRangeValid = true;
		engineRpm = rpmPercent * 80;
		rpmAlertMinRpm = rpmRedLineSetting * 80;	// meme echelle que engineRpm
		revLimitAlertActive = rpmPercent >= 97;

		gear = String(gearIndex);
		speed = String(static_cast<int>(40.0f + (gearIndex - 1) * 38.0f + gearPhase * 38.0f));

		// Pedales : l'interface les lit dans absLevel/absFilteredLevel (frein)
		// et tcLevel/tcFilteredLevel (accelerateur).
		const bool braking = gearPhase >= 0.9f;
		// Lever de pied juste avant le freinage: phase "L" de HandlingTrace.
		const bool lifting = !braking && gearPhase >= 0.85f;
		// Accelerateur partiel avant le lever de pied: phase "milieu de virage" de HandlingTrace.
		const int throttle = braking ? 15 : (lifting ? 0 : (gearPhase >= 0.7f ? 30 : 100));
		const int brakeInput = braking ? 80 : 0;
		tcLevel = String(throttle);
		tcFilteredLevel = String(throttle * 9 / 10);
		absLevel = String(brakeInput);
		absFilteredLevel = String(brakeInput * 9 / 10);
		absActive = braking ? "1" : "0";
		tcActive = (gearPhase < 0.1f) ? "1" : "0";

		char buffer[16];
		const uint32_t lapMs = now % 90000;
		snprintf(buffer, sizeof(buffer), "%02u:%02u.%03u",
			static_cast<unsigned>(lapMs / 60000),
			static_cast<unsigned>((lapMs / 1000) % 60),
			static_cast<unsigned>(lapMs % 1000));
		currentLapTime = String(buffer);
		lastLapTime = "01:25.104";
		bestLapTime = "01:24.382";
		lapInvalidated = ((now / 1000) % 20 < 3) ? "True" : "False";

		snprintf(buffer, sizeof(buffer), "%+.3f", 0.8f * sinf(now / 3000.0f));
		sessionBestLiveDeltaSeconds = String(buffer);
		sessionBestLiveDeltaProgressSeconds = "0.00";

		// Champs historiques du protocole : tyrePressureRearLeft = LAP,
		// tyrePressureFrontRight = POS, tyrePressureFrontLeft = REM.
		tyrePressureRearLeft = "3/10";
		tyrePressureFrontRight = "4";
		tyrePressureFrontLeft = "12";
		brakeBias = "80";

		const int fuelPercent = 100 - static_cast<int>((now / 1000) % 100);
		fuelIsEV = false;
		fuelValueValid = true;
		fuelLabel = "FUEL";
		fuelDisplayValue = String(fuelPercent);
		fuelProgressPercent = fuelPercent;
		fuelAlertActive = (fuelPercent < 15) ? "True" : "False";

		for (int i = 0; i < 4; ++i)
		{
			tyreTemperatures[i] = 70.0f + 20.0f * sinf(now / 2500.0f + i);
		}
		remainingTime = "18:42";
		// Turbo: depression au freinage, pression qui monte avec le regime ailleurs.
		updateTurbo(true, braking ? -0.6f : 0.1f + 1.1f * gearPhase);
		// Debit de carburant: suit l'accelerateur (0,3 %/min au ralenti, 3 %/min a fond).
		fuelFlow.percentPerMinute = 0.3f + 2.7f * throttle / 100.0f;

		// Comportement: virage gauche/droite toutes les 8 s ; sous-virage au freinage
		// (lacet a 85 % du neutre), leger sous-virage au lever de pied (95 %), neutre a
		// accelerateur partiel, survirage a pleine charge (120 %).
		{
			const float demoSpeedMs = (40.0f + (gearIndex - 1) * 38.0f + gearPhase * 38.0f) / 3.6f;
			const float demoWheelBase = 2.65f;
			const float demoSteer = 0.08f * sinf(now / 1273.0f);
			const float demoYawRatio = braking ? 0.85f : (lifting ? 0.95f : (throttle < 50 ? 1.0f : 1.2f));
			const float demoAngular[3] = {0.0f,
				demoYawRatio * demoSpeedMs * tanf(demoSteer) / demoWheelBase, 0.0f};
			const float demoSteerAngles[2] = {demoSteer, demoSteer};
			const float demoRadius[4] = {0.33f, 0.33f, 0.34f, 0.34f};
			// Avant libre: ecart naturel du virage (voie ~1,6 m). Arriere: differentiel
			// bloque a 70 % (R/F ~0,3), et patinage de la roue interieure en debut de
			// rapport a pleine charge (R/F negatif).
			const float trackSpeedMs = 0.8f * demoAngular[1];
			const float rearTrackSpeedMs = 0.3f * trackSpeedMs;
			const float spin = (throttle > 90 && gearPhase < 0.3f) ? 1.5f : 0.0f;
			const float spinLeft = demoAngular[1] > 0.0f ? spin : 0.0f;	// virage a gauche: interieur = gauche
			const float spinRight = demoAngular[1] > 0.0f ? 0.0f : spin;
			const float demoWheelRps[4] = {
				(demoSpeedMs - trackSpeedMs) / demoRadius[0],
				(demoSpeedMs + trackSpeedMs) / demoRadius[1],
				(demoSpeedMs - rearTrackSpeedMs + spinLeft) / demoRadius[2],
				(demoSpeedMs + rearTrackSpeedMs + spinRight) / demoRadius[3]};
			handling.update({demoSpeedMs, demoAngular, demoSteerAngles, demoWheelBase,
				demoWheelRps, demoRadius, throttle, brakeInput}, now);
		}
		
		// Historique des tours simule: un tour toutes les 12 s, un tour sur 5 "au stand".
		const uint32_t demoLap = now / 12000;
		clearLapHistory();
		const uint32_t firstDemoLap = demoLap > LAP_HISTORY_SIZE ? demoLap - (LAP_HISTORY_SIZE - 1) : 1;
		for (uint32_t lap = firstDemoLap; lap <= demoLap; ++lap)
		{
			const bool demoPit = (lap % 5 == 0);
			pushLapHistory(lap, 84000 + (lap * 7919) % 2500 + (demoPit ? 9000 : 0), demoPit);
		}
		bestLapMs = -1;
		for (int i = 0; i < lapHistoryCount; ++i)
			if (!lapHistoryPit[i] && (bestLapMs < 0 || lapHistoryMs[i] < bestLapMs))
				bestLapMs = lapHistoryMs[i];		

		gameRunning = "True";

		// Simule une session GT7 en cours : pas de ecran d'attente, pas de mise en veille.
		previousGameRunning = true;
		gameStoppedTimerStarted = false;
		if (firstUpdate)
		{
			screenSleeping = false;
			screenOffByUser = false;
			forceUpdate = true;
		}
	}

#endif

	void loop()
	{
#if INCLUDE_GT7_WIFI
		static uint32_t lastHeartbeatTime = 0;
		const uint32_t now = millis();
		// GT7 需要持續收到 heartbeat 才會繼續傳送遙測資料。
		if (now - lastHeartbeatTime >= 500)
		{
			lastHeartbeatTime = now;
			gt7Telem.sendHeartbeat();
		}

#if DASHBOARD_DEMO_MODE
		updateDemoState();
#else
		readGT7Wifi();
		updateGT7GameState();
#endif
#endif

#if defined(BOARD_RGB_LED_BLUE_PIN)
		// LED eteinte hors course (pas de telemetrie) et ecran en veille.
		shiftLed.update(engineRpm, rpmAlertMinRpm,
			previousGameRunning && !screenSleeping, millis());
#endif

		/*
		 * GT7 停止五分鐘，自動關閉背光。
		 */
		if (!screenSleeping &&
			gameStoppedTimerStarted &&
			millis() - gameStoppedTime >= SCREEN_SLEEP_TIMEOUT)
		{
			screenSleeping = true;
			screenOffByUser = false;
			fadeScreenOff();
		}

		// Touch calibration is an idle-only recovery aid. Live telemetry always
		// wins immediately, discarding any unconfirmed candidate so this screen
		// can never hold the dashboard or its runtime services open.
		if (settingsScreen == SettingsScreen::TouchCalibration && previousGameRunning)
		{
			closeSettings();
		}

		// Keep reading touch while asleep so a tap can wake the display and a
		// long press can open Settings.
		readTouch();

		// 確認畫面開啟時，不讓 Dashboard 或 Connecting 畫面蓋回來。
		if (settingsScreen != SettingsScreen::Closed)
		{
			return;
		}

		if (screenSleeping)
		{
			return;
		}

		// 還沒收到 GT7 Telemetry 時顯示等待畫面。
		if (!previousGameRunning)
		{
			updateConnectingScreen();
			return;
		}

		// 從 Connecting 切回主儀表時，完整重畫一次。
		if (connectingScreenActive)
			redrawDashboard();

		drawRadarDashboard(static_cast<DashboardState &>(*this), forceUpdate);

		forceUpdate = false;
	}

	void idle() {}

#include "dashboard/themes/RadarTheme.inc"

	void drawTouchCalibrationHint(bool confirm)
	{
		const char *label = confirm ? "TOUCH AGAIN" : "TOUCH SETUP";
		const uint16_t textColor = confirm
			? tft.color565(218, 111, 148)
			: tft.color565(105, 105, 105);
		constexpr int iconRadius = 4;
		constexpr int iconGap = 5;
		tft.setTextFont(1);
		const int textWidth = tft.textWidth(label);
		const int groupWidth = iconRadius * 2 + iconGap + textWidth;
		const int groupX = X_CENTER - groupWidth / 2;
		const int iconX = groupX + iconRadius - 2;
		const int textX = groupX + iconRadius * 2 + iconGap;

		tft.fillRect(62, 4, 196, 29, TFT_BLACK);
		tft.drawCircle(iconX, 17, iconRadius, textColor);
		tft.fillCircle(iconX, 17, 1, textColor);
		tft.setTextDatum(ML_DATUM);
		tft.setTextColor(textColor, TFT_BLACK);
		tft.drawString(label, textX, 18, 1);
	}

	void drawConnectingScreenBase()
	{
		tft.fillScreen(TFT_BLACK);

		tft.setTextPadding(0);
		tft.setTextDatum(MC_DATUM);

		static const char *const title = GT7_EXTENDED_TITLE;
		static constexpr int titleY = 75;
		static constexpr int titleFont = 4;
		tft.setTextColor(TFT_WHITE, TFT_BLACK);
		tft.drawCentreString(
			title,
			SCREEN_WIDTH / 2,
			titleY,
			titleFont);

		// Sous le titre, sur une meme ligne : auteur du fork au centre, version
		// du fork alignee sur le bord droit du titre.
		tft.setTextFont(titleFont);
		const int titleWidth = tft.textWidth(title);
		const int subtitleY = titleY + tft.fontHeight(titleFont) + 2;
		tft.setTextColor(tft.color565(170, 170, 170), TFT_BLACK);
		tft.setTextDatum(TC_DATUM);
		tft.drawString(GT7_EXTENDED_AUTHOR, SCREEN_WIDTH / 2, subtitleY, 1);
		tft.setTextDatum(TR_DATUM);
		tft.drawString(
			String("v") + GT7_EXTENDED_DASH_VERSION,
			SCREEN_WIDTH / 2 + titleWidth / 2,
			subtitleY,
			1);
		tft.setTextDatum(MC_DATUM);

		tft.setTextColor(
			tft.color565(120, 120, 120),
			TFT_BLACK);

		// The asymmetric top-centre label lets a mismatched touch rotation be
		// inferred after two matching intentional taps while telemetry is idle.
		drawTouchCalibrationHint(false);

		tft.drawCentreString(
			"Waiting for Telemetry",
			SCREEN_WIDTH / 2,
			120,
			2);

		const int barWidth = 110;
		const int barHeight = 3;
		const int barX = (SCREEN_WIDTH - barWidth) / 2;
		const int barY = 162;

		tft.fillRect(
			barX,
			barY,
			barWidth,
			barHeight,
			tft.color565(28, 28, 28));

		tft.drawCentreString(
			"Tap for settings",
			SCREEN_WIDTH / 2,
			SCREEN_HEIGHT - 26,
			1);

		// Credit du projet d'origine (position: voir CREDIT_Y).
		tft.drawCentreString(
			GT7_ORIGIN_CREDIT,
			SCREEN_WIDTH / 2,
			CREDIT_Y,
			1);
	}

	void drawConnectingBar()
	{
		const int barWidth = 110;
		const int barHeight = 3;

		const int barX = (SCREEN_WIDTH - barWidth) / 2;
		const int barY = 162;

		// 每次先重畫整條深灰底
		tft.fillRect(
			barX,
			barY,
			barWidth,
			barHeight,
			tft.color565(28, 28, 28));

		const int highlightWidth = 32;

		// 讓流光可以從左側外面進入，再從右側離開
		const int travelWidth = barWidth + highlightWidth * 2;
		const int highlightX =
			(connectingAnimationStep % travelWidth) - highlightWidth;

		/*
		 * 將亮區切成多段灰階：
		 *
		 * 暗 → 灰 → 亮灰 → 灰 → 暗
		 *
		 * 因為高度只有 3px，看起來會像柔和的流光。
		 */
		const int segmentCount = 16;
		const int segmentWidth =
			(highlightWidth + segmentCount - 1) / segmentCount;

		for (int i = 0; i < segmentCount; i++)
		{
			const float position =
				(float)i / (segmentCount - 1);

			// 三角形亮度，中央最亮、兩側漸暗
			float brightness =
				1.0f - fabsf(position * 2.0f - 1.0f);

			// 亮度範圍約 45～190，避免白得太搶眼
			uint8_t gray =
				45 + (uint8_t)(brightness * 145);

			int segmentX =
				barX + highlightX + i * segmentWidth;

			int drawWidth = segmentWidth + 1;

			// 裁切左邊界
			if (segmentX < barX)
			{
				drawWidth -= barX - segmentX;
				segmentX = barX;
			}

			// 裁切右邊界
			if (segmentX + drawWidth > barX + barWidth)
			{
				drawWidth =
					barX + barWidth - segmentX;
			}

			if (drawWidth > 0)
			{
				tft.fillRect(
					segmentX,
					barY,
					drawWidth,
					barHeight,
					tft.color565(gray, gray, gray));
			}
		}
	}
	void updateConnectingScreen()
	{
		if (!connectingScreenActive)
		{
			tft.fillScreen(TFT_BLACK);
			drawConnectingScreenBase();

			connectingAnimationStep = 0;
			lastConnectingAnimationTime = 0;
			connectingScreenActive = true;
		}

		const unsigned long now = millis();

		// 數值越大，動畫移動越慢
		if (now - lastConnectingAnimationTime >= 65)
		{
			lastConnectingAnimationTime = now;

			connectingAnimationStep += 2;

			const int highlightWidth = 32;
			const int barWidth = 110;
			const int travelWidth =
				barWidth + highlightWidth * 2;

			if (connectingAnimationStep >= travelWidth)
			{
				connectingAnimationStep = 0;
			}

			drawConnectingBar();
		}
	}

	bool isActiveValue(String value)
	{
		value.trim();
		return value == "True" ||
			   value == "true" ||
			   value == "TRUE" ||
			   value == "1";
	}

	int32_t previousBestLapMs = -1;
	int32_t lastProcessedLapMs = -1;

	String updateLastBestDifference(
		int32_t lastLapMs,
		int32_t bestLapMs)
	{
		if (lastLapMs <= 0 || bestLapMs <= 0)
		{
			return "+0.000";
		}

		// 同一個上一圈可能每個封包重複傳送，只處理一次。
		if (lastLapMs == lastProcessedLapMs)
		{
			return sessionBestLiveDeltaSeconds;
		}

		lastProcessedLapMs = lastLapMs;

		String result = "+0.000";

		if (previousBestLapMs > 0)
		{
			result = formatDeltaSeconds(
				static_cast<float>(lastLapMs - previousBestLapMs) / 1000.0f);
		}

		// 本圈處理完後，才更新保存的最佳圈。
		previousBestLapMs = bestLapMs;

		return result;
	}

	// Aides d'ecran (reprises de l'ancien ClassicTheme.inc) : fondu de la luminosite,
	// redessin complet du tableau de bord, zone touchee.
	void fadeToBrightness(uint8_t targetBrightness)
	{
		if (currentBrightness == targetBrightness)
			return;

		int brightness = currentBrightness;

		if (brightness < targetBrightness)
		{
			while (brightness < targetBrightness)
			{
				brightness += FADE_STEP;

				if (brightness > targetBrightness)
					brightness = targetBrightness;

				tft.setBrightness(brightness);
				delay(FADE_DELAY_MS);
			}
		}
		else
		{
			while (brightness > targetBrightness)
			{
				brightness -= FADE_STEP;

				if (brightness < targetBrightness)
					brightness = targetBrightness;

				tft.setBrightness(brightness);
				delay(FADE_DELAY_MS);
			}
		}

		currentBrightness = targetBrightness;
	}

	void fadeScreenOn()
	{
		fadeToBrightness(normalBrightness());
	}

	void fadeScreenOff()
	{
		fadeToBrightness(0);
	}

	// Ecran efface et caches vides : le tableau de bord est entierement redessine
	// au prochain passage (apres l'ecran de connexion ou la fermeture des reglages).
	void redrawDashboard()
	{
		tft.fillScreen(TFT_BLACK);
		prevData.clear();
		connectingScreenActive = false;
		connectingAnimationStep = 0;
		forceUpdate = true;
	}

	bool touchInside(int x, int y, int width, int height) const
	{
		return touchX >= x && touchX < x + width &&
			touchY >= y && touchY < y + height;
	}

	void drawSettingsButton(
		int x,
		int y,
		int width,
		int height,
		const String &label,
		bool pressed,
		bool active = false,
		bool destructive = false)
	{
		const uint16_t normalFill = destructive
			? tft.color565(92, 24, 28)
			: active
				? tft.color565(18, 74, 104)
				: tft.color565(42, 46, 52);
		const uint16_t fill = pressed ? tft.color565(38, 126, 150) : normalFill;
		const uint16_t border = active ? TFT_WHITE : TFT_LIGHTGREY;
		tft.fillRoundRect(x, y, width, height, 7, fill);
		tft.drawRoundRect(x, y, width, height, 7, border);
		if (active)
			tft.drawRoundRect(x + 1, y + 1, width - 2, height - 2, 6, border);
		tft.setTextColor(TFT_WHITE, fill);
		tft.setTextDatum(MC_DATUM);
		tft.drawString(label, x + width / 2, y + height / 2, 2);
	}

	void drawDeviceBrightnessValue()
	{
		static LGFX_Sprite brightnessSprite(&tft);
		static bool spriteCreated = false;
		if (!spriteCreated)
		{
			brightnessSprite.setColorDepth(16);
			spriteCreated = brightnessSprite.createSprite(116, 48) != nullptr;
		}
		if (spriteCreated)
		{
			brightnessSprite.fillSprite(TFT_BLACK);
			brightnessSprite.setTextColor(TFT_WHITE, TFT_BLACK);
			brightnessSprite.setTextDatum(MC_DATUM);
			brightnessSprite.drawString(
				String(userBrightnessPercent) + "%", 58, 24, 4);
			brightnessSprite.pushSprite(102, 72);
		}
		else
		{
			tft.fillRect(102, 72, 116, 48, TFT_BLACK);
			tft.setTextColor(TFT_WHITE, TFT_BLACK);
			tft.setTextDatum(MC_DATUM);
			tft.drawString(String(userBrightnessPercent) + "%", X_CENTER, 96, 4);
		}
	}

	void drawTouchCalibrationScreen(int pressedButton = -1)
	{
		tft.fillScreen(TFT_BLACK);
		tft.setTextPadding(0);
		tft.setTextDatum(MC_DATUM);
		tft.setTextColor(TFT_WHITE, TFT_BLACK);
		tft.drawString("TOUCH SETUP", X_CENTER, 27, 4);

		tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
		tft.drawString(
			touchCalibrationVerified ? "Touch verified" : "Tap the target to verify",
			X_CENTER, 59, 2);

		const uint16_t targetColor = touchCalibrationVerified
			? tft.color565(88, 190, 130)
			: tft.color565(210, 82, 126);
		tft.drawCircle(252, 104, 15, targetColor);
		tft.drawCircle(252, 104, 7, targetColor);
		tft.drawFastHLine(232, 104, 41, targetColor);
		tft.drawFastVLine(252, 84, 41, targetColor);

		drawSettingsButton(25, 169, 125, 50, "CANCEL", pressedButton == 1);
		drawSettingsButton(170, 169, 125, 50, "SAVE", pressedButton == 2,
			touchCalibrationVerified);
		tft.setTextDatum(TL_DATUM);
	}

	void drawSettingsScreen(int pressedButton = -1)
	{
		tft.fillScreen(TFT_BLACK);
		tft.setTextPadding(0);
		tft.setTextDatum(MC_DATUM);

		if (settingsScreen == SettingsScreen::DeviceSettings)
		{
			tft.setTextColor(TFT_WHITE, TFT_BLACK);
			tft.drawString("SETTINGS", X_CENTER, 20, 4);
			tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
			tft.drawString("BRIGHTNESS", X_CENTER, 53, 2);
			drawSettingsButton(30, 72, 70, 48, "-", pressedButton == 0);
			drawSettingsButton(220, 72, 70, 48, "+", pressedButton == 1);
			drawDeviceBrightnessValue();
			drawSettingsButton(30, 132, 260, 38, "RESET WIFI", pressedButton == 2,
				false, true);
			drawSettingsButton(30, 182, 260, 38, "BACK", pressedButton == 3);
		}
		else if (settingsScreen == SettingsScreen::WifiResetConfirmation)
		{
			tft.setTextColor(TFT_WHITE, TFT_BLACK);
			tft.drawString("Reset saved Wi-Fi?", X_CENTER, 60, 4);
			tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
			tft.drawString("Device will restart", X_CENTER, 95, 2);
			drawSettingsButton(25, 135, 120, 62, "CANCEL", pressedButton == 0);
			drawSettingsButton(175, 135, 120, 62, "RESET", pressedButton == 1,
				false, true);
		}
		else if (settingsScreen == SettingsScreen::TouchCalibration)
		{
			drawTouchCalibrationScreen(pressedButton);
		}

		tft.setTextDatum(TL_DATUM);
	}

	void redrawSettingsButton(int button, bool pressed)
	{
		if (settingsScreen == SettingsScreen::DeviceSettings)
		{
			if (button == 0)
				drawSettingsButton(30, 72, 70, 48, "-", pressed);
			else if (button == 1)
				drawSettingsButton(220, 72, 70, 48, "+", pressed);
			else if (button == 2)
				drawSettingsButton(30, 132, 260, 38, "RESET WIFI", pressed,
					false, true);
			else if (button == 3)
				drawSettingsButton(30, 182, 260, 38, "BACK", pressed);
		}
		else if (settingsScreen == SettingsScreen::WifiResetConfirmation)
		{
			if (button == 0)
				drawSettingsButton(25, 135, 120, 62, "CANCEL", pressed);
			else if (button == 1)
				drawSettingsButton(175, 135, 120, 62, "RESET", pressed,
					false, true);
		}
		else if (settingsScreen == SettingsScreen::TouchCalibration)
		{
			if (button == 1)
				drawSettingsButton(25, 169, 125, 50, "CANCEL", pressed);
			else if (button == 2)
				drawSettingsButton(170, 169, 125, 50, "SAVE", pressed,
					touchCalibrationVerified);
		}
		tft.setTextDatum(TL_DATUM);
	}

	void showSettingsScreen(SettingsScreen screen)
	{
		screenSleeping = false;
		screenOffByUser = false;
		tft.setBrightness(normalBrightness());
		currentBrightness = normalBrightness();
		settingsScreen = screen;
		wifiResetConfirmOpen = screen == SettingsScreen::WifiResetConfirmation;
		settingsPressedButton = -1;
		settingsLastInteractionTime = millis();
		drawSettingsScreen();
	}

	void closeSettings()
	{
		settingsScreen = SettingsScreen::Closed;
		wifiResetConfirmOpen = false;
		settingsPressedButton = -1;
		touchCalibrationVerified = false;
		touchCalibrationEntryArmed = false;
		touchCalibrationAwaitingSecondTap = false;
		touchCalibrationFirstTapTime = 0;
		pendingTouchRotation = touchRotation;
		redrawDashboard();
	}

	void showTouchCalibration(TouchRotation candidate)
	{
		pendingTouchRotation = candidate;
		touchCalibrationVerified = false;
		showSettingsScreen(SettingsScreen::TouchCalibration);
	}

	void showWifiResetConfirm()
	{
		showSettingsScreen(SettingsScreen::WifiResetConfirmation);
	}

	void showWifiResettingScreen()
	{
		tft.fillScreen(TFT_BLACK);
		tft.setTextPadding(0);
		tft.setTextDatum(MC_DATUM);

		tft.setTextColor(TFT_CYAN, TFT_BLACK);
		tft.drawString("WI-FI RESET", SCREEN_WIDTH / 2, 90, 4);

		tft.setTextColor(TFT_WHITE, TFT_BLACK);
		tft.drawString("Restarting...", SCREEN_WIDTH / 2, 145, 2);

		tft.setTextDatum(TL_DATUM);
	}

	bool takeWifiResetRequest()
	{
		if (!wifiResetRequested)
		{
			return false;
		}

		wifiResetRequested = false;
		return true;
	}

	int settingsButtonAtTouch() const
	{
		if (settingsScreen == SettingsScreen::DeviceSettings)
		{
			if (touchInside(30, 72, 70, 48)) return 0;
			if (touchInside(220, 72, 70, 48)) return 1;
			if (touchInside(30, 132, 260, 38)) return 2;
			if (touchInside(30, 182, 260, 38)) return 3;
		}
		else if (settingsScreen == SettingsScreen::WifiResetConfirmation)
		{
			if (touchInside(25, 135, 120, 62)) return 0;
			if (touchInside(175, 135, 120, 62)) return 1;
		}
		else if (settingsScreen == SettingsScreen::TouchCalibration)
		{
			if (touchInside(224, 76, 56, 56)) return 0;
			const bool originalCancel =
				originalTouchX >= 25 && originalTouchX < 150 &&
				originalTouchY >= 169 && originalTouchY < 219;
			if (touchInside(25, 169, 125, 50) || originalCancel) return 1;
			if (touchCalibrationVerified && touchInside(170, 169, 125, 50)) return 2;
		}
		return -1;
	}

	void activateSettingsButton(int button)
	{
		if (settingsScreen == SettingsScreen::DeviceSettings)
		{
			if (button == 0 || button == 1)
			{
				const int adjustment = button == 0
					? -BRIGHTNESS_STEP_PERCENT
					: BRIGHTNESS_STEP_PERCENT;
				userBrightnessPercent = constrain(
					static_cast<int>(userBrightnessPercent) + adjustment,
					static_cast<int>(MIN_BRIGHTNESS_PERCENT), 100);
				tft.setBrightness(normalBrightness());
				currentBrightness = normalBrightness();
				scheduleBrightnessSave();
				settingsLastInteractionTime = millis();
				drawDeviceBrightnessValue();
			}
			else if (button == 2)
			{
				showWifiResetConfirm();
			}
			else if (button == 3)
			{
				closeSettings();
			}
		}
		else if (settingsScreen == SettingsScreen::WifiResetConfirmation)
		{
			if (button == 0)
			{
				showSettingsScreen(SettingsScreen::DeviceSettings);
			}
			else if (button == 1)
			{
				showWifiResettingScreen();
				wifiResetRequested = true;
			}
		}
		else if (settingsScreen == SettingsScreen::TouchCalibration)
		{
			if (button == 0)
			{
				touchCalibrationVerified = true;
				settingsLastInteractionTime = millis();
				drawTouchCalibrationScreen();
			}
			else if (button == 1)
			{
				closeSettings();
			}
			else if (button == 2 && touchCalibrationVerified)
			{
				touchRotation = pendingTouchRotation;
				if (dashboardPreferencesReady)
					dashboardPreferences.putUChar(
						"touchRot", static_cast<uint8_t>(touchRotation));
				closeSettings();
			}
		}
	}

	void readTouch()
	{
		saveBrightnessIfDue();
		if (!TOUCH_SCREEN_CONTROL_ENABLED)
		{
			return;
		}

		static bool wasTouched = false;
		static bool waitForReleaseAfterScreenChange = false;
		uint16_t rawTouchX = 0;
		uint16_t rawTouchY = 0;
		const bool isTouched = tft.getTouch(&rawTouchX, &rawTouchY);
		if (isTouched)
		{
			applyTouchRotation(
				rawTouchX, rawTouchY, touchRotation, originalTouchX, originalTouchY);
			const TouchRotation effectiveRotation =
				settingsScreen == SettingsScreen::TouchCalibration
					? pendingTouchRotation
					: touchRotation;
			applyTouchRotation(
				rawTouchX, rawTouchY, effectiveRotation, touchX, touchY);
		}

		if (touchCalibrationAwaitingSecondTap &&
			millis() - touchCalibrationFirstTapTime > TOUCH_CALIBRATION_DOUBLE_TAP_MS)
		{
			touchCalibrationAwaitingSecondTap = false;
			touchCalibrationFirstTapTime = 0;
			pendingTouchRotation = touchRotation;
			if (!previousGameRunning && !screenSleeping &&
				settingsScreen == SettingsScreen::Closed)
				drawTouchCalibrationHint(false);
		}

		if (waitForReleaseAfterScreenChange)
		{
			if (!isTouched)
			{
				waitForReleaseAfterScreenChange = false;
				wasTouched = false;
			}
			return;
		}

		// Waking always takes precedence over Settings touch targets. The first
		// tap only restores the display; it never activates a hidden button.
		if (screenSleeping)
		{
			if (!isTouched && wasTouched)
			{
				screenSleeping = false;
				screenOffByUser = false;
				fadeScreenOn();
				forceUpdate = true;
				if (!previousGameRunning)
				{
					gameStoppedTimerStarted = true;
					gameStoppedTime = millis();
				}
				else
				{
					gameStoppedTimerStarted = false;
				}

				if (settingsScreen != SettingsScreen::Closed)
					drawSettingsScreen();
			}
			wasTouched = isTouched;
			return;
		}

		if (isTouched && !wasTouched && !previousGameRunning)
		{
			gameStoppedTimerStarted = true;
			gameStoppedTime = millis();
			if (settingsScreen == SettingsScreen::Closed)
			{
				const int candidate = calibrationRotationForTouch(rawTouchX, rawTouchY);
				touchCalibrationEntryArmed = candidate >= 0;
				if (candidate >= 0)
				{
					if (touchCalibrationAwaitingSecondTap &&
						candidate != static_cast<int>(pendingTouchRotation))
					{
						touchCalibrationAwaitingSecondTap = false;
						touchCalibrationFirstTapTime = 0;
					}
					pendingTouchRotation = static_cast<TouchRotation>(candidate);
				}
			}
		}

		if (settingsScreen != SettingsScreen::Closed)
		{
			if (!isTouched && settingsLastInteractionTime != 0 &&
				millis() - settingsLastInteractionTime >= SETTINGS_TIMEOUT_MS)
			{
				closeSettings();
				wasTouched = false;
				return;
			}

			if (isTouched && !wasTouched)
			{
				settingsLastInteractionTime = millis();
				settingsPressedButton = settingsButtonAtTouch();
			}
			else if (!isTouched && wasTouched)
			{
				settingsLastInteractionTime = millis();
				const int releasedButton = settingsButtonAtTouch();
				const int pressedButton = settingsPressedButton;
				settingsPressedButton = -1;
				if (pressedButton >= 0 && releasedButton == pressedButton)
				{
					activateSettingsButton(pressedButton);
					waitForReleaseAfterScreenChange = true;
				}
			}

			wasTouched = isTouched;
			return;
		}

		if (!isTouched && wasTouched)
		{
			if (touchCalibrationEntryArmed)
			{
				if (touchCalibrationAwaitingSecondTap &&
					millis() - touchCalibrationFirstTapTime <= TOUCH_CALIBRATION_DOUBLE_TAP_MS)
				{
					touchCalibrationAwaitingSecondTap = false;
					showTouchCalibration(pendingTouchRotation);
				}
				else
				{
					touchCalibrationAwaitingSecondTap = true;
					touchCalibrationFirstTapTime = millis();
					drawTouchCalibrationHint(true);
				}
			}
			else
			{
				touchCalibrationAwaitingSecondTap = false;
				touchCalibrationFirstTapTime = 0;
				showSettingsScreen(SettingsScreen::DeviceSettings);
			}
			touchCalibrationEntryArmed = false;
			waitForReleaseAfterScreenChange = true;
		}

		wasTouched = isTouched;
	}

};

#endif