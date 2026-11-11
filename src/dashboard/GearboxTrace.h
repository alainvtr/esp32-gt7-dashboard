// 08/11/2026 20:45 - Tour valide a la fin du tour suivant (tours d'entree et de sortie de stand exclus) ; virage le plus lent : lacet net exige (voie des stands exclue), rapport le plus frequent ; analyse en 4 verdicts (vitesse max, etagement, virage lent, passages) et une action chiffree en valeurs de rapport
// 06/11/2026 02:05 - Etagement : chute de regime a chaque passage (rapports du jeu), regime de passage mesure, point le plus lent du tour ; conseils d'apres les regimes de couple et de puissance max saisis pour la voiture
// 06/11/2026 01:45 - Creation : releves de la boite par tour (vitesse max, regime max et temps au regime de passage sur le dernier rapport) et conseil sur le reglage "vitesse max" de la transmission
#pragma once

#include <Arduino.h>
#include <math.h>
#include <stdarg.h>
#include "tuning.h"

// Boite de vitesses : le dernier rapport est-il adapte au circuit ?
//
// Par tour complet (le 1er tour de la session, partiel, et les tours avec pit sont
// ignores ; un tour n'est valide qu'a la fin du tour suivant, quand on sait s'il
// contenait l'entree de la voie des stands), sur le dernier rapport :
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
//  - point le plus lent de chaque tour (virage le plus lent, lacet net exige pour
//    ne pas prendre la voie des stands) : sous le couple max, le rapport utilise
//    est trop long pour ce virage.
// Un verdict par sujet (vitesse max, etagement, virage lent, passages) et une
// seule action, sur le premier sujet a corriger dans cet ordre : vitesse max
// (decale tous les rapports), etagement, virage lent, passages trop tot. Les
// actions sur un rapport donnent la valeur a saisir (rapport plus grand = plus
// court), limitee pour ne pas faire retomber un passage voisin sous le couple max.
//
// Une session va d'un retour en piste (la boite ne se regle qu'au garage) ou d'un
// appui sur RAZ au suivant ; son bilan devient le bilan "avant" (meme voiture).
class GearboxTrace
{
public:
	static constexpr int MAX_GEARS = 8;
	static constexpr uint32_t MAX_STEP_MS = 100;	// trou de telemetrie ignore

	enum Level : uint8_t { LEVEL_UNKNOWN, LEVEL_OK, LEVEL_WARN };
	enum Topic : uint8_t { TOPIC_TOP_SPEED, TOPIC_SPACING, TOPIC_CORNER, TOPIC_SHIFTS, TOPIC_COUNT };

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
		// Point le plus lent des tours, sur le rapport le plus frequent : regime et vitesse moyens.
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

	struct Verdict
	{
		Level level = LEVEL_UNKNOWN;
		char text[56] = "";
	};

	struct Report
	{
		Verdict topic[TOPIC_COUNT];
		int actionTopic = -1;	// sujet de l'action, -1 = rien a changer
		char action[64] = "";
		char note[64] = "";
	};

	void update(float speedKmh, float rpm, int gear, int throttlePercent, float yaw,
		const float *gearRatios, int shiftRpm, int calcMaxSpeed, bool paused, uint32_t now)
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
		// Point le plus lent du tour (virage le plus lent), voiture en mouvement et en
		// virage net : la voie des stands, en ligne droite, ne compte pas.
		if (speedKmh >= ENGINEER_GEAR_CORNER_MIN_KMH && fabsf(yaw) >= ENGINEER_GEAR_CORNER_MIN_YAW &&
			gear >= 1 && gear <= MAX_GEARS &&
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

	// Fin du tour lapNumber : il attend la fin du tour suivant (un stand peut lui
	// etre attribue entre-temps) ; le tour precedent en attente est retenu s'il a
	// commence dans la session et sans stand.
	void endLap(int lapNumber, bool pit)
	{
		if (pendingValid_ && !pendingPit_)
			commit(pending_);
		pending_ = lap_;
		pendingLap_ = lapNumber;
		pendingPit_ = pit || lapPit_;
		pendingValid_ = lapValid_ && lap_.maxSpeedKmh > 0.0f;
		lap_ = LapStats();
		lapPit_ = false;
		lapValid_ = true;
		currentLap_ = lapNumber + 1;
	}

	// Stand confirme : le tour d'entree (en attente ou en cours) et le tour en cours
	// ne seront pas retenus.
	void markPit(int lapNumber)
	{
		if (pendingValid_ && lapNumber == pendingLap_)
			pendingPit_ = true;
		if (lapNumber >= currentLap_)
			lapPit_ = true;
	}

	// Tour en cours et tour en attente non comptables (nouvelle course : le compteur
	// de tours recule ; nouvelle session).
	void discardLap()
	{
		lap_ = LapStats();
		lapValid_ = false;
		lapPit_ = false;
		pendingValid_ = false;
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
		// Virage le plus lent : rapport le plus frequent (le plus bas en cas d'egalite).
		int cornerGear = 0;
		for (int g = 1; g <= MAX_GEARS; ++g)
			if (total_.cornerCount[g - 1] > (cornerGear > 0 ? total_.cornerCount[cornerGear - 1] : 0))
				cornerGear = g;
		if (cornerGear > 0)
		{
			const int count = total_.cornerCount[cornerGear - 1];
			s.cornerGear = cornerGear;
			s.cornerRpm = total_.cornerRpm[cornerGear - 1] / count;
			s.cornerSpeedKmh = total_.cornerSpeedKmh[cornerGear - 1] / count;
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

	static Report report(const Summary &s, const EngineBand &engine)
	{
		Report r;
		reportTopSpeed(s, r);
		reportSpacing(s, engine, r);
		reportCorner(s, engine, r);
		reportShifts(s, engine, r);
		if (r.actionTopic < 0)
		{
			snprintf(r.action, sizeof(r.action), s.valid() ? "Rien a changer" : "Rouler encore");
			snprintf(r.note, sizeof(r.note), s.valid() ? "" : "Tours complets sans stand ; le 1er ne compte pas");
		}
		return r;
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
		// Virage le plus lent, par rapport utilise (indice rapport - 1).
		int cornerCount[MAX_GEARS] = {};
		float cornerRpm[MAX_GEARS] = {};
		float cornerSpeedKmh[MAX_GEARS] = {};
	};

	// Premier sujet a corriger : il porte l'action (un seul reglage a la fois).
	static bool takeAction(Report &r, Topic topic)
	{
		if (r.actionTopic >= 0) return false;
		r.actionTopic = topic;
		return true;
	}

	static void setVerdict(Report &r, Topic topic, Level level, const char *format, ...)
	{
		r.topic[topic].level = level;
		va_list args;
		va_start(args, format);
		vsnprintf(r.topic[topic].text, sizeof(r.topic[topic].text), format, args);
		va_end(args);
	}

	// "Limite : 1re>2e reste au-dessus du couple" (passage lower -> lower+1).
	static void limitNote(Report &r, int lower)
	{
		char a[8];
		char b[8];
		snprintf(r.note, sizeof(r.note), "Limite : %s>%s reste au-dessus du couple",
			gearName(lower, a, sizeof(a)), gearName(lower + 1, b, sizeof(b)));
	}

	// "1re : 3.000 -> 3.240 (+8 %)" : rapport plus grand = plus court.
	static void ratioAction(Report &r, const Summary &s, int gear, float factor)
	{
		char name[8];
		const float ratio = s.ratios[gear - 1];
		snprintf(r.action, sizeof(r.action), "%s : %.3f -> %.3f (%+d %%)",
			gearName(gear, name, sizeof(name)), ratio, ratio * factor,
			static_cast<int>(lroundf((factor - 1.0f) * 100.0f)));
	}

	// Facteur maximal pour raccourcir le rapport gear (ratio x facteur) sans faire
	// retomber le passage gear -> gear+1 sous le couple max (ni, pour le dernier
	// rapport, atteindre le passage en bout de ligne droite).
	static float shortenLimit(const Summary &s, const EngineBand &engine, int gear)
	{
		if (gear < s.topGear)
		{
			const float after = s.rpmAfterShift(gear, static_cast<float>(s.shiftRpm));
			return isfinite(after) ? after / engine.torqueRpm : 1.0f;
		}
		return isfinite(s.peakRpmRatio) && s.peakRpmRatio > 0.0f ? 1.0f / s.peakRpmRatio : 1.0f;
	}

	// Facteur maximal pour allonger le rapport gear (ratio / facteur) sans faire
	// retomber le passage gear-1 -> gear sous le couple max.
	static float lengthenLimit(const Summary &s, const EngineBand &engine, int gear)
	{
		if (gear <= 1) return ENGINEER_GEAR_MAX_FACTOR;
		const float after = s.rpmAfterShift(gear - 1, static_cast<float>(s.shiftRpm));
		return isfinite(after) ? after / engine.torqueRpm : 1.0f;
	}

	static void reportTopSpeed(const Summary &s, Report &r)
	{
		char top[8];
		gearName(max(s.topGear, 1), top, sizeof(top));
		if (!s.valid())
		{
			setVerdict(r, TOPIC_TOP_SPEED, LEVEL_UNKNOWN, "Attente : %d / %d tours valides",
				s.laps, ENGINEER_GEAR_MIN_LAPS);
			return;
		}
		if (s.topGear <= 0 || s.shiftRpm <= 0)
		{
			setVerdict(r, TOPIC_TOP_SPEED, LEVEL_UNKNOWN, "Rapports ou passage inconnus");
			return;
		}
		int percent = 0;
		if (s.limiterS >= ENGINEER_GEAR_SHORT_S)
		{
			setVerdict(r, TOPIC_TOP_SPEED, LEVEL_WARN, "Trop courte : %.1f s/tour au passage en %s",
				s.limiterS, top);
			percent = ENGINEER_GEAR_STEP_PERCENT;
		}
		else if (s.topGearS < ENGINEER_GEAR_UNUSED_S)
		{
			setVerdict(r, TOPIC_TOP_SPEED, LEVEL_WARN, "Trop longue : %s presque inutilisee", top);
			percent = -ENGINEER_GEAR_STEP_PERCENT;
		}
		else if (s.peakRpmRatio < ENGINEER_GEAR_LONG_RATIO)
		{
			setVerdict(r, TOPIC_TOP_SPEED, LEVEL_WARN, "Trop longue : %s a %d %% du passage au max",
				top, static_cast<int>(lroundf(s.peakRpmRatio * 100.0f)));
			// A vitesse egale, le regime suit le rapport : raccourcir de (1 - ratio)
			// amene le regime de passage a la vitesse max atteinte.
			percent = -static_cast<int>(lroundf((1.0f - s.peakRpmRatio) * 100.0f));
		}
		else
		{
			setVerdict(r, TOPIC_TOP_SPEED, LEVEL_OK, "OK : %s a %d %% du passage, %.1f s au passage",
				top, static_cast<int>(lroundf(s.peakRpmRatio * 100.0f)), s.limiterS);
			return;
		}
		if (!takeAction(r, TOPIC_TOP_SPEED)) return;
		if (s.calcMaxSpeed > 0)
			snprintf(r.action, sizeof(r.action), "Vitesse max : %d -> %d (%+d %%)", s.calcMaxSpeed,
				static_cast<int>(lroundf(s.calcMaxSpeed * (100 + percent) / 100.0f)), percent);
		else
			snprintf(r.action, sizeof(r.action), "Vitesse max %+d %%", percent);
		snprintf(r.note, sizeof(r.note), "Transmission > vitesse max (decale tous les rapports)");
	}

	static void reportSpacing(const Summary &s, const EngineBand &engine, Report &r)
	{
		if (!engine.known())
		{
			setVerdict(r, TOPIC_SPACING, LEVEL_UNKNOWN, "A saisir : couple et puissance (MOTEUR)");
			return;
		}
		// Passage g -> g+1 qui retombe le plus bas, au regime de passage du jeu ; seuls
		// les rapports utilises comptent, si des passages ont ete mesures.
		bool anyMeasured = false;
		for (int g = 1; g < s.topGear; ++g)
			if (s.shiftCount[g - 1] > 0) anyMeasured = true;
		int worst = 0;
		float worstAfter = NAN;
		for (int g = 1; g < s.topGear; ++g)
		{
			if (anyMeasured && s.shiftCount[g - 1] == 0) continue;
			const float after = s.rpmAfterShift(g, static_cast<float>(s.shiftRpm));
			if (isfinite(after) && (worst == 0 || after < worstAfter))
			{
				worst = g;
				worstAfter = after;
			}
		}
		if (worst == 0 || s.shiftRpm <= 0)
		{
			setVerdict(r, TOPIC_SPACING, LEVEL_UNKNOWN, "Rapports ou passage inconnus");
			return;
		}
		char lower[8];
		char upper[8];
		gearName(worst, lower, sizeof(lower));
		gearName(worst + 1, upper, sizeof(upper));
		if (worstAfter >= engine.torqueRpm)
		{
			setVerdict(r, TOPIC_SPACING, LEVEL_OK, "OK : retombee mini %d (%s>%s) > couple %d",
				static_cast<int>(lroundf(worstAfter)), lower, upper, engine.torqueRpm);
			return;
		}
		setVerdict(r, TOPIC_SPACING, LEVEL_WARN, "%s>%s retombe a %d < couple %d", lower, upper,
			static_cast<int>(lroundf(worstAfter)), engine.torqueRpm);
		if (!s.valid() || !takeAction(r, TOPIC_SPACING)) return;
		// Rapprocher : rapport du dessus plus court, sinon rapport du dessous plus long.
		const float needed = engine.torqueRpm / worstAfter;
		const float shorten = min(needed, shortenLimit(s, engine, worst + 1));
		const float lengthen = min(needed, lengthenLimit(s, engine, worst));
		if (shorten >= ENGINEER_GEAR_MIN_FACTOR && shorten >= lengthen)
		{
			ratioAction(r, s, worst + 1, shorten);
			if (shorten < needed && worst + 1 < s.topGear)
				limitNote(r, worst + 1);
			else if (shorten < needed)
				snprintf(r.note, sizeof(r.note), "Limite : dernier rapport sans atteindre le passage");
			else
				snprintf(r.note, sizeof(r.note), "Rapport plus grand = plus court");
		}
		else if (lengthen >= ENGINEER_GEAR_MIN_FACTOR)
		{
			ratioAction(r, s, worst, 1.0f / lengthen);
			if (lengthen < needed && worst > 1)
				limitNote(r, worst - 1);
			else
				snprintf(r.note, sizeof(r.note), "Rapport plus petit = plus long");
		}
		else
		{
			snprintf(r.action, sizeof(r.action), "Pas de marge : plage du moteur trop etroite");
			snprintf(r.note, sizeof(r.note), "Passer plus pres du rupteur limite la retombee");
		}
	}

	static void reportCorner(const Summary &s, const EngineBand &engine, Report &r)
	{
		if (!engine.known())
		{
			setVerdict(r, TOPIC_CORNER, LEVEL_UNKNOWN, "A saisir : couple et puissance (MOTEUR)");
			return;
		}
		if (s.cornerGear <= 0)
		{
			setVerdict(r, TOPIC_CORNER, LEVEL_UNKNOWN, "Pas encore de virage mesure");
			return;
		}
		const int g = s.cornerGear;
		char name[8];
		gearName(g, name, sizeof(name));
		if (s.cornerRpm >= engine.torqueRpm)
		{
			setVerdict(r, TOPIC_CORNER, LEVEL_OK, "OK : %s a %d tr/min (%d km/h)", name,
				static_cast<int>(lroundf(s.cornerRpm)), static_cast<int>(lroundf(s.cornerSpeedKmh)));
			return;
		}
		setVerdict(r, TOPIC_CORNER, LEVEL_WARN, "%s a %d < couple %d (%d km/h)", name,
			static_cast<int>(lroundf(s.cornerRpm)), engine.torqueRpm,
			static_cast<int>(lroundf(s.cornerSpeedKmh)));
		if (!s.valid() || !takeAction(r, TOPIC_CORNER)) return;
		// Un rapport plus bas, si le regime y reste sous le passage : pilotage.
		const float lowerRpm = g >= 2 && s.ratios[g - 2] > 0.0f && s.ratios[g - 1] > 0.0f
			? s.cornerRpm * s.ratios[g - 2] / s.ratios[g - 1]
			: NAN;
		if (isfinite(lowerRpm) && lowerRpm <= s.shiftRpm)
		{
			char lower[8];
			snprintf(r.action, sizeof(r.action), "Retrograder en %s (%d tr/min)",
				gearName(g - 1, lower, sizeof(lower)), static_cast<int>(lroundf(lowerRpm)));
			snprintf(r.note, sizeof(r.note), "Pilotage : pas de reglage a faire");
			return;
		}
		// Sinon raccourcir ce rapport, sans faire retomber le passage suivant sous le couple.
		const float needed = engine.torqueRpm / s.cornerRpm;
		const float factor = min(needed, shortenLimit(s, engine, g));
		if (factor < ENGINEER_GEAR_MIN_FACTOR)
		{
			snprintf(r.action, sizeof(r.action), "Pas de marge sur la %s", name);
			snprintf(r.note, sizeof(r.note), "Plus courte, le passage suivant tomberait sous le couple");
			return;
		}
		ratioAction(r, s, g, factor);
		if (factor < needed && g < s.topGear)
			limitNote(r, g);
		else if (factor < needed)
			snprintf(r.note, sizeof(r.note), "Limite : dernier rapport sans atteindre le passage");
		else
			snprintf(r.note, sizeof(r.note), "Rapport plus grand = plus court");
	}

	static void reportShifts(const Summary &s, const EngineBand &engine, Report &r)
	{
		if (!engine.known())
		{
			setVerdict(r, TOPIC_SHIFTS, LEVEL_UNKNOWN, "A saisir : couple et puissance (MOTEUR)");
			return;
		}
		bool anyMeasured = false;
		for (int g = 1; g < s.topGear; ++g)
		{
			if (s.shiftCount[g - 1] == 0) continue;
			anyMeasured = true;
			if (s.shiftUpRpm[g - 1] >= engine.powerRpm * ENGINEER_GEAR_EARLY_RATIO) continue;
			char lower[8];
			char upper[8];
			setVerdict(r, TOPIC_SHIFTS, LEVEL_WARN, "%s>%s a %d < puissance %d",
				gearName(g, lower, sizeof(lower)), gearName(g + 1, upper, sizeof(upper)),
				static_cast<int>(lroundf(s.shiftUpRpm[g - 1])), engine.powerRpm);
			if (!takeAction(r, TOPIC_SHIFTS)) return;
			snprintf(r.action, sizeof(r.action), "Passer plus tard : apres %d tr/min", engine.powerRpm);
			snprintf(r.note, sizeof(r.note), "Pilotage : pas de reglage a faire");
			return;
		}
		if (anyMeasured)
			setVerdict(r, TOPIC_SHIFTS, LEVEL_OK, "OK : passages apres la puissance max");
		else
			setVerdict(r, TOPIC_SHIFTS, LEVEL_UNKNOWN, "Aucun passage mesure (gaz ouverts)");
	}

	void commit(const LapStats &lap)
	{
		++total_.laps;
		total_.maxSpeedKmh += lap.maxSpeedKmh;
		total_.topGearS += lap.topGearS;
		total_.limiterS += lap.limiterS;
		total_.peakRpm += lap.peakRpm;
		if (lap.cornerGear > 0)
		{
			total_.cornerCount[lap.cornerGear - 1]++;
			total_.cornerRpm[lap.cornerGear - 1] += lap.cornerRpm;
			total_.cornerSpeedKmh[lap.cornerGear - 1] += lap.cornerSpeedKmh;
		}
	}

	LapStats lap_;
	Totals total_;
	bool lapValid_ = false;	// le tour en cours a commence dans la session
	bool lapPit_ = false;	// stand attribue au tour en cours
	int currentLap_ = 0;
	LapStats pending_;	// tour termine, en attente de la fin du tour suivant
	int pendingLap_ = 0;
	bool pendingPit_ = false;
	bool pendingValid_ = false;
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
