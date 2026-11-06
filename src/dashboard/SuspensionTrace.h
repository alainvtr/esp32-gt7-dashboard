// 06/11/2026 01:45 - Creation : releves de suspension (suspHeight par roue et bodyHeight) par situation (arret, freinage, acceleration, virage gauche / droite) avec minimum et maximum, pour apprendre les unites avant d'ecrire des conseils
#pragma once

#include <Arduino.h>
#include <math.h>
#include "tuning.h"
#include "HandlingTrace.h"

// Releves de suspension, sans conseil pour l'instant : le jeu envoie suspHeight[4]
// (AVG, AVD, ARG, ARD) et bodyHeight, mais leur unite et leur sens (hauteur ou
// course, augmente-t-elle en compression ?) restent a confirmer en jeu. Ces releves
// remplacent une trace serie (PC trop loin de la console).
//
// Situations, d'apres la vitesse, les pedales et le lacet (positif a gauche) :
//  - ARRET : vitesse <= ENGINEER_STOPPED_KMH (reference statique)
//  - FREIN : frein >= HANDLING_BRAKE_MIN_PERCENT
//  - ACCEL : gaz >= HANDLING_EXIT_MIN_THROTTLE_PERCENT, en ligne droite
//  - VIR G / VIR D : virage franc sans frein
// En roulant, seulement au-dessus de SUSPENSION_MIN_KMH. Minimum et maximum : en
// roulant (au-dessus de ENGINEER_CLOSE_KMH).
class SuspensionTrace
{
public:
	static constexpr int CHANNEL_COUNT = 5;	// 4 roues + caisse
	static constexpr int BODY = 4;
	static constexpr uint32_t MAX_STEP_MS = 100;
	enum Condition : uint8_t { COND_STAND, COND_BRAKE, COND_ACCEL, COND_LEFT, COND_RIGHT, COND_COUNT };

	struct Channel
	{
		HandlingTrace::SessionStat stat[COND_COUNT];
		float minValue = NAN;
		float maxValue = NAN;
	};

	void update(float speedKmh, int throttlePercent, int brakePercent, float yaw,
		const float *suspHeight, float bodyHeight, bool paused, uint32_t now)
	{
		const uint32_t stepMs = lastSample_ == 0 ? 0 : now - lastSample_;
		lastSample_ = now;
		if (paused || stepMs == 0 || stepMs > MAX_STEP_MS) return;
		const float dtS = stepMs / 1000.0f;
		const float values[CHANNEL_COUNT] = {suspHeight[0], suspHeight[1], suspHeight[2],
			suspHeight[3], bodyHeight};

		int condition = -1;
		if (speedKmh <= ENGINEER_STOPPED_KMH)
			condition = COND_STAND;
		else if (speedKmh >= SUSPENSION_MIN_KMH)
		{
			if (brakePercent >= HANDLING_BRAKE_MIN_PERCENT)
				condition = COND_BRAKE;
			else if (fabsf(yaw) >= SUSPENSION_CORNER_YAW)
				condition = yaw > 0.0f ? COND_LEFT : COND_RIGHT;
			else if (throttlePercent >= HANDLING_EXIT_MIN_THROTTLE_PERCENT &&
				fabsf(yaw) <= SUSPENSION_STRAIGHT_YAW)
				condition = COND_ACCEL;
		}

		for (int c = 0; c < CHANNEL_COUNT; ++c)
		{
			if (!isfinite(values[c])) continue;
			Channel &channel = channels_[c];
			if (condition >= 0)
				channel.stat[condition].add(values[c], dtS);
			if (speedKmh > ENGINEER_CLOSE_KMH)
			{
				if (!isfinite(channel.minValue) || values[c] < channel.minValue) channel.minValue = values[c];
				if (!isfinite(channel.maxValue) || values[c] > channel.maxValue) channel.maxValue = values[c];
			}
		}
	}

	void reset()
	{
		for (int c = 0; c < CHANNEL_COUNT; ++c)
			channels_[c] = Channel();
	}

	const Channel &channel(int index) const { return channels_[index]; }
	float conditionTimeS(int condition) const { return channels_[0].stat[condition].timeS; }

private:
	Channel channels_[CHANNEL_COUNT];
	uint32_t lastSample_ = 0;
};
