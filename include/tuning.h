// 06/11/2026 16:05 - Ajout des reglages du conseil de repartition de freinage en roulant (ligne F) ; seuils existants inchanges
// 06/11/2026 02:05 - Ajout des reglages de l'etagement (passages mesures, virage lent, passages trop tot) et de la saisie des regimes moteur ; seuils existants inchanges
// 06/11/2026 01:45 - Ajout des reglages des ecrans BOITE (vitesse max) et SUSPENSION (releves) ; seuils existants inchanges
// 30/10/2026 15:28 - Ajout des reglages de l'ingenieur de course (toucher a l'arret, donnees minimales, classes de R/F, virages rapides, ecart minimal) ; seuils existants inchanges
// 23/10/2026 15:05 - Creation : reglages ajustables a la main (detection du pit, analyse HANDLING, LED de passage de rapport), regroupes hors du code pour ne plus etre ecrases par une livraison
#pragma once

#include <stdint.h>

// ============================================================================
// REGLAGES AJUSTABLES
// Ce fichier n'est modifie que par Alain (ou avec son accord explicite).
// Le code lit ces valeurs ; il suffit de recompiler apres modification.
// ============================================================================

// ---------------------------------------------------------------------------
// Detection du passage au pit (GT7 n'envoie aucun drapeau pit).
// Motif : vitesse stabilisee au limiteur (phase 1) -> arret (phase 2)
//         -> vitesse de nouveau stabilisee (phase 3).
// Trace des phases sur le port serie : -DDASHBOARD_PIT_DEBUG=1 (platformio.ini).
// ---------------------------------------------------------------------------
static constexpr float LAP_PIT_STABLE_BAND_KMH = 2.0f;	// tolerance pour dire "vitesse stable"
static constexpr uint32_t LAP_PIT_STABLE_MS = 800;	// duree minimale des phases 1 et 3
static constexpr float LAP_PIT_MIN_KMH = 58.0f;	// plage de vitesse acceptee pour les phases 1 et 3
static constexpr float LAP_PIT_MAX_KMH = 81.0f;
static constexpr float LAP_PIT_STOPPED_KMH = 1.0f;	// en dessous : arret (phase 2)
static constexpr uint32_t LAP_PIT_STOP_MS = 1500;	// duree minimale de l'arret
static constexpr uint32_t LAP_PIT_APPROACH_TIMEOUT_MS = 10000;	// delai max entre phase 1 et arret
static constexpr uint32_t LAP_PIT_LEAVE_TIMEOUT_MS = 15000;	// delai max entre arret et phase 3

// ---------------------------------------------------------------------------
// Analyse de comportement (cellule HANDLING du theme Radar).
// Lignes E / M / S = entree / milieu / sortie de virage ; Y/N = equilibre,
// R/F = blocage du differentiel arriere (propulsion).
// ---------------------------------------------------------------------------
// Detection du virage.
static constexpr float HANDLING_MIN_NEUTRAL_YAW = 0.1f;	// rad/s : en dessous, ligne droite
static constexpr float HANDLING_MIN_FRONT_DIFF_KMH = 0.5f;	// en dessous, R/F non calcule
// Phases, d'apres les pedales (en %).
static constexpr int HANDLING_BRAKE_MIN_PERCENT = 5;	// frein au-dela : entree (E)
static constexpr int HANDLING_LIFT_MAX_THROTTLE_PERCENT = 10;	// gaz en deca : entree (E)
static constexpr int HANDLING_EXIT_MIN_THROTTLE_PERCENT = 50;	// gaz au-dela : sortie (S), sinon milieu (M)
// Moyennes glissantes.
static constexpr float HANDLING_AVERAGE_TIME_S = 10.0f;	// secondes de virage cumule par phase
static constexpr float HANDLING_MIN_SAMPLE_TIME_S = 0.5f;	// avant : "--"
// Couleur de la ligne selon Y/N : bleu en dessous, rouge au-dessus (ou negatif), blanc entre.
static constexpr float HANDLING_UNDERSTEER_RATIO = 0.9f;
static constexpr float HANDLING_OVERSTEER_RATIO = 1.1f;

// ---------------------------------------------------------------------------
// LED de passage de rapport (clignotement bleu a l'approche de minAlertRPM,
// uniquement quand le regime monte).
// ---------------------------------------------------------------------------
static constexpr float SHIFT_LED_APPROACH_START_RATIO = 0.85f;	// debut de zone = 85 % du seuil
static constexpr uint32_t SHIFT_LED_BLINK_HALF_PERIOD_SLOW_MS = 200;	// debut de zone : 2,5 clignotements/s
static constexpr uint32_t SHIFT_LED_BLINK_HALF_PERIOD_FAST_MS = 40;	// pres du seuil : 12,5 clignotements/s
static constexpr uint32_t SHIFT_LED_DIRECTION_WINDOW_MS = 100;	// intervalle de comparaison du regime
static constexpr int SHIFT_LED_DIRECTION_HYSTERESIS_RPM = 40;	// ecart minimal pour changer de sens

// ---------------------------------------------------------------------------
// Ingenieur de course : ecran ouvert par un appui sur la cellule HANDLING,
// voiture arretee. Bilan de la session (depuis le dernier retour en piste ou
// RAZ), un conseil, et comparaison avec la session precedente.
// Equilibre neutre : memes seuils que les couleurs HANDLING ci-dessus.
// ---------------------------------------------------------------------------
// Toucher en jeu : accepte seulement a l'arret (evite d'ouvrir un ecran en roulant).
static constexpr float ENGINEER_STOPPED_KMH = 1.0f;	// toucher accepte si vitesse <= cette valeur
static constexpr float ENGINEER_CLOSE_KMH = 5.0f;	// au-dela : ecrans de reglage refermes
// Donnees minimales pour conclure sur une ligne (virage cumule dans la session).
static constexpr float ENGINEER_MIN_TIME_S = 10.0f;	// Y/N : en deca, ligne "peu" et ignoree
static constexpr float ENGINEER_MIN_LOCK_TIME_S = 5.0f;	// R/F : en deca, blocage inconnu
// Classes de R/F (blocage du differentiel arriere).
static constexpr float ENGINEER_LOCK_SPIN_MAX = -0.10f;	// en dessous : roue interieure qui patine
static constexpr float ENGINEER_LOCK_LOCKED_MAX = 0.30f;	// en dessous : arriere bloque (~0)
static constexpr float ENGINEER_LOCK_FREE_MIN = 0.80f;	// au-dessus : arriere libre (~1), DGL hors de cause
// Virages rapides (aero dominante) : separes des virages lents pour orienter le conseil chassis.
static constexpr float ENGINEER_FAST_KMH = 120.0f;
static constexpr float ENGINEER_BAND_MIN_TIME_S = 5.0f;	// donnees minimales par tranche de vitesse
// Comparaison avant / apres : ecart minimal de |Y/N - 1| pour parler d'effet.
static constexpr float ENGINEER_MIN_CHANGE = 0.03f;
// Pas des conseils DGL (Initial / Accel / Decel).
static constexpr int ENGINEER_DGL_STEP = 5;

// ---------------------------------------------------------------------------
// Ecran BOITE (appui sur le rapport engage, a l'arret) : reglage "vitesse max"
// de la transmission, d'apres le dernier rapport, par tour complet sans pit.
// Regime de passage = minAlertRPM envoye par le jeu.
// ---------------------------------------------------------------------------
static constexpr int ENGINEER_GEAR_MIN_LAPS = 2;	// tours complets avant de conclure
static constexpr float ENGINEER_GEAR_LIMIT_RATIO = 1.0f;	// "au passage" : regime >= ce ratio x passage
static constexpr float ENGINEER_GEAR_SHORT_S = 0.5f;	// s/tour au passage sur le dernier rapport : trop courte
static constexpr float ENGINEER_GEAR_UNUSED_S = 0.5f;	// s/tour sur le dernier rapport en deca : trop longue
static constexpr float ENGINEER_GEAR_LONG_RATIO = 0.90f;	// regime max / passage en deca : trop longue
static constexpr int ENGINEER_GEAR_STEP_PERCENT = 5;	// pas du conseil (trop courte, rapport inutilise)
// Etagement (si les regimes de couple max et de puissance max sont saisis : ecran MOTEUR).
static constexpr int ENGINEER_GEAR_SHIFT_MIN_THROTTLE = 50;	// % de gaz : passage montant mesure
static constexpr float ENGINEER_GEAR_CORNER_MIN_KMH = 25.0f;	// point le plus lent : au-dessus (tete-a-queue exclu)
static constexpr float ENGINEER_GEAR_EARLY_RATIO = 0.97f;	// passage < ce ratio x puissance max : trop tot
// Saisie des regimes (ecran MOTEUR).
static constexpr int ENGINE_RPM_SMALL_STEP = 100;
static constexpr int ENGINE_RPM_LARGE_STEP = 1000;
static constexpr int ENGINE_RPM_MAX = 20000;

// ---------------------------------------------------------------------------
// Ecran SUSPENSION (appui sur TYRE TEMP, a l'arret) : releves par situation.
// ---------------------------------------------------------------------------
static constexpr float SUSPENSION_MIN_KMH = 30.0f;	// en deca (et au-dessus de l'arret) : non classe
static constexpr float SUSPENSION_CORNER_YAW = 0.15f;	// rad/s : virage franc
static constexpr float SUSPENSION_STRAIGHT_YAW = 0.05f;	// rad/s : ligne droite (acceleration)

// ---------------------------------------------------------------------------
// Conseil de repartition de freinage en roulant : "F 0.82 >AR" quelques secondes
// sur la ligne F de la cellule HANDLING (voir src/dashboard/BrakeHint.h).
// ---------------------------------------------------------------------------
static constexpr float BRAKE_HINT_WINDOW_S = 10.0f;	// freinage en virage mesure par message
static constexpr float BRAKE_HINT_MIN_LIFT_S = 3.0f;	// L mesure en deca : L non verifie
static constexpr float BRAKE_HINT_UNDER_RATIO = 0.85f;	// F en deca : repartition vers l'AR
static constexpr float BRAKE_HINT_OVER_RATIO = 1.15f;	// F au-dessus : repartition vers l'AV
static constexpr int BRAKE_HINT_MAX_REPEAT = 3;	// messages de suite dans le meme sens, puis silence
static constexpr uint32_t BRAKE_HINT_SHOW_MS = 4000;	// duree d'affichage
static constexpr float BRAKE_HINT_MIN_KMH = 60.0f;	// affichage : ligne droite rapide...
static constexpr int BRAKE_HINT_MIN_THROTTLE = 80;	// ... gaz ouverts...
static constexpr float BRAKE_HINT_STRAIGHT_YAW = 0.05f;	// ... sans virage (rad/s)
