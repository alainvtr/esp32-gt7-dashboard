// 20/10/2026 16:57 - Suppression du test des couleurs au demarrage (LED ressoudee et validee, ordre des couleurs conforme)
// 16/10/2026 16:51 - LED validee (externe) : test de demarrage ramene a 0,4 s par couleur, trace serie desactivee
// 16/10/2026 15:15 - Test de demarrage allonge (3 s par couleur, 2 tours) avec relecture du niveau des broches, pour mesure au multimetre
// 16/10/2026 14:51 - Diagnostic LED eteinte: test rouge/vert/bleu au demarrage et trace serie [LED]
// 16/10/2026 14:37 - LED RGB de la carte : clignotement bleu a l'approche du seuil d'alerte regime (minAlertRPM), uniquement quand le regime monte
#pragma once

#include <Arduino.h>

// Trace sur le port serie (regime, seuil, sens, etat) toutes les 250 ms.
// Mettre a 1 pour diagnostiquer.
#ifndef DASHBOARD_SHIFT_LED_DEBUG
#define DASHBOARD_SHIFT_LED_DEBUG 0
#endif

// Pilotage de la LED RGB de la carte pour annoncer l'arrivee du seuil d'alerte
// du jeu (minAlertRPM, alias RPMFlashingAlertMin).
//
//  - zone d'approche : de APPROACH_START_RATIO x seuil jusqu'au seuil ;
//  - dans cette zone, si le regime MONTE : clignotement bleu, de plus en plus
//    rapide a mesure qu'on s'approche du seuil ;
//  - si le regime DESCEND, sous la zone, ou au-dela du seuil : LED eteinte.
//
// Le sens (monte / descend) est determine en comparant le regime a celui
// d'il y a DIRECTION_WINDOW_MS, avec une marge (DIRECTION_HYSTERESIS_RPM) pour
// ne pas reagir aux petites variations de la telemetrie. Tant que l'ecart
// reste dans la marge (regime stable), le dernier sens connu est conserve.
class ShiftLed
{
public:
	static constexpr float APPROACH_START_RATIO = 0.85f;	// debut de zone = 85 % du seuil
	static constexpr uint32_t BLINK_HALF_PERIOD_SLOW_MS = 200;	// debut de zone: 2,5 clignotements/s
	static constexpr uint32_t BLINK_HALF_PERIOD_FAST_MS = 40;	// pres du seuil: 12,5 clignotements/s
	static constexpr uint32_t DIRECTION_WINDOW_MS = 100;
	static constexpr int DIRECTION_HYSTERESIS_RPM = 40;

	void begin(int redPin, int greenPin, int bluePin, bool activeLow)
	{
		redPin_ = redPin;
		greenPin_ = greenPin;
		bluePin_ = bluePin;
		activeLow_ = activeLow;
		pinMode(redPin_, OUTPUT);
		pinMode(greenPin_, OUTPUT);
		pinMode(bluePin_, OUTPUT);
		// Rouge et vert ne servent pas pour l'instant: toujours eteints.
		digitalWrite(redPin_, offLevel());
		digitalWrite(greenPin_, offLevel());
		blueOn_ = true;	// force l'ecriture au premier appel
		setBlue(false);
		started_ = true;
	}

	// active = false (pas de telemetrie, ecran en veille...) => LED eteinte.
	void update(int rpm, int alertMinRpm, bool active, uint32_t now)
	{
		if (!started_)
			return;
#if DASHBOARD_SHIFT_LED_DEBUG
		trace(rpm, alertMinRpm, active, now);
#endif

		if (!active || alertMinRpm <= 0)
		{
			reset();
			return;
		}

		updateDirection(rpm, now);

		const float start = alertMinRpm * APPROACH_START_RATIO;
		const bool inApproach = rpm >= start && rpm < alertMinRpm;
		if (!inApproach || !rising_)
		{
			blinking_ = false;
			setBlue(false);
			return;
		}

		// 0 en debut de zone, 1 au seuil: la demi-periode raccourcit avec l'approche.
		const float progress = constrain((rpm - start) / (alertMinRpm - start), 0.0f, 1.0f);
		const uint32_t halfPeriod = BLINK_HALF_PERIOD_SLOW_MS -
			static_cast<uint32_t>(progress *
				(BLINK_HALF_PERIOD_SLOW_MS - BLINK_HALF_PERIOD_FAST_MS));

		if (!blinking_)
		{
			// Entree dans la zone: allumage immediat.
			blinking_ = true;
			lastToggle_ = now;
			setBlue(true);
		}
		else if (now - lastToggle_ >= halfPeriod)
		{
			lastToggle_ = now;
			setBlue(!blueOn_);
		}
	}

private:
#if DASHBOARD_SHIFT_LED_DEBUG
	void trace(int rpm, int alertMinRpm, bool active, uint32_t now)
	{
		if (now - lastTrace_ < 250)
			return;
		lastTrace_ = now;
		Serial.printf("[LED] actif=%d rpm=%d seuil=%d debut=%d monte=%d bleu=%d\n",
			active ? 1 : 0, rpm, alertMinRpm,
			static_cast<int>(alertMinRpm * APPROACH_START_RATIO),
			rising_ ? 1 : 0, blueOn_ ? 1 : 0);
	}
	uint32_t lastTrace_ = 0;
#endif

	void updateDirection(int rpm, uint32_t now)
	{
		if (!hasReference_)
		{
			hasReference_ = true;
			referenceRpm_ = rpm;
			referenceTime_ = now;
			return;
		}
		if (now - referenceTime_ < DIRECTION_WINDOW_MS)
			return;

		const int delta = rpm - referenceRpm_;
		if (delta > DIRECTION_HYSTERESIS_RPM)
			rising_ = true;
		else if (delta < -DIRECTION_HYSTERESIS_RPM)
			rising_ = false;
		referenceRpm_ = rpm;
		referenceTime_ = now;
	}

	void reset()
	{
		hasReference_ = false;
		rising_ = false;
		blinking_ = false;
		setBlue(false);
	}

	uint8_t offLevel() const { return activeLow_ ? HIGH : LOW; }
	uint8_t onLevel() const { return activeLow_ ? LOW : HIGH; }

	void setBlue(bool on)
	{
		if (on == blueOn_)
			return;
		blueOn_ = on;
		digitalWrite(bluePin_, on ? onLevel() : offLevel());
	}

	int redPin_ = -1;
	int greenPin_ = -1;
	int bluePin_ = -1;
	bool activeLow_ = true;
	bool started_ = false;
	bool blueOn_ = false;
	bool blinking_ = false;
	bool rising_ = false;
	bool hasReference_ = false;
	int referenceRpm_ = 0;
	uint32_t referenceTime_ = 0;
	uint32_t lastToggle_ = 0;
};
