// 25/10/2026 16:40 - Creation : debit de carburant instantane en L/min (essai "debitmetre"), calcule sur une fenetre glissante de ~2 s a partir du niveau envoye a chaque paquet
#pragma once

#include <Arduino.h>
#include <math.h>

// Debit de carburant instantane (L/min), a partir de fuelLevel (litres) que GT7
// envoie a chaque paquet (~60 par seconde).
//
// Un echantillon est garde toutes les FUEL_FLOW_SAMPLE_MS ; le debit est la baisse
// du niveau entre le plus ancien et le plus recent echantillon de la fenetre,
// divisee par le temps ecoule. La fenetre (~2 s) lisse l'affichage et absorbe un
// niveau qui evoluerait par petits paliers.
//
// Remise a zero (affichage "--") :
//  - niveau invalide, voiture electrique ou jeu en pause (appelant : valid = false) ;
//  - trou de telemetrie > FUEL_FLOW_MAX_GAP_MS (menus, perte Wi-Fi) ;
//  - niveau qui remonte de plus de FUEL_FLOW_REFUEL_L (ravitaillement) ;
//  - niveau qui chute de plus de FUEL_FLOW_MAX_DROP_L entre 2 echantillons (changement
//    de voiture ou de course : aucun moteur ne consomme autant en 0,2 s).
class FuelFlow
{
public:
	static constexpr uint32_t FUEL_FLOW_SAMPLE_MS = 200;
	static constexpr int FUEL_FLOW_SAMPLES = 11;	// 10 intervalles = fenetre de 2 s
	static constexpr int FUEL_FLOW_MIN_SAMPLES = 6;	// 1 s de donnees avant d'afficher
	static constexpr uint32_t FUEL_FLOW_MAX_GAP_MS = 500;
	static constexpr float FUEL_FLOW_REFUEL_L = 0.05f;
	static constexpr float FUEL_FLOW_MAX_DROP_L = 0.5f;	// = 150 L/min sur 0,2 s

	// NAN tant qu'il n'y a pas assez de donnees.
	float litersPerMinute = NAN;

	void reset()
	{
		count_ = 0;
		litersPerMinute = NAN;
	}

	void update(float fuelLiters, bool valid, uint32_t now)
	{
		const bool gap = lastSeen_ != 0 && now - lastSeen_ > FUEL_FLOW_MAX_GAP_MS;
		lastSeen_ = now;
		if (!valid || gap)
		{
			reset();
			if (!valid) return;
		}

		if (count_ > 0)
		{
			const Sample &last = sample(count_ - 1);
			if (fuelLiters > last.fuel + FUEL_FLOW_REFUEL_L ||
				fuelLiters < last.fuel - FUEL_FLOW_MAX_DROP_L)
				reset();	// ravitaillement ou saut de niveau : on repart du nouveau niveau
			else if (now - last.time < FUEL_FLOW_SAMPLE_MS)
				return;
		}
		push({fuelLiters, now});

		if (count_ < FUEL_FLOW_MIN_SAMPLES) return;
		const Sample &oldest = sample(0);
		const Sample &newest = sample(count_ - 1);
		const float minutes = (newest.time - oldest.time) / 60000.0f;
		if (minutes <= 0.0f) return;
		// Jamais negatif : une legere remontee (arrondi du jeu) donne 0.
		litersPerMinute = fmaxf(0.0f, (oldest.fuel - newest.fuel) / minutes);
	}

private:
	struct Sample
	{
		float fuel;
		uint32_t time;
	};

	// index 0 = plus ancien echantillon de la fenetre.
	const Sample &sample(int index) const
	{
		return samples_[(head_ + index) % FUEL_FLOW_SAMPLES];
	}

	void push(const Sample &s)
	{
		if (count_ < FUEL_FLOW_SAMPLES)
		{
			samples_[(head_ + count_) % FUEL_FLOW_SAMPLES] = s;
			++count_;
		}
		else
		{
			samples_[head_] = s;	// remplace le plus ancien
			head_ = (head_ + 1) % FUEL_FLOW_SAMPLES;
		}
	}

	Sample samples_[FUEL_FLOW_SAMPLES] = {};
	int head_ = 0;
	int count_ = 0;
	uint32_t lastSeen_ = 0;
};
