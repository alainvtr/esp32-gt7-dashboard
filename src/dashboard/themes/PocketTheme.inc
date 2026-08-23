	static uint8_t pocketGlyphRow(char c, int row)
	{
		static constexpr uint8_t DIGITS[10][7] = {
			{14, 17, 19, 21, 25, 17, 14}, {4, 12, 4, 4, 4, 4, 14},
			{14, 17, 1, 2, 4, 8, 31}, {30, 1, 1, 14, 1, 1, 30},
			{2, 6, 10, 18, 31, 2, 2}, {31, 16, 16, 30, 1, 1, 30},
			{14, 16, 16, 30, 17, 17, 14}, {31, 1, 2, 4, 8, 8, 8},
			{14, 17, 17, 14, 17, 17, 14}, {14, 17, 17, 15, 1, 1, 14}};
		if (c >= '0' && c <= '9') return DIGITS[c - '0'][row];
		static constexpr uint8_t N[7] = {17, 25, 25, 21, 19, 19, 17};
		static constexpr uint8_t R[7] = {30, 17, 17, 30, 20, 18, 17};
		static constexpr uint8_t DASH[7] = {0, 0, 0, 14, 0, 0, 0};
		if (c == 'N') return N[row];
		if (c == 'R') return R[row];
		if (c == '-') return DASH[row];
		return 0;
	}

	void drawPocketPixelText(LGFX_Sprite &sprite, const String &text,
		int centerX, int y, int scale, uint16_t color)
	{
		const int glyphW = 5 * scale;
		const int pitch = 6 * scale;
		const int startX = centerX - (text.length() * pitch - scale) / 2;
		for (size_t i = 0; i < text.length(); ++i)
		{
			for (int row = 0; row < 7; ++row)
			{
				const uint8_t bits = pocketGlyphRow(text[i], row);
				for (int col = 0; col < 5; ++col)
					if (bits & (1 << (4 - col)))
						sprite.fillRect(startX + i * pitch + col * scale,
							y + row * scale, scale, scale, color);
			}
		}
	}

	void drawPocketGearSpeed(const DashboardState &state, bool forceUpdate)
	{
		const String cacheState = state.gear + ":" + state.speed;
		if (!forceUpdate && prevData["pocketGearSpeed"] == cacheState) return;
		static LGFX_Sprite sprite(&tft);
		static bool created = false;
		if (!created)
		{
			sprite.setColorDepth(16);
			created = sprite.createSprite(158, 105) != nullptr;
		}
		if (!created) return;
		const uint16_t pale = tft.color565(188, 205, 124);
		const uint16_t darkest = tft.color565(23, 43, 18);
		sprite.fillSprite(pale);
		drawPocketPixelText(sprite, state.gear, 79, 4, 8, darkest);
		drawPocketPixelText(sprite, state.speed, 79, 72, 4, darkest);
		sprite.setTextColor(darkest, pale);
		sprite.setTextDatum(ML_DATUM);
		sprite.setTextSize(1.25f, 1.20f);
		sprite.drawString("KM/H", 120, 94, 1);
		sprite.setTextSize(1.0f);
		sprite.pushSprite(7, 50);
		prevData["pocketGearSpeed"] = cacheState;
		releaseSpriteAfterThemePreview(sprite, created);
	}

	void drawPocketValue(int x, int y, int w, int h, const String &value,
		const String &id, uint16_t color, uint16_t background, uint8_t font,
		float scaleX, float scaleY, bool forceUpdate,
		textdatum_t datum = MC_DATUM)
	{
		const String cacheState = value + ":" + String(color);
		if (!forceUpdate && prevData[id] == cacheState) return;
		tft.fillRect(x, y, w, h, background);
		tft.setTextColor(color, background);
		tft.setTextDatum(datum);
		tft.setTextSize(scaleX, scaleY);
		const int textX = datum == MR_DATUM ? x + w - 2
			: (datum == ML_DATUM ? x + 2 : x + w / 2);
		tft.drawString(value, textX, y + h / 2, font);
		tft.setTextSize(1.0f);
		tft.setTextDatum(TL_DATUM);
		prevData[id] = cacheState;
	}

	void drawPocketValueBuffered(int x, int y, int w, int h,
		const String &value, const String &id, uint16_t color,
		uint16_t background, uint8_t font, float scaleX, float scaleY,
		bool forceUpdate, LGFX_Sprite &sprite, bool &created)
	{
		const String cacheState = value + ":" + String(color);
		if (!forceUpdate && prevData[id] == cacheState) return;
		if (!created)
		{
			sprite.setColorDepth(16);
			created = sprite.createSprite(w, h) != nullptr;
		}
		if (!created)
		{
			drawPocketValue(x, y, w, h, value, id, color, background,
				font, scaleX, scaleY, true);
			return;
		}
		sprite.fillSprite(background);
		sprite.setTextColor(color, background);
		sprite.setTextDatum(MC_DATUM);
		sprite.setTextSize(scaleX, scaleY);
		sprite.drawString(value, w / 2, h / 2, font);
		sprite.setTextSize(1.0f);
		sprite.pushSprite(x, y);
		prevData[id] = cacheState;
		releaseSpriteAfterThemePreview(sprite, created);
	}

	void drawPocketPixelFrame(int x, int y, int w, int h, uint16_t color)
	{
		static constexpr int T = 2;
		static constexpr int FOLD = 6;
		static constexpr int DEPTH = 6;
		// Orthogonal inward folds: across, inward, back to the outer side.
		tft.fillRect(x + FOLD, y, w - FOLD * 2, T, color);
		tft.fillRect(x + FOLD - T, y, T, DEPTH, color);
		tft.fillRect(x, y + DEPTH - T, FOLD, T, color);
		tft.fillRect(x + w - FOLD, y, T, DEPTH, color);
		tft.fillRect(x + w - FOLD, y + DEPTH - T, FOLD, T, color);

		tft.fillRect(x, y + DEPTH - T, T, h - (DEPTH - T) * 2, color);
		tft.fillRect(x + w - T, y + DEPTH - T, T,
			h - (DEPTH - T) * 2, color);

		tft.fillRect(x, y + h - DEPTH, FOLD, T, color);
		tft.fillRect(x + FOLD - T, y + h - DEPTH, T, DEPTH, color);
		tft.fillRect(x + FOLD, y + h - T, w - FOLD * 2, T, color);
		tft.fillRect(x + w - FOLD, y + h - DEPTH, FOLD, T, color);
		tft.fillRect(x + w - FOLD, y + h - DEPTH, T, DEPTH, color);
	}

	void drawPocketRpm(const DashboardState &state, bool forceUpdate)
	{
		static constexpr int COUNT = 14;
		static constexpr int X = 24;
		static constexpr int Y = 190;
		static constexpr int PITCH = 10;
		static int previousActive = -1;
		static uint8_t previousPulseStep = 255;
		const uint16_t darkest = tft.color565(23, 43, 18);
		const uint16_t dark = tft.color565(76, 95, 47);
		const uint16_t light = tft.color565(154, 166, 92);
		const int active = constrain((state.rpmPercent * COUNT + 99) / 100, 0, COUNT);
		const bool warning = (state.rpmAlertRangeValid &&
			state.rpmPercent >= state.rpmRedLineSetting) || state.revLimitAlertActive;
		uint8_t pulseStep = 0;
		if (warning)
		{
			const float wave = 0.5f + 0.5f * cosf(TWO_PI * 3.0f * millis() / 1000.0f);
			pulseStep = static_cast<uint8_t>(lroundf(wave * 5.0f));
		}
		if (!forceUpdate && active == previousActive && pulseStep == previousPulseStep)
			return;
		const uint16_t activeColor = warning
			? blendRgb565(darkest, dark, pulseStep * 35) : darkest;
		const uint16_t pale = tft.color565(188, 205, 124);
		// Clear only the marker lane; never touch the surrounding 2 px frame.
		tft.fillRect(13, 182, 158, 7, pale);
		for (int i = 0; i < COUNT; ++i)
		{
			const uint16_t color = i < active ? activeColor : light;
			tft.fillRect(X + i * PITCH, Y, 8, 12, color);
		}
		if (state.rpmAlertRangeValid)
		{
			const int markerX = X + constrain(
				state.rpmRedLineSetting * ((COUNT - 1) * PITCH) / 100,
				0, (COUNT - 1) * PITCH);
			tft.fillTriangle(markerX, 188, markerX - 3, 182,
				markerX + 3, 182, darkest);
		}
		previousActive = active;
		previousPulseStep = pulseStep;
	}

	void drawPocketFuelAids(const DashboardState &state, bool forceUpdate)
	{
		const bool fuelChanged = forceUpdate ||
			prevData["pocketFuel"] != state.brakeBias;
		const bool absChanged = forceUpdate ||
			prevData["pocketAbs"] != state.absActive;
		const bool tcsChanged = forceUpdate ||
			prevData["pocketTcs"] != state.tcActive;
		if (!fuelChanged && !absChanged && !tcsChanged) return;
		const uint16_t pale = tft.color565(188, 205, 124);
		const uint16_t darkest = tft.color565(23, 43, 18);
		const uint16_t dark = tft.color565(76, 95, 47);
		const uint16_t light = tft.color565(132, 146, 80);
		tft.setTextColor(darkest, pale);
		tft.setTextDatum(MC_DATUM);
		const bool validFuel = state.brakeBias != "--" && state.brakeBias.length();
		if (fuelChanged)
		{
			tft.fillRect(176, 165, 44, 60, pale);
			tft.setTextSize(1.0f);
			tft.drawString(validFuel ? state.brakeBias + "%" : "--", 198, 174, 2);
			// Narrower pump leaves enough visual room for the fuel percentage.
			tft.fillRect(192, 184, 13, 21, dark);
			tft.fillRect(195, 187, 7, 6, pale);
			tft.fillRect(189, 205, 19, 3, dark);
			tft.drawFastVLine(208, 189, 15, dark);
			tft.fillRect(205, 186, 4, 4, dark);
			tft.fillRect(209, 190, 3, 7, dark);
			tft.drawString("FUEL", 198, 220, 1);
		}

		const bool absOn = isActiveValue(state.absActive);
		const bool tcsOn = isActiveValue(state.tcActive);
		// Match the RPM bar: inactive is pale, active is the solid dark tone.
		const uint16_t absColor = absOn ? darkest : light;
		const uint16_t tcsColor = tcsOn ? darkest : light;

		if (absChanged)
		{
			tft.fillRect(222, 165, 44, 60, pale);
			// Compact circular ABS badge: active state is solid with reversed lettering.
			if (absOn)
				tft.fillCircle(244, 194, 11, absColor);
			else
			{
				tft.drawCircle(244, 194, 11, absColor);
				tft.drawCircle(244, 194, 10, absColor);
			}
			for (int angle = 132; angle <= 228; angle += 3)
			{
				const float radians = angle * DEG_TO_RAD;
				const int arcX = 244 + lroundf(cosf(radians) * 14.0f);
				const int arcY = 194 + lroundf(sinf(radians) * 14.0f);
				tft.drawPixel(arcX, arcY, absColor);
				tft.drawPixel(arcX - 1, arcY, absColor);
			}
			for (int angle = -48; angle <= 48; angle += 3)
			{
				const float radians = angle * DEG_TO_RAD;
				const int arcX = 244 + lroundf(cosf(radians) * 14.0f);
				const int arcY = 194 + lroundf(sinf(radians) * 14.0f);
				tft.drawPixel(arcX, arcY, absColor);
				tft.drawPixel(arcX + 1, arcY, absColor);
			}
			tft.setTextColor(absOn ? pale : absColor, absOn ? absColor : pale);
			tft.setTextDatum(TL_DATUM);
			static constexpr int absTextWidth = 18;
			static constexpr int absTextHeight = 8;
			tft.drawString("ABS", 245 - absTextWidth / 2,
				195 - absTextHeight / 2, 1);
			tft.setTextDatum(MC_DATUM);
			tft.setTextColor(darkest, pale);
			tft.drawString("ABS", 244, 221, 1);
		}

		if (tcsChanged)
		{
			tft.fillRect(268, 165, 38, 60, pale);
			tft.setTextColor(darkest, pale);
		// Chunky rear-view slipping-car glyph, drawn for this 42 px status cell.
		// Crisp symmetrical trapezoid roof; the inset trapezoid is the rear window.
		tft.fillTriangle(284, 181, 294, 181, 279, 189, tcsColor);
		tft.fillTriangle(294, 181, 299, 189, 279, 189, tcsColor);
		tft.fillTriangle(285, 183, 293, 183, 282, 188, pale);
		tft.fillTriangle(293, 183, 296, 188, 282, 188, pale);

		// Solid rear body with two square, background-colour lamps.
		tft.fillRect(279, 189, 22, 10, tcsColor);
		tft.fillRect(282, 191, 4, 3, pale);
		tft.fillRect(294, 191, 4, 3, pale);
		tft.fillRect(281, 199, 5, 3, tcsColor);
		tft.fillRect(294, 199, 5, 3, tcsColor);

		// Rounded pixel S trails: more intermediate points remove sharp elbows.
		static constexpr int8_t SKID[][2] = {
			{3, 0}, {4, 1}, {3, 2}, {1, 3}, {-1, 4},
			{-2, 5}, {-1, 6}, {1, 7}, {4, 7}};
		for (int trail = 0; trail < 2; ++trail)
		{
			const int originX = 280 + trail * 13;
			for (size_t i = 0; i < sizeof(SKID) / sizeof(SKID[0]); ++i)
			{
				const int x = originX + SKID[i][0];
				const int y = 202 + SKID[i][1];
				tft.fillCircle(x, y, 1, tcsColor);
				if (i > 0)
					tft.drawLine(originX + SKID[i - 1][0], 202 + SKID[i - 1][1],
						x, y, tcsColor);
			}
		}
		tft.drawString("TCS", 290, 221, 1);
		}

		// Three equal status columns.
		for (int y = 167; y < 227; y += 8)
		{
			tft.fillRect(220, y, 2, 4, dark);
			tft.fillRect(266, y, 2, 4, dark);
		}
		tft.setTextDatum(TL_DATUM);
		// Dynamic clearing must never leave the outer folded corners incomplete.
		drawPocketPixelFrame(10, 162, 300, 69, dark);
		prevData["pocketFuel"] = state.brakeBias;
		prevData["pocketAbs"] = state.absActive;
		prevData["pocketTcs"] = state.tcActive;
	}

	void drawPocketBottom(const DashboardState &state, bool forceUpdate)
	{
		String tyreState;
		for (int i = 0; i < 4; ++i)
			tyreState += (i ? ":" : "") +
				(isnan(state.tyreTemperatures[i]) ? String("--")
					: String(lroundf(state.tyreTemperatures[i])));
		const String cacheState = tyreState + ":" + state.absLevel + ":" +
			state.absFilteredLevel + ":" + state.tcLevel + ":" + state.tcFilteredLevel;
		if (!forceUpdate && prevData["pocketBottom"] == cacheState) return;
		const uint16_t pale = tft.color565(188, 205, 124);
		const uint16_t darkest = tft.color565(23, 43, 18);
		const uint16_t dark = tft.color565(76, 95, 47);
		static LGFX_Sprite sprite(&tft);
		static bool created = false;
		if (!created)
		{
			sprite.setColorDepth(16);
			created = sprite.createSprite(310, 20) != nullptr;
		}
		if (!created)
		{
			// Allocation can temporarily fail after browsing several themes. Keep
			// the panel functional and retry the buffered path on a later update.
			tft.fillRect(5, 216, 310, 20, pale);
			tft.setTextColor(darkest, pale);
			tft.setTextDatum(ML_DATUM);
			tft.drawString("TYRE", 8, 226, 1);
			const char *fallbackLabels[] = {"FL", "FR", "RL", "RR"};
			for (int i = 0; i < 4; ++i)
				tft.drawString(String(fallbackLabels[i]) +
					(isnan(state.tyreTemperatures[i]) ? String("--")
						: String(lroundf(state.tyreTemperatures[i]))),
					41 + i * 31, 226, 1);
			const int fallbackValues[] = {
				constrain(state.absFilteredLevel.toInt(), 0, 100),
				constrain(state.tcFilteredLevel.toInt(), 0, 100)};
			for (int row = 0; row < 2; ++row)
			{
				const int y = 218 + row * 9;
				tft.drawString(row == 0 ? "B" : "T", 179, y + 4, 1);
				tft.drawRect(190, y, 122, 7, dark);
				if (fallbackValues[row] > 0)
					tft.fillRect(191, y + 1,
						120 * fallbackValues[row] / 100, 5, darkest);
			}
			prevData["pocketBottom"] = cacheState;
			return;
		}

		sprite.fillSprite(pale);
		sprite.setTextColor(darkest, pale);
		sprite.setTextDatum(ML_DATUM);
		sprite.drawString("TYRE", 3, 10, 1);
		const char *labels[] = {"FL", "FR", "RL", "RR"};
		for (int i = 0; i < 4; ++i)
		{
			const String value = String(labels[i]) +
				(isnan(state.tyreTemperatures[i]) ? String("--")
					: String(lroundf(state.tyreTemperatures[i])));
			sprite.drawString(value, 36 + i * 31, 10, 1);
		}
		const int values[] = {constrain(state.absFilteredLevel.toInt(), 0, 100),
			constrain(state.tcFilteredLevel.toInt(), 0, 100)};
		for (int row = 0; row < 2; ++row)
		{
			const int y = 2 + row * 9;
			sprite.drawString(row == 0 ? "B" : "T", 174, y + 4, 1);
			sprite.drawRect(185, y, 122, 7, dark);
			if (values[row] > 0)
				sprite.fillRect(186, y + 1, 120 * values[row] / 100, 5, darkest);
		}
		sprite.pushSprite(5, 216);
		prevData["pocketBottom"] = cacheState;
		releaseSpriteAfterThemePreview(sprite, created);
	}

	void drawPocketDashboard(const DashboardState &state, bool forceUpdate)
	{
		static LGFX_Sprite currentSprite(&tft);
		static bool currentSpriteCreated = false;
		const uint16_t pale = tft.color565(188, 205, 124);
		const uint16_t darkest = tft.color565(23, 43, 18);
		const uint16_t dark = tft.color565(76, 95, 47);
		if (forceUpdate)
		{
			tft.fillScreen(tft.color565(154, 166, 92));
			tft.fillRect(5, 5, 310, 230, pale);
			tft.setTextSize(1.0f);
			tft.setTextColor(pale, darkest);
			tft.setTextDatum(TL_DATUM);
			tft.fillRect(9, 14, 31, 18, darkest);
			// Single-pixel clipped corners keep the label chunky but less boxy.
			tft.drawPixel(9, 14, pale);
			tft.drawPixel(39, 14, pale);
			tft.drawPixel(9, 31, pale);
			tft.drawPixel(39, 31, pale);
			tft.drawString("LAP", 15, 19, 1);
			tft.fillRect(181, 14, 31, 18, darkest);
			tft.drawPixel(181, 14, pale);
			tft.drawPixel(211, 14, pale);
			tft.drawPixel(181, 31, pale);
			tft.drawPixel(211, 31, pale);
			tft.drawString("POS", 187, 19, 1);
			for (int x = 8; x < 312; x += 8) tft.fillRect(x, 39, 4, 2, dark);
			drawPocketPixelFrame(174, 49, 136, 106, dark);
			for (int y = 84; y < 150; y += 34)
				for (int x = 180; x < 306; x += 8) tft.fillRect(x, y, 4, 2, dark);
			tft.setTextColor(darkest, pale);
			tft.setTextDatum(ML_DATUM);
			tft.drawString("BEST", 180, 71, 1);
			tft.drawString("LAST", 180, 105, 1);
			tft.drawString("CUR", 180, 139, 1);
			tft.setTextDatum(TL_DATUM);
			drawPocketPixelFrame(10, 162, 300, 69, dark);
			// Match the other dividers: FUEL's left edge is a dotted line.
			for (int y = 167; y < 227; y += 8)
				tft.fillRect(173, y, 2, 4, dark);
			// RPM needs a slight optical correction to match the wider DELTA word.
			tft.setTextSize(1.30f, 1.20f);
			tft.drawString("RPM", 18, 173, 1);
			tft.setTextSize(1.20f, 1.15f);
			tft.setTextDatum(ML_DATUM);
			tft.drawString("DELTA", 18, 216, 1);
			tft.setTextSize(1.0f);
			tft.setTextDatum(TL_DATUM);
		}

		drawPocketValue(43, 12, 90, 22, state.tyrePressureRearLeft,
			"pocketLap", darkest, pale, 2, 1.35f, 1.18f, forceUpdate);
		drawPocketValue(215, 13, 92, 20, state.tyrePressureFrontRight,
			"pocketPos", darkest, pale, 2, 1.35f, 1.18f, forceUpdate);
		drawPocketGearSpeed(state, forceUpdate);
		drawPocketValue(210, 59, 97, 24, state.bestLapTime,
			"pocketBest", darkest, pale, 1, 1.65f, 1.50f, forceUpdate);
		drawPocketValue(210, 93, 97, 24, state.lastLapTime,
			"pocketLast", darkest, pale, 1, 1.65f, 1.50f, forceUpdate);
		drawPocketValueBuffered(210, 127, 97, 24, state.currentLapTime,
			"pocketCurrent", state.lapInvalidated == "True" ? dark : darkest,
			pale, 1, 1.65f, 1.50f, forceUpdate,
			currentSprite, currentSpriteCreated);
		drawPocketRpm(state, forceUpdate);
		drawPocketValue(66, 204, 102, 24, state.sessionBestLiveDeltaSeconds,
			"pocketDelta", darkest, pale, 2, 1.0f, 1.0f, forceUpdate, MR_DATUM);
		drawPocketFuelAids(state, forceUpdate);
	}
