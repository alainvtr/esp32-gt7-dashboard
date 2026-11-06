// 06/11/2026 02:05 - Etagement : chute de regime a chaque passage (rapports du jeu), regime de passage mesure, point le plus lent du tour ; conseils d'apres les regimes de couple et de puissance max saisis pour la voiture
// 06/11/2026 01:45 - Creation : releves de la boite par tour (vitesse max, regime max et temps au regime de passage sur le dernier rapport) et conseil sur le reglage "vitesse max" de la transmission
#pragma once

#include <Arduino.h>
#include <math.h>
#include "tuning.h"

// Boite de vitesses : le dernier rapport est-il adapte au circuit ?
//
// Par tour complet (le 1er tour de la session, partiel, et les tours avec pit sont
// ignores), sur le dernier rapport :
//  - temps passe au regime de passage de rapport du jeu (minAlertRPM) ou au-dela :
//    la voiture voudrait un rapport de plus -> boite trop courte ;
//  - regime maximal atteint, en fraction de ce regime : loin en dessous, la vitesse
//    max reglee n'est jamais atteinte -> boite trop longue (on peut la raccourcir
//    pour mieux accelerer) ;
//  - vitesse max du tour, comparee a calcMaxSpeed (vitesse max permise par les
//    rapports, envoyee par le jeu), pour information.
// Le conseil porte sur le curseur "vitesse max" de la transmission (boite
// entierement personnalisable), en % : il ne depend pas de l'unite de calcMaxSpeed.
//
// Etagement, si les regimes de couple max et de puissance max de la voiture sont
// saisis (la plage utile du moteur va de l'un a l'autre) :
//  - chute de regime a chaque passage : regime apres = regime au passage x rapport
//    suivant / rapport actuel (rapports envoyes par le jeu). En passant au regime
//    de passage du jeu, retomber sous le couple max = ecart trop grand entre les
//    deux rapports ;
//  - regime reel des passages montants (gaz >= ENGINEER_GEAR_SHIFT_MIN_THROTTLE) :
//    avant la puissance max = passages trop tot (pilotage, pas reglage) ;
//  - point le plus lent de chaque tour (virage le plus lent) : sous le couple max,
//    le rapport utilise est trop long pour ce virage.
// Ordre des conseils : vitesse max (decale tous les rapports), etagement, virage
// lent, passages trop tot.
//
// Une session va d'un retour en piste (la boite ne se regle qu'au garage) ou d'un
// appui sur RAZ au suivant ; son bilan devient le bilan "avant" (meme voiture).
class GearboxTrace
{
public:
	static constexpr int MAX_GEARS = 8;
	static constexpr uint32_t MAX_STEP_MS = 100;	// trou de telemetrie ignore

	enum AdviceKind : uint8_t { ADVICE_INFO, ADVICE_SHORT, ADVICE_LONG, ADVICE_OK,
		ADVICE_SPACING, ADVICE_CORNER, ADVICE_DRIVER };

	// Regimes de la plage utile du moteur, saisis par voiture (0 = inconnu).
	struct EngineBand
	{
		int torqueRpm = 0;
		int powerRpm = 0;
		bool known() const { return torqueRpm > 0 && powerRpm > 0; }
	};

	struct Summary
	{
		int laps = 0;	// tours complets mesures
		int topGear = 0;
		int shiftRpm = 0;
		int calcMaxSpeed = 0;	// valeur du jeu (km/h a confirmer)
		float maxSpeedKmh = NAN;	// moyenne des vitesses max des tours
		float topGearS = NAN;	// temps sur le dernier rapport, par tour
		float limiterS = NAN;	// temps au regime de passage sur le dernier rapport, par tour
		float peakRpmRatio = NAN;	// regime max sur le dernier rapport / regime de passage
		float ratios[MAX_GEARS] = {};	// rapports envoyes par le jeu (0 = absent)
		// Passages montants g -> g+1, indice g-1 : nombre et regime moyen au passage.
		int shiftCount[MAX_GEARS] = {};
		float shiftUpRpm[MAX_GEARS] = {};
		// Point le plus lent des tours : regime moyen, vitesse moyenne, rapport (dernier tour).
		float cornerRpm = NAN;
		float cornerSpeedKmh = NAN;
		int cornerGear = 0;

		bool valid() const { return laps >= ENGINEER_GEAR_MIN_LAPS; }

		// Regime apres le passage g -> g+1 en passant a fromRpm (NAN si rapports absents).
		float rpmAfterShift(int gear, float fromRpm) const
		{
			if (gear < 1 || gear >= MAX_GEARS || ratios[gear - 1] <= 0.0f || ratios[gear] <= 0.0f)
				return NAN;
			return fromRpm * ratios[gear] / ratios[gear - 1];
		}

		// Regime de passage a retenir pour g -> g+1 : celui mesure, sinon celui du jeu.
		float usedShiftRpm(int gear) const
		{
			return shiftCount[gear - 1] > 0 ? shiftUpRpm[gear - 1] : static_cast<float>(shiftRpm);
		}
	};

	struct Advice
	{
		AdviceKind kind = ADVICE_INFO;
		char diagnosis[64] = "";
		char action[64] = "";
		char note[64] = "";
	};

	void update(float speedKmh, float rpm, int gear, int throttlePercent, const float *gearRatios,
		int shiftRpm, int calcMaxSpeed, bool paused, uint32_t now)
	{
		const uint32_t stepMs = lastSample_ == 0 ? 0 : now - lastSample_;
		lastSample_ = now;
		int ratioCount = 0;
		for (int i = 0; i < MAX_GEARS; ++i)
			if (gearRatios[i] > 0.0f) ratioCount = i + 1;
		if (ratioCount > 0)
		{
			topGear_ = ratioCount;
			for (int i = 0; i < MAX_GEARS; ++i) ratios_[i] = gearRatios[i];
		}
		if (shiftRpm > 0) shiftRpm_ = shiftRpm;
		if (calcMaxSpeed > 0) calcMaxSpeed_ = calcMaxSpeed;
		const int previousGear = lastGear_;
		const float previousRpm = lastRpm_;
		const int previousThrottle = lastThrottle_;
		lastGear_ = gear;
		lastRpm_ = rpm;
		lastThrottle_ = throttlePercent;
		if (paused || stepMs == 0 || stepMs > MAX_STEP_MS) return;

		// Passage montant g -> g+1 sous les gaz : regime juste avant le passage.
		if (gear == previousGear + 1 && previousGear >= 1 && previousGear < MAX_GEARS &&
			previousThrottle >= ENGINEER_GEAR_SHIFT_MIN_THROTTLE && previousRpm > 0.0f)
		{
			shiftCount_[previousGear - 1]++;
			shiftRpmSum_[previousGear - 1] += previousRpm;
		}

		const float dtS = stepMs / 1000.0f;
		lap_.maxSpeedKmh = max(lap_.maxSpeedKmh, speedKmh);
		// Point le plus lent du tour (virage le plus lent), voiture en mouvement.
		if (speedKmh >= ENGINEER_GEAR_CORNER_MIN_KMH && gear >= 1 && gear <= MAX_GEARS &&
			(lap_.cornerGear == 0 || speedKmh < lap_.cornerSpeedKmh))
		{
			lap_.cornerSpeedKmh = speedKmh;
			lap_.cornerRpm = rpm;
			lap_.cornerGear = gear;
		}
		if (topGear_ > 0 && gear == topGear_)
		{
			lap_.topGearS += dtS;
			lap_.peakRpm = max(lap_.peakRpm, rpm);
			if (shiftRpm_ > 0 && rpm >= shiftRpm_ * ENGINEER_GEAR_LIMIT_RATIO)
				lap_.limiterS += dtS;
		}
	}

	// Fin de tour : retenu s'il a commence dans la session et sans pit.
	void endLap(bool pit)
	{
		if (lapValid_ && !pit && lap_.maxSpeedKmh > 0.0f)
		{
			++total_.laps;
			total_.maxSpeedKmh += lap_.maxSpeedKmh;
			total_.topGearS += lap_.topGearS;
			total_.limiterS += lap_.limiterS;
			total_.peakRpm += lap_.peakRpm;
			if (lap_.cornerGear > 0)
			{
				++total_.corners;
				total_.cornerRpm += lap_.cornerRpm;
				total_.cornerSpeedKmh += lap_.cornerSpeedKmh;
				total_.cornerGear = lap_.cornerGear;
			}
		}
		lap_ = LapStats();
		lapValid_ = true;
	}

	// Tour en cours non comptable (nouvelle course : le compteur de tours recule).
	void discardLap()
	{
		lap_ = LapStats();
		lapValid_ = false;
	}

	// Nouvelle session : le bilan de la session ecoulee devient "avant".
	void endSession(int32_t newCarCode)
	{
		const Summary finished = summary();
		if (finished.valid())
		{
			before_ = finished;
			// Session ouverte sans retour en piste (mode demo) : voiture inconnue, on
			// prend celle en piste.
			beforeCar_ = sessionCar_ != 0 ? sessionCar_ : newCarCode;
			hasBefore_ = true;
		}
		total_ = Totals();
		for (int i = 0; i < MAX_GEARS; ++i)
		{
			shiftCount_[i] = 0;
			shiftRpmSum_[i] = 0.0f;
		}
		discardLap();
		sessionCar_ = newCarCode;
	}

	Summary summary() const
	{
		Summary s;
		s.laps = total_.laps;
		s.topGear = topGear_;
		s.shiftRpm = shiftRpm_;
		s.calcMaxSpeed = calcMaxSpeed_;
		if (total_.laps > 0)
		{
			s.maxSpeedKmh = total_.maxSpeedKmh / total_.laps;
			s.topGearS = total_.topGearS / total_.laps;
			s.limiterS = total_.limiterS / total_.laps;
			if (shiftRpm_ > 0)
				s.peakRpmRatio = total_.peakRpm / total_.laps / shiftRpm_;
		}
		for (int i = 0; i < MAX_GEARS; ++i)
		{
			s.ratios[i] = ratios_[i];
			s.shiftCount[i] = shiftCount_[i];
			s.shiftUpRpm[i] = shiftCount_[i] > 0 ? shiftRpmSum_[i] / shiftCount_[i] : NAN;
		}
		if (total_.corners > 0)
		{
			s.cornerRpm = total_.cornerRpm / total_.corners;
			s.cornerSpeedKmh = total_.cornerSpeedKmh / total_.corners;
			s.cornerGear = total_.cornerGear;
		}
		return s;
	}

	// Bilan "avant", seulement pour la meme voiture.
	bool before(int32_t carCode, Summary &out) const
	{
		if (!hasBefore_ || beforeCar_ != carCode) return false;
		out = before_;
		return true;
	}

	// "1re", "2e"...
	static const char *gearName(int gear, char *text, size_t size)
	{
		snprintf(text, size, gear == 1 ? "1re" : "%de", gear);
		return text;
	}

	// Passage g -> g+1 qui retombe le plus sous le couple max, au regime de passage
	// du jeu (0 = aucun). Seuls les rapports utilises comptent, si des passages ont
	// ete mesures.
	static int worstSpacing(const Summary &s, const EngineBand &engine, float &rpmAfter)
	{
		bool anyMeasured = false;
		for (int g = 1; g < s.topGear; ++g)
			if (s.shiftCount[g - 1] > 0) anyMeasured = true;
		int worst = 0;
		float worstDeficit = 0.0f;
		for (int g = 1; g < s.topGear; ++g)
		{
			if (anyMeasured && s.shiftCount[g - 1] == 0) continue;
			const float after = s.rpmAfterShift(g, static_cast<float>(s.shiftRpm));
			if (!isfinite(after)) continue;
			const float deficit = engine.torqueRpm - after;
			if (deficit > worstDeficit)
			{
				worstDeficit = deficit;
				worst = g;
				rpmAfter = after;
			}
		}
		return worst;
	}

	static Advice analyze(const Summary &s, const EngineBand &engine)
	{
		Advice a;
		if (!s.valid())
		{
			snprintf(a.diagnosis, sizeof(a.diagnosis), "Pas assez de tours : %d / %d",
				s.laps, ENGINEER_GEAR_MIN_LAPS);
			snprintf(a.action, sizeof(a.action), "Rouler encore (tours complets, sans pit)");
			snprintf(a.note, sizeof(a.note), "Le 1er tour de la session ne compte pas");
			return a;
		}
		if (s.topGear <= 0 || s.shiftRpm <= 0)
		{
			snprintf(a.diagnosis, sizeof(a.diagnosis), "Rapports ou regime de passage inconnus");
			return a;
		}
		snprintf(a.note, sizeof(a.note), "Transmission > vitesse max (boite personnalisable)");
		if (s.limiterS >= ENGINEER_GEAR_SHORT_S)
		{
			a.kind = ADVICE_SHORT;
			snprintf(a.diagnosis, sizeof(a.diagnosis), "Trop courte : %.1f s/tour au passage en %de",
				s.limiterS, s.topGear);
			actionPercent(a, s, ENGINEER_GEAR_STEP_PERCENT);
			return a;
		}
		if (s.topGearS < ENGINEER_GEAR_UNUSED_S)
		{
			a.kind = ADVICE_LONG;
			snprintf(a.diagnosis, sizeof(a.diagnosis), "Trop longue : %de presque jamais utilisee",
				s.topGear);
			actionPercent(a, s, -ENGINEER_GEAR_STEP_PERCENT);
			return a;
		}
		if (s.peakRpmRatio < ENGINEER_GEAR_LONG_RATIO)
		{
			a.kind = ADVICE_LONG;
			snprintf(a.diagnosis, sizeof(a.diagnosis), "Trop longue : %de a %d %% du passage au max",
				s.topGear, static_cast<int>(s.peakRpmRatio * 100.0f + 0.5f));
			// A vitesse egale, le regime suit le rapport : raccourcir de (1 - ratio)
			// amene le regime de passage a la vitesse max atteinte.
			actionPercent(a, s, -static_cast<int>((1.0f - s.peakRpmRatio) * 100.0f + 0.5f));
			return a;
		}
		if (!engine.known())
		{
			a.kind = ADVICE_OK;
			snprintf(a.diagnosis, sizeof(a.diagnosis), "Vitesse max adaptee au circuit");
			snprintf(a.action, sizeof(a.action), "Etagement : saisir les regimes (MOTEUR)");
			snprintf(a.note, sizeof(a.note), "Couple max et puissance max, fiche de la voiture");
			return a;
		}
		char gearA[8];
		char gearB[8];

		// Etagement : ecart trop grand entre deux rapports.
		float after = NAN;
		const int spacing = worstSpacing(s, engine, after);
		if (spacing > 0)
		{
			a.kind = ADVICE_SPACING;
			snprintf(a.diagnosis, sizeof(a.diagnosis), "%s > %s : retombe a %d, sous le couple max",
				gearName(spacing, gearA, sizeof(gearA)), gearName(spacing + 1, gearB, sizeof(gearB)),
				static_cast<int>(after + 0.5f));
			snprintf(a.action, sizeof(a.action), "Rapprocher : %s plus courte (ou %s plus longue)",
				gearB, gearA);
			snprintf(a.note, sizeof(a.note), "Passage a %d, couple max %d tr/min",
				s.shiftRpm, engine.torqueRpm);
			return a;
		}

		// Virage le plus lent : sous le couple max, le rapport est trop long.
		if (s.cornerGear > 0 && s.cornerRpm < engine.torqueRpm)
		{
			const int g = s.cornerGear;
			a.kind = ADVICE_CORNER;
			snprintf(a.diagnosis, sizeof(a.diagnosis), "Virage lent (%d km/h) en %s a %d tr/min",
				static_cast<int>(s.cornerSpeedKmh + 0.5f), gearName(g, gearA, sizeof(gearA)),
				static_cast<int>(s.cornerRpm + 0.5f));
			// Un rapport plus bas, si le regime y reste sous le passage.
			const float lower = g >= 2 && s.ratios[g - 2] > 0.0f && s.ratios[g - 1] > 0.0f
				? s.cornerRpm * s.ratios[g - 2] / s.ratios[g - 1]
				: NAN;
			if (isfinite(lower) && lower <= s.shiftRpm)
				snprintf(a.action, sizeof(a.action), "Retrograder en %s (%d tr/min) ou %s plus courte",
					gearName(g - 1, gearB, sizeof(gearB)), static_cast<int>(lower + 0.5f), gearA);
			else
				snprintf(a.action, sizeof(a.action), "%s plus courte", gearA);
			snprintf(a.note, sizeof(a.note), "Sous le couple max (%d tr/min)", engine.torqueRpm);
			return a;
		}

		// Passages mesures avant la puissance max : pilotage.
		for (int g = 1; g < s.topGear; ++g)
		{
			if (s.shiftCount[g - 1] == 0 ||
				s.shiftUpRpm[g - 1] >= engine.powerRpm * ENGINEER_GEAR_EARLY_RATIO)
				continue;
			a.kind = ADVICE_DRIVER;
			snprintf(a.diagnosis, sizeof(a.diagnosis), "Passages %s > %s a %d tr/min",
				gearName(g, gearA, sizeof(gearA)), gearName(g + 1, gearB, sizeof(gearB)),
				static_cast<int>(s.shiftUpRpm[g - 1] + 0.5f));
			snprintf(a.action, sizeof(a.action), "Passer plus tard : apres %d tr/min", engine.powerRpm);
			snprintf(a.note, sizeof(a.note), "Puissance max a %d tr/min (pilotage, pas reglage)",
				engine.powerRpm);
			return a;
		}

		a.kind = ADVICE_OK;
		snprintf(a.diagnosis, sizeof(a.diagnosis), "Boite adaptee au circuit");
		snprintf(a.action, sizeof(a.action), "Rien a changer");
		snprintf(a.note, sizeof(a.note), "Plage utile %d - %d tr/min respectee",
			engine.torqueRpm, engine.powerRpm);
		return a;
	}

private:
	struct LapStats
	{
		float maxSpeedKmh = 0.0f;
		float topGearS = 0.0f;
		float limiterS = 0.0f;
		float peakRpm = 0.0f;
		float cornerSpeedKmh = 0.0f;	// point le plus lent du tour
		float cornerRpm = 0.0f;
		int cornerGear = 0;	// 0 = aucun
	};

	struct Totals
	{
		int laps = 0;
		float maxSpeedKmh = 0.0f;
		float topGearS = 0.0f;
		float limiterS = 0.0f;
		float peakRpm = 0.0f;
		int corners = 0;
		float cornerRpm = 0.0f;
		float cornerSpeedKmh = 0.0f;
		int cornerGear = 0;
	};

	// "Vitesse max +5 % (env. 315)" : valeur estimee a partir de calcMaxSpeed.
	static void actionPercent(Advice &a, const Summary &s, int percent)
	{
		if (s.calcMaxSpeed > 0)
			snprintf(a.action, sizeof(a.action), "Vitesse max %+d %% (env. %d au lieu de %d)",
				percent, static_cast<int>(s.calcMaxSpeed * (100 + percent) / 100.0f + 0.5f),
				s.calcMaxSpeed);
		else
			snprintf(a.action, sizeof(a.action), "Vitesse max %+d %%", percent);
	}

	LapStats lap_;
	Totals total_;
	bool lapValid_ = false;	// le tour en cours a commence dans la session
	int topGear_ = 0;
	int shiftRpm_ = 0;
	int calcMaxSpeed_ = 0;
	float ratios_[MAX_GEARS] = {};
	int shiftCount_[MAX_GEARS] = {};
	float shiftRpmSum_[MAX_GEARS] = {};
	int lastGear_ = 0;
	float lastRpm_ = 0.0f;
	int lastThrottle_ = 0;
	uint32_t lastSample_ = 0;
	int32_t sessionCar_ = 0;
	Summary before_;
	int32_t beforeCar_ = 0;
	bool hasBefore_ = false;
};
