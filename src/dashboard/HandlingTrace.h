// 21/10/2026 10:51 - Trace du comportement (sous/survirage, ecarts de vitesse des roues) pour aider au reglage du DGL : 3 lignes pour le HUD + trace serie [HDL]
#pragma once

#include <Arduino.h>
#include <math.h>

// Trace serie [HDL] (toutes les donnees brutes) a HANDLING_SERIAL_PERIOD_MS.
#ifndef DASHBOARD_HANDLING_DEBUG
#define DASHBOARD_HANDLING_DEBUG 0
#endif

// Premiere etape de l'analyse de comportement : afficher les signaux bruts pour
// valider en jeu leur axe, leur signe et leur unite avant de construire un vrai
// indicateur.
//
//  Ligne 1 : "Y" lacet mesure (angularVelocity[HANDLING_YAW_AXIS], rad/s)
//            "N" lacet d'une voiture neutre = v x tan(braquage) / empattement (rad/s)
//            -> |Y| < |N| : sous-virage ; |Y| > |N| ou signes opposes : survirage
//  Ligne 2 : "S" braquage moyen des roues avant (degres), "L" empattement (m)
//  Ligne 3 : ecart de vitesse gauche - droite des roues (km/h), "F" avant, "R" arriere
//            (en virage, la roue exterieure tourne naturellement plus vite)
//
// Les lignes sont figees HANDLING_DISPLAY_PERIOD_MS pour rester lisibles en roulant.
class HandlingTrace
{
public:
	static constexpr int HANDLING_YAW_AXIS = 1;	// GT7: axe Y vertical (a confirmer en jeu)
	static constexpr uint32_t HANDLING_DISPLAY_PERIOD_MS = 200;
	static constexpr uint32_t HANDLING_SERIAL_PERIOD_MS = 100;
	static constexpr float HANDLING_MIN_SPEED_MS = 2.0f;	// en dessous: lacet neutre non calcule

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

	String line1 = "Y --   N --";
	String line2 = "S --   L --";
	String line3 = "F --   R --";

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

		if (now - lastDisplay_ >= HANDLING_DISPLAY_PERIOD_MS)
		{
			lastDisplay_ = now;
			char buffer[24];
			char neutral[8];
			if (isfinite(neutralYaw))
				snprintf(neutral, sizeof(neutral), "%+.2f", neutralYaw);
			else
				snprintf(neutral, sizeof(neutral), "--");
			snprintf(buffer, sizeof(buffer), "Y%+.2f N%s", yaw, neutral);
			line1 = buffer;
			snprintf(buffer, sizeof(buffer), "S%+.1f L%.2f", steer * RAD_TO_DEG, in.wheelBase);
			line2 = buffer;
			snprintf(buffer, sizeof(buffer), "F%+.1f R%+.1f", frontDiff, rearDiff);
			line3 = buffer;
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
	uint32_t lastDisplay_ = 0;
	uint32_t lastSerial_ = 0;
};
