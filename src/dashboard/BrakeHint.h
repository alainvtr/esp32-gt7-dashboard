// 06/11/2026 16:05 - Creation : conseil de repartition de freinage en roulant, affiche quelques secondes sur la ligne F de la cellule HANDLING ("F 0.82 >AR"), sans pouvoir rester affiche en permanence
#pragma once

#include <Arduino.h>
#include <math.h>
#include "tuning.h"
#include "HandlingTrace.h"

// Conseil de repartition de freinage en roulant (reglable en piste, pas le DGL).
//
// Le jeu n'envoie pas la valeur de la repartition : la carte ne voit pas un reglage
// fait en roulant, et un desequilibre peut venir d'ailleurs (DGL Decel, chassis,
// trajectoire). Pour qu'un message ne reste pas affiche en permanence :
//  1. une fenetre de mesure par message : BRAKE_HINT_WINDOW_S de freinage en virage
//     (ligne F), mesures depuis la fin du message precedent (donc apres un reglage
//     eventuel) ;
//  2. seuils plus larges que les couleurs de la ligne F (desequilibre net) ;
//  3. pas de message si la ligne L (lever de pied) est elle aussi desequilibree :
//     le DGL est alors en cause, voir l'ingenieur de course a l'arret. L est pris
//     sur la fenetre, ou sur toute la session si la fenetre en contient trop peu
//     (le lever de pied est souvent bref) ;
//  4. au plus BRAKE_HINT_MAX_REPEAT messages de suite dans le meme sens, puis
//     silence jusqu'au prochain retour en piste (ou RAZ / FAIT de l'ingenieur) ;
//  5. affichage seulement en ligne droite, gaz ouverts, pendant BRAKE_HINT_SHOW_MS.
// Les fenetres sont prises dans les cumuls de session de HandlingTrace (difference
// entre maintenant et le debut de la fenetre).
class BrakeHint
{
public:
	enum Direction : int8_t { DIRECTION_NONE = 0, DIRECTION_REAR = 1, DIRECTION_FRONT = -1 };

	// Nouvelle session : compteurs a zero, fenetre ouverte maintenant.
	void reset(const HandlingTrace &handling)
	{
		state_ = STATE_COLLECTING;
		lastDirection_ = DIRECTION_NONE;
		repeat_ = 0;
		silenced_ = false;
		openWindow(handling);
	}

	void update(const HandlingTrace &handling, float speedKmh, int throttlePercent, float yaw, uint32_t now)
	{
		const HandlingTrace::SessionStat brake =
			handling.sessionPhase(HandlingTrace::PHASE_BRAKE).totalBalance();
		const HandlingTrace::SessionStat lift =
			handling.sessionPhase(HandlingTrace::PHASE_LIFT).totalBalance();
		// Cumuls remis a zero ailleurs (RAZ) : la fenetre repart de la.
		if (brake.timeS < windowBrake_.timeS || lift.timeS < windowLift_.timeS)
			openWindow(handling);

		switch (state_)
		{
		case STATE_COLLECTING:
			if (!silenced_ && brake.timeS - windowBrake_.timeS >= BRAKE_HINT_WINDOW_S)
				evaluate(handling, brake, lift);
			break;
		case STATE_PENDING:
			// En ligne droite, gaz ouverts : rien a gerer au volant a ce moment-la.
			if (speedKmh >= BRAKE_HINT_MIN_KMH && throttlePercent >= BRAKE_HINT_MIN_THROTTLE &&
				fabsf(yaw) <= BRAKE_HINT_STRAIGHT_YAW)
			{
				state_ = STATE_SHOWING;
				showUntil_ = now + BRAKE_HINT_SHOW_MS;
			}
			break;
		case STATE_SHOWING:
			if (static_cast<int32_t>(now - showUntil_) >= 0)
			{
				state_ = STATE_COLLECTING;
				openWindow(handling);	// mesure suivante : apres un reglage eventuel
			}
			break;
		}
	}

	bool showing() const { return state_ == STATE_SHOWING; }

	// Ligne F pendant l'affichage : "F 0.82 >AR".
	String line() const
	{
		char text[16];
		snprintf(text, sizeof(text), "F %.2f >%s", balance_,
			lastDirection_ == DIRECTION_REAR ? "AR" : "AV");
		return text;
	}

private:
	enum State : uint8_t { STATE_COLLECTING, STATE_PENDING, STATE_SHOWING };

	static float windowMean(const HandlingTrace::SessionStat &now, const HandlingTrace::SessionStat &start)
	{
		const float timeS = now.timeS - start.timeS;
		return timeS > 0.0f ? (now.weightedSum - start.weightedSum) / timeS : NAN;
	}

	static bool liftOutOfBalance(float balance)
	{
		return balance < HANDLING_UNDERSTEER_RATIO || balance > HANDLING_OVERSTEER_RATIO;
	}

	void evaluate(const HandlingTrace &handling, const HandlingTrace::SessionStat &brake,
		const HandlingTrace::SessionStat &lift)
	{
		const float brakeBalance = windowMean(brake, windowBrake_);
		bool liftKnown = true;
		float liftBalance = NAN;
		if (lift.timeS - windowLift_.timeS >= BRAKE_HINT_MIN_LIFT_S)
			liftBalance = windowMean(lift, windowLift_);
		else if (lift.timeS >= BRAKE_HINT_MIN_LIFT_S)
			liftBalance = lift.mean();	// toute la session
		else
			liftKnown = false;

		Direction direction = DIRECTION_NONE;
		if (brakeBalance < 0.0f || brakeBalance > BRAKE_HINT_OVER_RATIO)
			direction = DIRECTION_FRONT;	// survirage au freinage : plus de frein a l'avant
		else if (brakeBalance < BRAKE_HINT_UNDER_RATIO)
			direction = DIRECTION_REAR;	// sous-virage au freinage : plus de frein a l'arriere
		if (direction != DIRECTION_NONE && liftKnown && liftOutOfBalance(liftBalance))
			direction = DIRECTION_NONE;	// L aussi desequilibre : DGL Decel en cause

		openWindow(handling);
		if (direction == DIRECTION_NONE)
		{
			// Equilibre retrouve (ou autre cause) : le compteur repart de zero.
			lastDirection_ = DIRECTION_NONE;
			repeat_ = 0;
			return;
		}
		repeat_ = direction == lastDirection_ ? repeat_ + 1 : 1;
		lastDirection_ = direction;
		if (repeat_ > BRAKE_HINT_MAX_REPEAT)
		{
			silenced_ = true;	// la repartition ne suffit pas : voir l'ingenieur a l'arret
			return;
		}
		balance_ = brakeBalance;
		state_ = STATE_PENDING;
	}

	void openWindow(const HandlingTrace &handling)
	{
		windowBrake_ = handling.sessionPhase(HandlingTrace::PHASE_BRAKE).totalBalance();
		windowLift_ = handling.sessionPhase(HandlingTrace::PHASE_LIFT).totalBalance();
	}

	State state_ = STATE_COLLECTING;
	HandlingTrace::SessionStat windowBrake_;
	HandlingTrace::SessionStat windowLift_;
	Direction lastDirection_ = DIRECTION_NONE;
	int repeat_ = 0;
	bool silenced_ = false;
	float balance_ = NAN;
	uint32_t showUntil_ = 0;
};
