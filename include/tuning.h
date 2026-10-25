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
