// 08/11/2026 20:45 - Regle globale : meme tendance sur toutes les lignes mesurees -> chassis d'abord (antiroulis), puis DGL sur la ligne la plus ecartee ; S sous-virage avec roue interieure qui patine : hors tableau, plus de conseil Accel + (lignes suivantes examinees)
// 06/11/2026 02:05 - Regimes de couple max et de puissance max memorises par voiture (cle separee "m<carCode>", l'enregistrement v2 ne change pas) pour l'etagement de la boite
// 06/11/2026 01:45 - Bouton FAIT : conseil marque applique (comparaison certaine "Accel +5 : S 1.20 -> 1.05"), sinon session "sans changement" (ecart = bruit de mesure) ; enregistrement v2 (la transmission des enregistrements v1 est reprise)
// 30/10/2026 15:28 - Creation : ingenieur de course (bilan de session F/L/M/S, un conseil DGL / repartition de freinage / chassis, comparaison avec la session precedente), memorise par voiture (carCode) en flash
#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <math.h>
#include "tuning.h"
#include "HandlingTrace.h"
#include "GearboxTrace.h"

// Ingenieur de course : analyse les cumuls de session de HandlingTrace et donne
// UN conseil, dans l'ordre de la methode de reglage :
//   S (Accel) -> F / L (repartition de freinage, puis Decel) -> M (Initial).
// Exception : si toutes les lignes mesurees (au moins ENGINEER_GLOBAL_MIN_LINES)
// ont la meme tendance, la cause est commune a toutes les phases : chassis d'abord
// (antiroulis), le DGL ensuite sur la ligne la plus ecartee. Le reglage marque
// FAIT est alors juge sur l'ecart moyen a 1 de toutes les lignes.
// Regles : voir claude/lecture-cellule-handling.md (projet).
//
// Une "session" va d'un retour en piste (le DGL ne se regle qu'au garage) ou d'un
// appui sur RAZ (repartition de freinage reglee en piste) au suivant. A la fin
// d'une session, son bilan et le conseil qui en decoule sont memorises pour la
// voiture (carCode) : la session suivante est comparee a ce bilan "avant".
//
// Les valeurs du DGL ne sont pas saisies : les conseils sont relatifs ("Accel +5").
// Le bouton FAIT indique que le conseil a ete applique : la session suivante est
// comparee sur la ligne visee ("Accel +5 : S 1.20 -> 1.05 mieux"). Une session
// terminee sans FAIT est "sans changement" : l'ecart avec la precedente donne le
// bruit de mesure (pilotage, pneus, carburant), a comparer a ENGINEER_MIN_CHANGE.
//
// Les conseils DGL ne valent que pour une propulsion (FR / MR / RR) : R/F compare
// l'ecart des roues arriere (motrices) a celui des roues avant (libres).
class RaceEngineer
{
public:
	enum Drivetrain : uint8_t { DRIVE_UNKNOWN, DRIVE_FR, DRIVE_MR, DRIVE_RR, DRIVE_OTHER };
	enum Verdict : uint8_t { VERDICT_NONE, VERDICT_WAITING, VERDICT_BETTER, VERDICT_WORSE, VERDICT_NO_EFFECT,
		VERDICT_NOISE };	// NOISE : session precedente sans changement
	enum AdviceKind : uint8_t { ADVICE_INFO, ADVICE_SETTING, ADVICE_OK };

	static constexpr int PHASE_COUNT = HandlingTrace::PHASE_COUNT;
	static constexpr int ALL_LINES = PHASE_COUNT;	// conseil global (toutes les lignes)
	static char phaseName(int phase) { return "FLMS"[phase]; }

	// Bilan d'une phase sur la session (NAN = pas de donnees).
	struct PhaseSummary
	{
		float balance = NAN;	// Y/N moyen
		float balanceTimeS = 0.0f;	// temps de virage cumule
		float lock = NAN;	// R/F moyen
		float lockTimeS = 0.0f;
		float slowBalance = NAN;	// Y/N en virages lents (< ENGINEER_FAST_KMH)
		float slowTimeS = 0.0f;
		float fastBalance = NAN;	// Y/N en virages rapides
		float fastTimeS = 0.0f;

		bool valid() const { return balanceTimeS >= ENGINEER_MIN_TIME_S; }
	};

	struct Summary
	{
		PhaseSummary phase[PHASE_COUNT];

		bool anyValid() const
		{
			for (int p = 0; p < PHASE_COUNT; ++p)
				if (phase[p].valid()) return true;
			return false;
		}
	};

	struct Advice
	{
		AdviceKind kind = ADVICE_INFO;
		int8_t line = -1;	// phase visee, -1 = aucune
		char diagnosis[64] = "";
		char action[64] = "";
		char note[64] = "";
		char label[24] = "";	// forme courte, reprise dans la comparaison
		bool onTrack = false;	// reglable en piste (repartition de freinage) : FAIT fait aussi la RAZ
	};

	// Bilan "avant" (session precedente) d'une phase, pour l'affichage.
	struct BeforePhase
	{
		float balance = NAN;
		float lock = NAN;
	};

	void begin()
	{
		ready_ = prefs_.begin("gt7eng", false);
	}

	// Voiture en piste : charge ce qui a ete memorise pour elle (transmission, bilan avant).
	void selectCar(int32_t carCode)
	{
		carCode_ = carCode;
		hasCar_ = true;
		record_ = CarRecord();
		engine_ = GearboxTrace::EngineBand();
		cancelApplied();
		if (!ready_) return;
		char key[16];
		makeEngineKey(key, sizeof(key));
		if (prefs_.getBytesLength(key) == sizeof(EngineRecord))
		{
			EngineRecord stored;
			prefs_.getBytes(key, &stored, sizeof(stored));
			engine_.torqueRpm = stored.torqueRpm;
			engine_.powerRpm = stored.powerRpm;
		}
		makeKey(key, sizeof(key));
		const size_t length = prefs_.getBytesLength(key);
		if (length == sizeof(CarRecord))
		{
			CarRecord stored;
			prefs_.getBytes(key, &stored, sizeof(stored));
			if (stored.version == RECORD_VERSION)
				record_ = stored;
		}
		else if (length >= 2 && length <= 128)
		{
			// Enregistrement d'une version precedente : seule la transmission est reprise.
			uint8_t raw[128];
			prefs_.getBytes(key, raw, length);
			if (raw[0] == 1 && raw[1] <= DRIVE_OTHER)
				record_.drivetrain = raw[1];
		}
	}

	bool hasCar() const { return hasCar_; }
	int32_t carCode() const { return carCode_; }
	Drivetrain drivetrain() const { return static_cast<Drivetrain>(record_.drivetrain); }

	void setDrivetrain(Drivetrain drivetrain)
	{
		record_.drivetrain = drivetrain;
		save();
	}

	// Plage utile du moteur (ecran MOTEUR), memorisee pour la voiture.
	const GearboxTrace::EngineBand &engineBand() const { return engine_; }
	void setEngineBand(int torqueRpm, int powerRpm)
	{
		engine_.torqueRpm = constrain(torqueRpm, 0, ENGINE_RPM_MAX);
		engine_.powerRpm = constrain(powerRpm, 0, ENGINE_RPM_MAX);
		if (!ready_ || !hasCar_) return;
		EngineRecord stored;
		stored.torqueRpm = static_cast<uint16_t>(engine_.torqueRpm);
		stored.powerRpm = static_cast<uint16_t>(engine_.powerRpm);
		char key[16];
		makeEngineKey(key, sizeof(key));
		prefs_.putBytes(key, &stored, sizeof(stored));
	}

	// Bouton FAIT : le conseil affiche est applique (pris en compte en fin de session).
	void markApplied(const Advice &advice)
	{
		if (advice.kind != ADVICE_SETTING || advice.line < 0) return;
		appliedPending_ = true;
		appliedLine_ = advice.line;
		snprintf(appliedLabel_, sizeof(appliedLabel_), "%s", advice.label);
	}
	void cancelApplied()
	{
		appliedPending_ = false;
		appliedLine_ = -1;
		appliedLabel_[0] = '\0';
	}
	bool appliedPending() const { return appliedPending_; }
	const char *appliedLabel() const { return appliedLabel_; }

	BeforePhase before(int phase) const
	{
		BeforePhase result;
		if (record_.hasBefore && record_.beforeTimeS[phase] >= ENGINEER_MIN_TIME_S)
		{
			result.balance = record_.beforeBalance[phase];
			if (record_.beforeLockTimeS[phase] >= ENGINEER_MIN_LOCK_TIME_S)
				result.lock = record_.beforeLock[phase];
		}
		return result;
	}

	static Summary summarize(const HandlingTrace &trace)
	{
		Summary summary;
		for (int p = 0; p < PHASE_COUNT; ++p)
		{
			const HandlingTrace::SessionPhase &session = trace.sessionPhase(p);
			const HandlingTrace::SessionStat balance = session.totalBalance();
			const HandlingTrace::SessionStat lock = session.totalLock();
			const HandlingTrace::SessionStat &slow = session.balance[HandlingTrace::BAND_SLOW];
			const HandlingTrace::SessionStat &fast = session.balance[HandlingTrace::BAND_FAST];
			PhaseSummary &out = summary.phase[p];
			out.balance = balance.mean();
			out.balanceTimeS = balance.timeS;
			out.lock = lock.mean();
			out.lockTimeS = lock.timeS;
			out.slowBalance = slow.mean();
			out.slowTimeS = slow.timeS;
			out.fastBalance = fast.mean();
			out.fastTimeS = fast.timeS;
		}
		return summary;
	}

	// Fin de session : son bilan devient "avant", avec le reglage marque FAIT (ou
	// "sans changement"). Une session trop courte (aucune ligne valide) ne remplace
	// pas le bilan precedent ; un FAIT y est tout de meme retenu (le changement
	// sera alors compare au bilan precedent).
	bool endSession(const Summary &summary)
	{
		if (!hasCar_) return false;
		const bool measured = summary.anyValid();
		if (!measured && !appliedPending_) return false;
		if (measured)
		{
			for (int p = 0; p < PHASE_COUNT; ++p)
			{
				record_.beforeBalance[p] = summary.phase[p].balance;
				record_.beforeTimeS[p] = summary.phase[p].balanceTimeS;
				record_.beforeLock[p] = summary.phase[p].lock;
				record_.beforeLockTimeS[p] = summary.phase[p].lockTimeS;
			}
			record_.hasBefore = 1;
		}
		record_.lastApplied = appliedPending_ ? 1 : 0;
		record_.lastLine = appliedLine_;
		snprintf(record_.lastLabel, sizeof(record_.lastLabel), "%s", appliedLabel_);
		cancelApplied();
		save();
		return true;
	}

	// Effet du reglage marque FAIT a la session precedente, sur la ligne visee ;
	// sans FAIT, plus grand ecart d'equilibre entre les deux sessions (bruit).
	Verdict compare(const Summary &now, char *text, size_t size) const
	{
		text[0] = '\0';
		if (!record_.hasBefore) return VERDICT_NONE;
		if (!record_.lastApplied) return compareUnchanged(now, text, size);
		const int line = record_.lastLine;
		if (line == ALL_LINES) return compareGlobal(now, text, size);
		if (line < 0 || line >= PHASE_COUNT) return VERDICT_NONE;
		const char name = phaseName(line);
		if (record_.beforeTimeS[line] < ENGINEER_MIN_TIME_S)
		{
			snprintf(text, size, "%s : pas de mesure %c avant le changement",
				record_.lastLabel, name);
			return VERDICT_WAITING;
		}
		const float before = record_.beforeBalance[line];
		if (!now.phase[line].valid())
		{
			snprintf(text, size, "%s : %c %.2f -> en attente de donnees",
				record_.lastLabel, name, before);
			return VERDICT_WAITING;
		}
		const float current = now.phase[line].balance;
		const char *conclusion;
		const Verdict verdict = judge(fabsf(current - 1.0f) - fabsf(before - 1.0f), conclusion);
		snprintf(text, size, "%s : %c %.2f -> %.2f %s",
			record_.lastLabel, name, before, current, conclusion);
		return verdict;
	}

	static HandlingTrace::Tendency tendencyOf(const PhaseSummary &phase)
	{
		if (!phase.valid()) return HandlingTrace::TENDENCY_NONE;
		return tendencyOfBalance(phase.balance);
	}

	Advice analyze(const Summary &s) const
	{
		Advice a;
		if (drivetrain() == DRIVE_UNKNOWN)
		{
			setText(a, ADVICE_INFO, -1, "Transmission inconnue",
				"Choisir FR, MR ou RR en haut a droite", "", "");
			return a;
		}
		if (drivetrain() == DRIVE_OTHER)
		{
			setText(a, ADVICE_INFO, -1, "FF / 4WD : conseils non pris en charge",
				"Le tableau Y/N reste lisible",
				"R/F ne vaut que pour une propulsion", "");
			return a;
		}
		if (!s.anyValid())
		{
			char action[64];
			snprintf(action, sizeof(action), "Rouler encore : %.0f s de virage par ligne",
				ENGINEER_MIN_TIME_S);
			setText(a, ADVICE_INFO, -1, "Pas assez de donnees", action,
				"Les lignes \"peu\" ne comptent pas", "");
			return a;
		}

		// Meme tendance partout : cause commune, chassis d'abord.
		if (globalTendency(s, a)) return a;

		// Ordre de la methode : S, puis F / L, puis M (qui decale toutes les lignes).
		const HandlingTrace::Phase brake = HandlingTrace::PHASE_BRAKE;
		const HandlingTrace::Phase lift = HandlingTrace::PHASE_LIFT;
		const HandlingTrace::Phase mid = HandlingTrace::PHASE_MID;
		const HandlingTrace::Phase exit = HandlingTrace::PHASE_EXIT;
		// S en sous-virage avec la roue interieure qui patine : hors du tableau (plus
		// de blocage reduirait le patinage mais pousserait davantage) ; pas de conseil
		// DGL, on examine les lignes suivantes.
		const bool exitUndecided = outOfBalance(s.phase[exit]) &&
			tendencyOf(s.phase[exit]) == HandlingTrace::TENDENCY_UNDER &&
			lockOf(s.phase[exit]) == LOCK_SPIN;
		if (outOfBalance(s.phase[exit]) && !exitUndecided) return adviceExit(s.phase[exit]);
		if (outOfBalance(s.phase[lift])) return adviceLift(s.phase[lift]);
		if (outOfBalance(s.phase[brake])) return adviceBrake(s.phase[brake], s.phase[lift]);
		if (outOfBalance(s.phase[mid])) return adviceMid(s.phase[mid]);
		if (exitUndecided)
		{
			char diagnosis[64];
			diagnosisOf(exit, s.phase[exit], diagnosis, sizeof(diagnosis));
			setText(a, ADVICE_INFO, exit, diagnosis, "Doser les gaz en sortie (ou TCS +1)",
				"Sous-virage + patinage : DGL non conclusif", "");
			return a;
		}

		// Moyennes neutres, mais une ligne qui ne derape qu'en virage rapide : aero.
		for (int p = 0; p < PHASE_COUNT; ++p)
			if (fastOnly(s.phase[p]))
				return aeroAdvice(p, s.phase[p], tendencyOfBalance(s.phase[p].fastBalance));

		// Toutes les lignes mesurees sont neutres.
		char missing[64] = "";
		for (int p = 0; p < PHASE_COUNT; ++p)
		{
			if (s.phase[p].valid()) continue;
			const size_t length = strlen(missing);
			snprintf(missing + length, sizeof(missing) - length,
				length == 0 ? "Sans donnees : %c" : " %c", phaseName(p));
		}
		setText(a, ADVICE_OK, -1, "Equilibre sain", "Rien a changer", missing, "");
		return a;
	}

private:
	static constexpr uint8_t RECORD_VERSION = 2;
	enum LockClass : uint8_t { LOCK_UNKNOWN, LOCK_SPIN, LOCK_LOCKED, LOCK_PARTIAL, LOCK_FREE };

	// Memorise en flash pour chaque voiture (cle "c<carCode>").
	struct CarRecord
	{
		uint8_t version = RECORD_VERSION;	// octet 0 (lu aussi pour les versions precedentes)
		uint8_t drivetrain = DRIVE_UNKNOWN;	// octet 1
		int8_t lastLine = -1;	// ligne visee par le reglage marque FAIT
		uint8_t hasBefore = 0;
		uint8_t lastApplied = 0;	// 1 = FAIT a la session precedente, 0 = sans changement
		float beforeBalance[PHASE_COUNT] = {NAN, NAN, NAN, NAN};
		float beforeTimeS[PHASE_COUNT] = {0, 0, 0, 0};
		float beforeLock[PHASE_COUNT] = {NAN, NAN, NAN, NAN};
		float beforeLockTimeS[PHASE_COUNT] = {0, 0, 0, 0};
		char lastLabel[24] = "";
	};

	static HandlingTrace::Tendency tendencyOfBalance(float balance)
	{
		if (!isfinite(balance)) return HandlingTrace::TENDENCY_NONE;
		if (balance < 0.0f || balance > HANDLING_OVERSTEER_RATIO) return HandlingTrace::TENDENCY_OVER;
		if (balance < HANDLING_UNDERSTEER_RATIO) return HandlingTrace::TENDENCY_UNDER;
		return HandlingTrace::TENDENCY_NEUTRAL;
	}

	static bool outOfBalance(const PhaseSummary &phase)
	{
		const HandlingTrace::Tendency tendency = tendencyOf(phase);
		return tendency == HandlingTrace::TENDENCY_UNDER || tendency == HandlingTrace::TENDENCY_OVER;
	}

	static LockClass lockOf(const PhaseSummary &phase)
	{
		if (phase.lockTimeS < ENGINEER_MIN_LOCK_TIME_S || !isfinite(phase.lock)) return LOCK_UNKNOWN;
		if (phase.lock < ENGINEER_LOCK_SPIN_MAX) return LOCK_SPIN;
		if (phase.lock < ENGINEER_LOCK_LOCKED_MAX) return LOCK_LOCKED;
		if (phase.lock < ENGINEER_LOCK_FREE_MIN) return LOCK_PARTIAL;
		return LOCK_FREE;
	}

	// Ecart a 1 qui diminue : mieux ; qui augmente : pire (au-dela du bruit).
	static Verdict judge(float change, const char *&conclusion)
	{
		if (change <= -ENGINEER_MIN_CHANGE)
		{
			conclusion = "mieux, garder";
			return VERDICT_BETTER;
		}
		if (change >= ENGINEER_MIN_CHANGE)
		{
			conclusion = "pire, revenir";
			return VERDICT_WORSE;
		}
		conclusion = "sans effet net";
		return VERDICT_NO_EFFECT;
	}

	// Conseil global : ecart moyen a 1 des lignes mesurees avant et maintenant.
	Verdict compareGlobal(const Summary &now, char *text, size_t size) const
	{
		float before = 0.0f;
		float current = 0.0f;
		int count = 0;
		for (int p = 0; p < PHASE_COUNT; ++p)
		{
			if (!now.phase[p].valid() || record_.beforeTimeS[p] < ENGINEER_MIN_TIME_S) continue;
			before += fabsf(record_.beforeBalance[p] - 1.0f);
			current += fabsf(now.phase[p].balance - 1.0f);
			++count;
		}
		if (count == 0)
		{
			snprintf(text, size, "%s : en attente de donnees", record_.lastLabel);
			return VERDICT_WAITING;
		}
		before /= count;
		current /= count;
		const char *conclusion;
		const Verdict verdict = judge(current - before, conclusion);
		snprintf(text, size, "%s : ecart moyen %.2f -> %.2f %s",
			record_.lastLabel, before, current, conclusion);
		return verdict;
	}

	Verdict compareUnchanged(const Summary &now, char *text, size_t size) const
	{
		float largest = -1.0f;
		int largestLine = -1;
		for (int p = 0; p < PHASE_COUNT; ++p)
		{
			if (!now.phase[p].valid() || record_.beforeTimeS[p] < ENGINEER_MIN_TIME_S) continue;
			const float change = fabsf(now.phase[p].balance - record_.beforeBalance[p]);
			if (change > largest)
			{
				largest = change;
				largestLine = p;
			}
		}
		if (largestLine < 0) return VERDICT_NONE;
		snprintf(text, size, "Sans changement : ecart max %.2f (%c), seuil %.2f",
			largest, phaseName(largestLine), ENGINEER_MIN_CHANGE);
		return VERDICT_NOISE;
	}

	static void setText(Advice &a, AdviceKind kind, int line, const char *diagnosis,
		const char *action, const char *note, const char *label)
	{
		a.kind = kind;
		a.line = static_cast<int8_t>(line);
		snprintf(a.diagnosis, sizeof(a.diagnosis), "%s", diagnosis);
		snprintf(a.action, sizeof(a.action), "%s", action);
		snprintf(a.note, sizeof(a.note), "%s", note);
		snprintf(a.label, sizeof(a.label), "%s", label);
	}

	// "S survirage : Y/N 1.20  R/F -0.29"
	static void diagnosisOf(int line, const PhaseSummary &phase, char *text, size_t size)
	{
		const bool over = tendencyOf(phase) == HandlingTrace::TENDENCY_OVER;
		if (lockOf(phase) == LOCK_UNKNOWN)
			snprintf(text, size, "%c %s : Y/N %.2f  R/F --", phaseName(line),
				over ? "survirage" : "sous-virage", phase.balance);
		else
			snprintf(text, size, "%c %s : Y/N %.2f  R/F %+.2f", phaseName(line),
				over ? "survirage" : "sous-virage", phase.balance, phase.lock);
	}

	// Conseil DGL "<reglage> +5" ou "-5".
	static void dglAdvice(Advice &a, int line, const PhaseSummary &phase, const char *setting,
		bool increase, const char *reason, const char *note)
	{
		char diagnosis[64];
		char action[64];
		char label[24];
		diagnosisOf(line, phase, diagnosis, sizeof(diagnosis));
		snprintf(label, sizeof(label), "%s %c%d", setting, increase ? '+' : '-', ENGINEER_DGL_STEP);
		snprintf(action, sizeof(action), "DGL %s : %s", label, reason);
		setText(a, ADVICE_SETTING, line, diagnosis, action, note, label);
	}

	static Advice missingLock(int line, const PhaseSummary &phase)
	{
		Advice a;
		char diagnosis[64];
		diagnosisOf(line, phase, diagnosis, sizeof(diagnosis));
		setText(a, ADVICE_INFO, line, diagnosis, "R/F pas encore mesure : rouler encore",
			"Il faut R/F pour savoir si le DGL est en cause", "");
		return a;
	}

	// Sortie (gaz francs) : DGL Accel.
	// Toutes les lignes mesurees (au moins ENGINEER_GLOBAL_MIN_LINES) dans la meme
	// tendance : antiroulis, puis DGL sur la ligne la plus ecartee.
	static bool globalTendency(const Summary &s, Advice &a)
	{
		int count = 0;
		HandlingTrace::Tendency common = HandlingTrace::TENDENCY_NONE;
		float sum = 0.0f;
		int extreme = -1;
		for (int p = 0; p < PHASE_COUNT; ++p)
		{
			const HandlingTrace::Tendency tendency = tendencyOf(s.phase[p]);
			if (tendency == HandlingTrace::TENDENCY_NONE) continue;
			if (tendency != HandlingTrace::TENDENCY_UNDER && tendency != HandlingTrace::TENDENCY_OVER)
				return false;
			if (count > 0 && tendency != common) return false;
			common = tendency;
			++count;
			sum += s.phase[p].balance;
			// La plus ecartee : la plus faible en sous-virage, la plus forte en survirage.
			if (extreme < 0 || (tendency == HandlingTrace::TENDENCY_UNDER
					? s.phase[p].balance < s.phase[extreme].balance
					: s.phase[p].balance > s.phase[extreme].balance))
				extreme = p;
		}
		if (count < ENGINEER_GLOBAL_MIN_LINES) return false;
		const bool under = common == HandlingTrace::TENDENCY_UNDER;
		char diagnosis[64];
		char note[64];
		snprintf(diagnosis, sizeof(diagnosis), "%s sur %d lignes (moyenne %.2f)",
			under ? "Sous-virage" : "Survirage", count, sum / count);
		snprintf(note, sizeof(note), "Chassis d'abord ; ensuite DGL sur %c (%.2f)",
			phaseName(extreme), s.phase[extreme].balance);
		setText(a, ADVICE_SETTING, ALL_LINES, diagnosis,
			under ? "Antiroulis AV -1 (ou AR +1)" : "Antiroulis AR -1 (ou AV +1)",
			note, under ? "Barres AV-1/AR+1" : "Barres AR-1/AV+1");
		return true;
	}

	static Advice adviceExit(const PhaseSummary &phase)
	{
		const int line = HandlingTrace::PHASE_EXIT;
		Advice a;
		switch (lockOf(phase))
		{
		case LOCK_UNKNOWN:
			return missingLock(line, phase);
		case LOCK_SPIN:
			dglAdvice(a, line, phase, "Accel", true, "la roue interieure patine",
				"Plus de blocage a l'acceleration");
			return a;
		case LOCK_LOCKED:
		case LOCK_PARTIAL:
			if (tendencyOf(phase) == HandlingTrace::TENDENCY_OVER)
				dglAdvice(a, line, phase, "Accel", false, "les 2 roues decrochent",
					"Si rien ne change : doser les gaz / TCS");
			else
				dglAdvice(a, line, phase, "Accel", false, "arriere trop bloque, pousse",
					"Moins de blocage a l'acceleration");
			return a;
		case LOCK_FREE:
		default:
			return chassisAdvice(line, phase);
		}
	}

	// Lever de pied (ni frein ni gaz) : DGL Decel seul.
	static Advice adviceLift(const PhaseSummary &phase)
	{
		const int line = HandlingTrace::PHASE_LIFT;
		Advice a;
		const LockClass lock = lockOf(phase);
		if (lock == LOCK_UNKNOWN) return missingLock(line, phase);
		if (tendencyOf(phase) == HandlingTrace::TENDENCY_UNDER)
		{
			if (lock == LOCK_FREE) return chassisAdvice(line, phase);
			dglAdvice(a, line, phase, "Decel", false, "arriere trop bloque",
				"Agit aussi sur F (freinage)");
			return a;
		}
		if (lock == LOCK_LOCKED) return chassisAdvice(line, phase);	// deja bloque
		dglAdvice(a, line, phase, "Decel", true, "arriere pas assez tenu",
			"Agit aussi sur F (freinage)");
		return a;
	}

	// Freinage avec L neutre (ou sans donnees) : repartition de freinage.
	static Advice adviceBrake(const PhaseSummary &phase, const PhaseSummary &lift)
	{
		const int line = HandlingTrace::PHASE_BRAKE;
		Advice a;
		char diagnosis[64];
		diagnosisOf(line, phase, diagnosis, sizeof(diagnosis));
		const bool under = tendencyOf(phase) == HandlingTrace::TENDENCY_UNDER;
		const char *note = lift.valid()
			? "L neutre : DGL hors de cause. Reglable en piste"
			: "L sans donnees : essayer le frein d'abord";
		setText(a, ADVICE_SETTING, line, diagnosis,
			under ? "Repartition de freinage vers l'ARRIERE" : "Repartition de freinage vers l'AVANT",
			note, under ? "Frein vers l'AR" : "Frein vers l'AV");
		a.onTrack = true;
		return a;
	}

	// Milieu (gaz partiels) : DGL Initial.
	static Advice adviceMid(const PhaseSummary &phase)
	{
		const int line = HandlingTrace::PHASE_MID;
		Advice a;
		const LockClass lock = lockOf(phase);
		if (lock == LOCK_UNKNOWN) return missingLock(line, phase);
		if (tendencyOf(phase) == HandlingTrace::TENDENCY_UNDER)
		{
			if (lock == LOCK_FREE) return chassisAdvice(line, phase);
			dglAdvice(a, line, phase, "Initial", false, "precharge trop forte",
				"Decale toutes les lignes : relire F/L/S");
			return a;
		}
		if (lock == LOCK_LOCKED) return chassisAdvice(line, phase);	// deja bloque
		dglAdvice(a, line, phase, "Initial", true, "precharge trop faible",
			"Decale toutes les lignes : relire F/L/S");
		return a;
	}

	// Arriere libre (R/F ~1) ou deja bloque : le DGL n'est pas en cause.
	// Virages rapides seuls -> aero ; sinon (ou lents et rapides) -> antiroulis.
	static Advice chassisAdvice(int line, const PhaseSummary &phase)
	{
		Advice a;
		char diagnosis[64];
		diagnosisOf(line, phase, diagnosis, sizeof(diagnosis));
		const HandlingTrace::Tendency tendency = tendencyOf(phase);
		const bool under = tendency == HandlingTrace::TENDENCY_UNDER;

		const bool bandsKnown = phase.slowTimeS >= ENGINEER_BAND_MIN_TIME_S &&
			phase.fastTimeS >= ENGINEER_BAND_MIN_TIME_S;
		if (fastOnly(phase) && tendencyOfBalance(phase.fastBalance) == tendency)
			return aeroAdvice(line, phase, tendency);

		// Mecanique : barres antiroulis (1 = souple, 10 = raide dans GT7).
		// Entree (F / L) : on agit sur l'essieu qui manque d'adherence ; milieu et
		// sortie : on peut aussi raidir l'essieu oppose.
		const bool entry = line == HandlingTrace::PHASE_BRAKE || line == HandlingTrace::PHASE_LIFT;
		const char *action;
		const char *label;
		if (under)
		{
			action = entry ? "Antiroulis AV -1 (plus souple)" : "Antiroulis AV -1, ou AR +1";
			label = entry ? "Antiroulis AV -1" : "Barres AV-1 / AR+1";
		}
		else
		{
			action = entry ? "Antiroulis AR -1 (plus souple)" : "Antiroulis AR -1, ou AV +1";
			label = entry ? "Antiroulis AR -1" : "Barres AR-1 / AV+1";
		}
		char note[64];
		if (bandsKnown)
			snprintf(note, sizeof(note), "DGL hors de cause. Lent %.2f / rapide %.2f",
				phase.slowBalance, phase.fastBalance);
		else
			snprintf(note, sizeof(note), "DGL hors de cause. Puis ressorts, pneus");
		setText(a, ADVICE_SETTING, line, diagnosis, action, note, label);
		return a;
	}

	// Neutre en virages lents, sous- ou survirage en virages rapides (aero dominante).
	static bool fastOnly(const PhaseSummary &phase)
	{
		if (phase.slowTimeS < ENGINEER_BAND_MIN_TIME_S || phase.fastTimeS < ENGINEER_BAND_MIN_TIME_S)
			return false;
		const HandlingTrace::Tendency fast = tendencyOfBalance(phase.fastBalance);
		return tendencyOfBalance(phase.slowBalance) == HandlingTrace::TENDENCY_NEUTRAL &&
			(fast == HandlingTrace::TENDENCY_UNDER || fast == HandlingTrace::TENDENCY_OVER);
	}

	static Advice aeroAdvice(int line, const PhaseSummary &phase, HandlingTrace::Tendency fast)
	{
		Advice a;
		const bool under = fast == HandlingTrace::TENDENCY_UNDER;
		char diagnosis[64];
		char note[64];
		snprintf(diagnosis, sizeof(diagnosis), "%c %s en virage rapide seulement",
			phaseName(line), under ? "sous-virage" : "survirage");
		snprintf(note, sizeof(note), "Rapide %.2f / lent %.2f : aero (si reglable)",
			phase.fastBalance, phase.slowBalance);
		setText(a, ADVICE_SETTING, line, diagnosis,
			under ? "Appui aero : + a l'AV (ou - a l'AR)" : "Appui aero : + a l'AR (ou - a l'AV)",
			note, under ? "Aero AV +" : "Aero AR +");
		return a;
	}

	void makeKey(char *key, size_t size) const
	{
		snprintf(key, size, "c%ld", static_cast<long>(carCode_));
	}

	void makeEngineKey(char *key, size_t size) const
	{
		snprintf(key, size, "m%ld", static_cast<long>(carCode_));
	}

	struct EngineRecord
	{
		uint16_t torqueRpm = 0;
		uint16_t powerRpm = 0;
	};

	void save()
	{
		if (!ready_ || !hasCar_) return;
		char key[16];
		makeKey(key, sizeof(key));
		prefs_.putBytes(key, &record_, sizeof(record_));
	}

	Preferences prefs_;
	bool ready_ = false;
	bool hasCar_ = false;
	int32_t carCode_ = 0;
	CarRecord record_;
	GearboxTrace::EngineBand engine_;
	bool appliedPending_ = false;
	int8_t appliedLine_ = -1;
	char appliedLabel_[24] = "";
};
