// 23/10/2026 00:06 - Affichage par phase de virage (entree / milieu / sortie) : equilibre Y/N et blocage du differentiel arriere R/F, moyennes glissantes ; trace brute conservee sur le port serie
// 21/10/2026 10:51 - Trace du comportement (sous/survirage, ecarts de vitesse des roues) pour aider au reglage du DGL : 3 lignes pour le HUD + trace serie [HDL]
#pragma once

#include <Arduino.h>
#include <math.h>

// Trace serie [HDL] (toutes les donnees brutes) a HANDLING_SERIAL_PERIOD_MS.
#ifndef DASHBOARD_HANDLING_DEBUG
#define DASHBOARD_HANDLING_DEBUG 0
#endif

// Analyse du comportement en virage pour aider au reglage du DGL.
//
// Signaux (valides en jeu le 22/10/2026) :
//  - Y : lacet mesure, angularVelocity[HANDLING_YAW_AXIS] (rad/s, positif a gauche)
//  - N : lacet d'une voiture neutre = v x tan(braquage) / empattement (meme signe que Y)
//  - F, R : ecart de vitesse gauche - droite des roues avant / arriere (km/h)
//
// Indicateurs, calcules uniquement en virage :
//  - Y/N : equilibre. < HANDLING_UNDERSTEER_RATIO : sous-virage,
//          > HANDLING_OVERSTEER_RATIO (ou negatif = contre-braquage) : survirage.
//  - R/F : blocage du differentiel arriere, valable pour une PROPULSION (l'avant,
//          non moteur, roule librement et donne l'ecart naturel du virage).
//          1 = arriere libre, 0 = arriere bloque, negatif = patinage de la roue
//          interieure.
//
// Chaque indicateur est moyenne par phase, selon les pedales :
//  - E (entree)  : frein, ou lever de pied        -> reglage DGL "Decel"
//  - M (milieu)  : accelerateur partiel, sans frein -> reglage DGL "Initial"
//  - S (sortie)  : accelerateur franc, sans frein   -> reglage DGL "Accel"
// La moyenne est glissante (exponentielle) sur environ HANDLING_AVERAGE_TIME_S
// secondes de virage cumule dans la phase : elle suit un changement de reglage
// sans attendre la fin du tour.
//
// Affichage : une ligne par phase, "E 0.92 +0.10" (phase, Y/N, R/F), figee
// HANDLING_DISPLAY_PERIOD_MS pour rester lisible ; "--" tant qu'il n'y a pas
// assez de donnees.
class HandlingTrace
{
public:
	static constexpr int HANDLING_YAW_AXIS = 1;	// GT7: axe Y vertical (confirme en jeu)
	static constexpr uint32_t HANDLING_DISPLAY_PERIOD_MS = 200;
	static constexpr uint32_t HANDLING_SERIAL_PERIOD_MS = 100;
	static constexpr float HANDLING_MIN_SPEED_MS = 2.0f;	// en dessous: lacet neutre non calcule

	// Detection du virage et des phases.
	static constexpr float HANDLING_MIN_NEUTRAL_YAW = 0.1f;	// rad/s: en dessous, ligne droite
	static constexpr float HANDLING_MIN_FRONT_DIFF_KMH = 0.5f;	// en dessous, R/F non calcule
	static constexpr int HANDLING_BRAKE_MIN_PERCENT = 5;
	static constexpr int HANDLING_LIFT_MAX_THROTTLE_PERCENT = 10;
	static constexpr int HANDLING_EXIT_MIN_THROTTLE_PERCENT = 50;

	// Moyennes et seuils d'interpretation.
	static constexpr float HANDLING_AVERAGE_TIME_S = 10.0f;
	static constexpr float HANDLING_MIN_SAMPLE_TIME_S = 0.5f;	// avant: "--"
	static constexpr uint32_t HANDLING_MAX_STEP_MS = 100;	// trou de telemetrie ignore
	static constexpr float HANDLING_UNDERSTEER_RATIO = 0.9f;
	static constexpr float HANDLING_OVERSTEER_RATIO = 1.1f;

	enum Phase : uint8_t { PHASE_ENTRY, PHASE_MID, PHASE_EXIT, PHASE_COUNT };
	enum Tendency : int8_t { TENDENCY_NONE = -2, TENDENCY_UNDER = -1, TENDENCY_NEUTRAL = 0, TENDENCY_OVER = 1 };

	struct Input
	{
		float speedMs;
		const float *angularVelocity;	// [3] rad/s
		const float *wheelSteeringAngle;	// [2] rad, gauche puis droite
		float wheelBase;	// m
		const float *wheelRps;	// [4] rad/s, AVG AVD ARG ARD
		const float *tyreRadius;	// [4] m
		int throttlePercent;
		int brakePercent;
	};

	String lines[PHASE_COUNT] = {"E --   --", "M --   --", "S --   --"};
	Tendency tendency[PHASE_COUNT] = {TENDENCY_NONE, TENDENCY_NONE, TENDENCY_NONE};

	void update(const Input &in, uint32_t now)
	{
		const float yaw = in.angularVelocity[HANDLING_YAW_AXIS];
		const float steer = 0.5f * (in.wheelSteeringAngle[0] + in.wheelSteeringAngle[1]);
		const bool neutralValid = in.wheelBase > 0.5f && in.speedMs >= HANDLING_MIN_SPEED_MS;
		const float neutralYaw = neutralValid
			? in.speedMs * tanf(steer) / in.wheelBase
			: NAN;

		float wheelKmh[4];
		for (int i = 0; i < 4; ++i)
			wheelKmh[i] = fabsf(in.wheelRps[i] * in.tyreRadius[i]) * 3.6f;
		const float frontDiff = wheelKmh[0] - wheelKmh[1];
		const float rearDiff = wheelKmh[2] - wheelKmh[3];

		accumulate(in, yaw, neutralYaw, frontDiff, rearDiff, now);

		if (now - lastDisplay_ >= HANDLING_DISPLAY_PERIOD_MS)
		{
			lastDisplay_ = now;
			refreshLines();
		}

#if DASHBOARD_HANDLING_DEBUG
		if (now - lastSerial_ >= HANDLING_SERIAL_PERIOD_MS)
		{
			lastSerial_ = now;
			Serial.printf("[HDL] v=%.1f acc=%d frein=%d brq=%+.3f/%+.3f L=%.3f "
				"av=%+.3f/%+.3f/%+.3f lacetN=%+.3f roues=%.1f/%.1f/%.1f/%.1f\n",
				in.speedMs * 3.6f, in.throttlePercent, in.brakePercent,
				in.wheelSteeringAngle[0], in.wheelSteeringAngle[1], in.wheelBase,
				in.angularVelocity[0], in.angularVelocity[1], in.angularVelocity[2],
				neutralYaw, wheelKmh[0], wheelKmh[1], wheelKmh[2], wheelKmh[3]);
		}
#endif
	}

private:
	struct Average
	{
		float value = 0.0f;
		float weightS = 0.0f;	// temps de virage cumule (plafonne), 0 = aucune donnee

		void add(float sample, float dtS)
		{
			weightS = min(weightS + dtS, HANDLING_AVERAGE_TIME_S);
			// Au debut, la moyenne est arithmetique ; ensuite exponentielle de
			// constante de temps HANDLING_AVERAGE_TIME_S.
			value += (sample - value) * (dtS / weightS);
		}
		bool valid() const { return weightS >= HANDLING_MIN_SAMPLE_TIME_S; }
	};

	static Phase phaseOf(const Input &in)
	{
		if (in.brakePercent >= HANDLING_BRAKE_MIN_PERCENT ||
			in.throttlePercent <= HANDLING_LIFT_MAX_THROTTLE_PERCENT)
			return PHASE_ENTRY;
		if (in.throttlePercent >= HANDLING_EXIT_MIN_THROTTLE_PERCENT)
			return PHASE_EXIT;
		return PHASE_MID;
	}

	void accumulate(const Input &in, float yaw, float neutralYaw,
		float frontDiff, float rearDiff, uint32_t now)
	{
		const uint32_t stepMs = lastSample_ == 0 ? 0 : now - lastSample_;
		lastSample_ = now;
		if (stepMs == 0 || stepMs > HANDLING_MAX_STEP_MS)
			return;
		if (!isfinite(neutralYaw) || fabsf(neutralYaw) < HANDLING_MIN_NEUTRAL_YAW)
			return;	// ligne droite ou trop lent

		const float dtS = stepMs / 1000.0f;
		const Phase phase = phaseOf(in);
		// Ratio borne pour qu'un echantillon aberrant (choc, vibreur) ne domine pas.
		balance_[phase].add(constrain(yaw / neutralYaw, -2.0f, 3.0f), dtS);
		if (fabsf(frontDiff) >= HANDLING_MIN_FRONT_DIFF_KMH)
			lock_[phase].add(constrain(rearDiff / frontDiff, -3.0f, 3.0f), dtS);
	}

	void refreshLines()
	{
		static const char phaseNames[PHASE_COUNT] = {'E', 'M', 'S'};
		for (int p = 0; p < PHASE_COUNT; ++p)
		{
			char balance[8];
			char lock[8];
			if (balance_[p].valid())
				snprintf(balance, sizeof(balance), "%.2f", balance_[p].value);
			else
				snprintf(balance, sizeof(balance), "--  ");
			if (lock_[p].valid())
				snprintf(lock, sizeof(lock), "%+.2f", lock_[p].value);
			else
				snprintf(lock, sizeof(lock), "--");
			char buffer[24];
			snprintf(buffer, sizeof(buffer), "%c %s %s", phaseNames[p], balance, lock);
			lines[p] = buffer;

			if (!balance_[p].valid())
				tendency[p] = TENDENCY_NONE;
			else if (balance_[p].value < 0.0f || balance_[p].value > HANDLING_OVERSTEER_RATIO)
				tendency[p] = TENDENCY_OVER;
			else if (balance_[p].value < HANDLING_UNDERSTEER_RATIO)
				tendency[p] = TENDENCY_UNDER;
			else
				tendency[p] = TENDENCY_NEUTRAL;
		}
	}

	Average balance_[PHASE_COUNT];
	Average lock_[PHASE_COUNT];
	uint32_t lastSample_ = 0;
	uint32_t lastDisplay_ = 0;
	uint32_t lastSerial_ = 0;
};
